// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/ethtool_netlink.h>
#include <linux/netdevice.h>
#include <net/netdev_lock.h>
#include <net/netdev_queues.h>
#include <net/netdev_rx_queue.h>

#include "dev.h"
#include "page_pool_priv.h"

void netdev_rx_queue_lease(struct netdev_rx_queue *rxq_dst,
			   struct netdev_rx_queue *rxq_src)
{
	netdev_assert_locked(rxq_src->dev);
	netdev_assert_locked(rxq_dst->dev);

	netdev_hold(rxq_src->dev, &rxq_src->lease_tracker, GFP_KERNEL);

	WRITE_ONCE(rxq_src->lease, rxq_dst);
	WRITE_ONCE(rxq_dst->lease, rxq_src);
}

void netdev_rx_queue_unlease(struct netdev_rx_queue *rxq_dst,
			     struct netdev_rx_queue *rxq_src)
{
	netdev_assert_locked(rxq_dst->dev);
	netdev_assert_locked(rxq_src->dev);

	WRITE_ONCE(rxq_src->lease, NULL);
	WRITE_ONCE(rxq_dst->lease, NULL);

	netdev_put(rxq_src->dev, &rxq_src->lease_tracker);
}

bool netif_rxq_is_leased(struct net_device *dev, unsigned int rxq_idx)
{
	if (rxq_idx < dev->real_num_rx_queues)
		return READ_ONCE(__netif_get_rx_queue(dev, rxq_idx)->lease);
	return false;
}

/* Virtual devices eligible for leasing have no dev->dev.parent, while
 * physical devices always have one. Use this to enforce the correct
 * lease traversal direction.
 */
static bool netif_lease_dir_ok(const struct net_device *dev,
			       enum netif_lease_dir dir)
{
	if (dir == NETIF_VIRT_TO_PHYS && !dev->dev.parent)
		return true;
	if (dir == NETIF_PHYS_TO_VIRT && dev->dev.parent)
		return true;
	return false;
}

bool netif_is_queue_leasee(const struct net_device *dev)
{
	return netif_lease_dir_ok(dev, NETIF_VIRT_TO_PHYS);
}

struct netdev_rx_queue *
__netif_get_rx_queue_lease(struct net_device **dev, unsigned int *rxq_idx,
			   enum netif_lease_dir dir)
{
	struct net_device *orig_dev = *dev;
	struct netdev_rx_queue *rxq = __netif_get_rx_queue(orig_dev, *rxq_idx);

	if (rxq->lease) {
		if (!netif_lease_dir_ok(orig_dev, dir))
			return NULL;
		rxq = rxq->lease;
		*rxq_idx = get_netdev_rx_queue_index(rxq);
		*dev = rxq->dev;
	}
	return rxq;
}

static int netdev_rx_queue_reconfig(struct net_device *dev,
				    unsigned int rxq_idx,
				    struct netdev_queue_config *qcfg_old,
				    struct netdev_queue_config *qcfg_new)
{
	const struct netdev_queue_mgmt_ops *qops = dev->queue_mgmt_ops;
	void *new_mem, *old_mem;
	int err;

	if (!qops || !qops->ndo_queue_stop || !qops->ndo_queue_mem_free ||
	    !qops->ndo_queue_mem_alloc || !qops->ndo_queue_start)
		return -EOPNOTSUPP;

	netdev_assert_locked(dev);

	new_mem = kvzalloc(qops->ndo_queue_mem_size, GFP_KERNEL);
	if (!new_mem)
		return -ENOMEM;

	old_mem = kvzalloc(qops->ndo_queue_mem_size, GFP_KERNEL);
	if (!old_mem) {
		err = -ENOMEM;
		goto err_free_new_mem;
	}

	err = qops->ndo_queue_mem_alloc(dev, qcfg_new, new_mem, rxq_idx);
	if (err)
		goto err_free_old_mem;

	if (netif_running(dev)) {
		err = qops->ndo_queue_stop(dev, old_mem, rxq_idx);
		if (err)
			goto err_free_new_queue_mem;

		err = qops->ndo_queue_start(dev, qcfg_new, new_mem, rxq_idx);
		if (err)
			goto err_start_queue;
	} else {
		swap(new_mem, old_mem);
	}

	qops->ndo_queue_mem_free(dev, old_mem);

	kvfree(old_mem);
	kvfree(new_mem);

	return 0;

err_start_queue:
	/* Restarting the queue with old_mem should be successful as we haven't
	 * changed any of the queue configuration, and there is not much we can
	 * do to recover from a failure here.
	 *
	 * WARN if we fail to recover the old rx queue, and at least free
	 * old_mem so we don't also leak that.
	 */
	if (qops->ndo_queue_start(dev, qcfg_old, old_mem, rxq_idx)) {
		WARN(1,
		     "Failed to restart old queue in error path. RX queue %d may be unhealthy.",
		     rxq_idx);
		qops->ndo_queue_mem_free(dev, old_mem);
	}

err_free_new_queue_mem:
	qops->ndo_queue_mem_free(dev, new_mem);

err_free_old_mem:
	kvfree(old_mem);

err_free_new_mem:
	kvfree(new_mem);

	return err;
}

int netdev_rx_queue_restart(struct net_device *dev, unsigned int rxq_idx)
{
	struct netdev_queue_config qcfg;

	netdev_queue_config(dev, rxq_idx, &qcfg);
	return netdev_rx_queue_reconfig(dev, rxq_idx, &qcfg, &qcfg);
}
EXPORT_SYMBOL_NS_GPL(netdev_rx_queue_restart, "NETDEV_INTERNAL");
