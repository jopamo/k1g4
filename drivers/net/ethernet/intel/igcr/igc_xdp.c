// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2020, Intel Corporation. */

#include <linux/if_vlan.h>
#include <net/xdp_sock_drv.h>

#include "igc.h"
#include "igc_xdp.h"

static void igc_xdp_stop_tx(struct igc_ring *ring)
{
	struct netdev_queue *nq = txring_txq(ring);

	/* Other RX queues and redirects can transmit after this NAPI drains. */
	__netif_tx_lock_bh(nq);
	set_bit(IGC_RING_FLAG_TX_DISABLED, &ring->flags);
	netif_tx_stop_queue(nq);
	__netif_tx_unlock_bh(nq);
}

static void igc_xdp_resume_tx(struct igc_ring *ring)
{
	struct netdev_queue *nq = txring_txq(ring);

	__netif_tx_lock_bh(nq);
	clear_bit(IGC_RING_FLAG_TX_DISABLED, &ring->flags);
	netif_tx_wake_queue(nq);
	__netif_tx_unlock_bh(nq);
}

static void igc_xdp_resume_napi(struct napi_struct *napi)
{
	napi_enable(napi);
	/* Poll once to rearm interrupts that arrived while NAPI was disabled. */
	napi_schedule(napi);
}

int igc_xdp_set_prog(struct igc_adapter *adapter, struct bpf_prog *prog,
		     struct netlink_ext_ack *extack)
{
	struct net_device *dev = adapter->netdev;
	bool if_running = netif_running(dev) &&
			  !test_bit(__IGC_DOWN, &adapter->state);
	struct bpf_prog *old_prog;
	bool need_update;
	unsigned int i;

	if (dev->mtu > ETH_DATA_LEN) {
		/* For now, the driver doesn't support XDP functionality with
		 * jumbo frames so we return error.
		 */
		NL_SET_ERR_MSG_MOD(extack, "Jumbo frames not supported");
		return -EOPNOTSUPP;
	}

	need_update = !!adapter->xdp_prog != !!prog;
	if (if_running && need_update) {
		for (i = 0; i < adapter->num_q_vectors; i++)
			napi_disable(&adapter->q_vector[i]->napi);
		synchronize_net();
		for (i = 0; i < adapter->num_tx_queues; i++)
			igc_xdp_stop_tx(adapter->tx_ring[i]);
		for (i = 0; i < adapter->num_rx_queues; i++) {
			igc_disable_rx_ring(adapter->rx_ring[i]);
			igc_disable_tx_ring(adapter->tx_ring[i]);
		}
	}

	old_prog = xchg(&adapter->xdp_prog, prog);
	if (old_prog)
		bpf_prog_put(old_prog);

	if (prog)
		xdp_features_set_redirect_target(dev, true);
	else
		xdp_features_clear_redirect_target(dev);

	if (if_running && need_update) {
		for (i = 0; i < adapter->num_rx_queues; i++) {
			igc_enable_tx_ring(adapter->tx_ring[i]);
			igc_enable_rx_ring(adapter->rx_ring[i]);
		}
		for (i = 0; i < adapter->num_tx_queues; i++)
			igc_xdp_resume_tx(adapter->tx_ring[i]);
		for (i = 0; i < adapter->num_q_vectors; i++)
			igc_xdp_resume_napi(&adapter->q_vector[i]->napi);
	}

	return 0;
}

static int igc_xdp_enable_pool(struct igc_adapter *adapter,
			       struct xsk_buff_pool *pool, u16 queue_id)
{
	struct net_device *ndev = adapter->netdev;
	struct device *dev = &adapter->pdev->dev;
	struct igc_ring *rx_ring, *tx_ring;
	struct napi_struct *napi, *tx_napi;
	bool needs_reset;
	u32 frame_size;
	int err;

	if (queue_id >= adapter->num_rx_queues ||
	    queue_id >= adapter->num_tx_queues)
		return -EINVAL;

	frame_size = xsk_pool_get_rx_frame_size(pool);
	/* Match the 1 KiB granularity of IGC_SRRCTL_BSIZEPKT(). */
	frame_size = round_down(frame_size, 1024);
	if (!FIELD_FIT(IGC_SRRCTL_BSIZEPKT_MASK, frame_size / 1024))
		return -EOPNOTSUPP;

	/* Single-buffer RX must fit two VLAN tags and inline timestamps.
	 * RCTL.SECRC strips the FCS before DMA.
	 */
	if (frame_size < ETH_FRAME_LEN + VLAN_HLEN * 2 + IGC_TS_HDR_LEN)
		return -EOPNOTSUPP;

	err = xsk_pool_dma_map(pool, dev, IGC_RX_DMA_ATTR);
	if (err) {
		netdev_err(ndev, "Failed to map xsk pool\n");
		return err;
	}

	needs_reset = netif_running(adapter->netdev) &&
		      !test_bit(__IGC_DOWN, &adapter->state) &&
		      igc_xdp_is_enabled(adapter);

	rx_ring = adapter->rx_ring[queue_id];
	tx_ring = adapter->tx_ring[queue_id];
	napi = &rx_ring->q_vector->napi;
	tx_napi = &tx_ring->q_vector->napi;

	if (needs_reset) {
		napi_disable(napi);
		if (tx_napi != napi)
			napi_disable(tx_napi);
		synchronize_net();
		igc_xdp_stop_tx(tx_ring);
		igc_disable_rx_ring(rx_ring);
		igc_disable_tx_ring(tx_ring);
	}

	set_bit(queue_id, &adapter->xsk_pool_mask);

	if (needs_reset) {
		igc_enable_rx_ring(rx_ring);
		igc_enable_tx_ring(tx_ring);
		igc_xdp_resume_tx(tx_ring);
		igc_xdp_resume_napi(napi);
		if (tx_napi != napi)
			igc_xdp_resume_napi(tx_napi);
	}

	return 0;
}

static int igc_xdp_disable_pool(struct igc_adapter *adapter, u16 queue_id)
{
	struct igc_ring *rx_ring, *tx_ring;
	struct xsk_buff_pool *pool;
	struct napi_struct *napi, *tx_napi;
	bool needs_reset;

	if (queue_id >= IGC_MAX_RX_QUEUES || queue_id >= IGC_MAX_TX_QUEUES)
		return -EINVAL;

	pool = xsk_get_pool_from_qid(adapter->netdev, queue_id);
	if (!pool)
		return -EINVAL;

	rx_ring = adapter->rx_ring[queue_id];
	tx_ring = adapter->tx_ring[queue_id];
	needs_reset = netif_running(adapter->netdev) &&
		      !test_bit(__IGC_DOWN, &adapter->state) &&
		      igc_xdp_is_enabled(adapter) && rx_ring && tx_ring;

	if (needs_reset) {
		napi = &rx_ring->q_vector->napi;
		tx_napi = &tx_ring->q_vector->napi;
		napi_disable(napi);
		if (tx_napi != napi)
			napi_disable(tx_napi);
		synchronize_net();
		igc_xdp_stop_tx(tx_ring);
		igc_disable_rx_ring(rx_ring);
		igc_disable_tx_ring(tx_ring);
	}

	clear_bit(queue_id, &adapter->xsk_pool_mask);
	xsk_pool_dma_unmap(pool, IGC_RX_DMA_ATTR);
	if (rx_ring)
		rx_ring->xsk_pool = NULL;
	if (tx_ring)
		tx_ring->xsk_pool = NULL;

	if (needs_reset) {
		igc_enable_rx_ring(rx_ring);
		igc_enable_tx_ring(tx_ring);
		igc_xdp_resume_tx(tx_ring);
		igc_xdp_resume_napi(napi);
		if (tx_napi != napi)
			igc_xdp_resume_napi(tx_napi);
	}

	return 0;
}

int igc_xdp_setup_pool(struct igc_adapter *adapter, struct xsk_buff_pool *pool,
		       u16 queue_id)
{
	return pool ? igc_xdp_enable_pool(adapter, pool, queue_id) :
		      igc_xdp_disable_pool(adapter, queue_id);
}
