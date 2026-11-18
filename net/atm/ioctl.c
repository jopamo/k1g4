// SPDX-License-Identifier: GPL-2.0
/* ATM ioctl handling */

/* Written 1995-2000 by Werner Almesberger, EPFL LRC/ICA */
/* 2003 John Levon  <levon@movementarian.org> */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/net.h>		/* struct socket, struct proto_ops */
#include <linux/atm.h>		/* ATM stuff */
#include <linux/atmdev.h>
#include <linux/capability.h>
#include <linux/mutex.h>
#include <asm/ioctls.h>
#include <net/compat.h>

#include "resources.h"
#include "common.h"


static DEFINE_MUTEX(ioctl_mutex);
static LIST_HEAD(ioctl_list);


void register_atm_ioctl(struct atm_ioctl *ioctl)
{
	mutex_lock(&ioctl_mutex);
	list_add_tail(&ioctl->list, &ioctl_list);
	mutex_unlock(&ioctl_mutex);
}
EXPORT_SYMBOL(register_atm_ioctl);

void deregister_atm_ioctl(struct atm_ioctl *ioctl)
{
	mutex_lock(&ioctl_mutex);
	list_del(&ioctl->list);
	mutex_unlock(&ioctl_mutex);
}
EXPORT_SYMBOL(deregister_atm_ioctl);

static int do_vcc_ioctl(struct socket *sock, unsigned int cmd,
			unsigned long arg, int compat)
{
	struct sock *sk = sock->sk;
	struct atm_vcc *vcc;
	int error;
	struct list_head *pos;
	void __user *argp = (void __user *)arg;
	void __user *buf;
	int __user *len;

	vcc = ATM_SD(sock);
	switch (cmd) {
	case SIOCOUTQ:
		if (sock->state != SS_CONNECTED ||
		    !test_bit(ATM_VF_READY, &vcc->flags)) {
			error =  -EINVAL;
			goto done;
		}
		error = put_user(sk->sk_sndbuf - sk_wmem_alloc_get(sk),
				 (int __user *)argp);
		goto done;
	case SIOCINQ:
	{
		struct sk_buff *skb;
		int amount;

		if (sock->state != SS_CONNECTED) {
			error = -EINVAL;
			goto done;
		}
		spin_lock_irq(&sk->sk_receive_queue.lock);
		skb = skb_peek(&sk->sk_receive_queue);
		amount = skb ? skb->len : 0;
		spin_unlock_irq(&sk->sk_receive_queue.lock);
		error = put_user(amount, (int __user *)argp);
		goto done;
	}
	case ATM_SETSC:
		net_warn_ratelimited("ATM_SETSC is obsolete; used by %s:%d\n",
				     current->comm, task_pid_nr(current));
		error = 0;
		goto done;
	case ATM_SETBACKEND:
	case ATM_NEWBACKENDIF:
	{
		atm_backend_t backend;
		error = get_user(backend, (atm_backend_t __user *)argp);
		if (error)
			goto done;
		switch (backend) {
		case ATM_BACKEND_PPP:
			request_module("pppoatm");
			break;
		case ATM_BACKEND_BR2684:
			request_module("br2684");
			break;
		}
		break;
	}
	}

	error = -ENOIOCTLCMD;

	mutex_lock(&ioctl_mutex);
	list_for_each(pos, &ioctl_list) {
		struct atm_ioctl *ic = list_entry(pos, struct atm_ioctl, list);
		if (try_module_get(ic->owner)) {
			error = ic->ioctl(sock, cmd, arg);
			module_put(ic->owner);
			if (error != -ENOIOCTLCMD)
				break;
		}
	}
	mutex_unlock(&ioctl_mutex);

	if (error != -ENOIOCTLCMD)
		goto done;

	if (cmd == ATM_GETNAMES) {
		if (IS_ENABLED(CONFIG_COMPAT) && compat) {
		} else {
			struct atm_iobuf __user *iobuf = argp;
			len = &iobuf->length;
			if (get_user(buf, &iobuf->buffer))
				return -EFAULT;
		}
		error = atm_getnames(buf, len);
	} else {
		int number;

		if (IS_ENABLED(CONFIG_COMPAT) && compat) {
		} else {
			struct atmif_sioc __user *sioc = argp;

			len = &sioc->length;
			if (get_user(buf, &sioc->arg))
				return -EFAULT;
			if (get_user(number, &sioc->number))
				return -EFAULT;
		}
		error = atm_dev_ioctl(cmd, buf, len, number, compat);
	}

done:
	return error;
}

int vcc_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg)
{
	return do_vcc_ioctl(sock, cmd, arg, 0);
}

