// SPDX-License-Identifier: GPL-2.0

#include <linux/compat.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/major.h>

#include "fb_internal.h"
#include "fbcon.h"

/*
 * We hold a reference to the fb_info in file->private_data,
 * but if the current registered fb has changed, we don't
 * actually want to use it.
 *
 * So look up the fb_info using the inode minor number,
 * and just verify it against the reference we have.
 */
static struct fb_info *file_fb_info(struct file *file)
{
	struct inode *inode = file_inode(file);
	int fbidx = iminor(inode);
	struct fb_info *info = registered_fb[fbidx];

	if (info != file->private_data)
		info = NULL;
	return info;
}

static ssize_t fb_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	struct fb_info *info = file_fb_info(file);

	if (!info)
		return -ENODEV;

	if (fb_WARN_ON_ONCE(info, !info->fbops->fb_read))
		return -EINVAL;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	return info->fbops->fb_read(info, buf, count, ppos);
}

static ssize_t fb_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	struct fb_info *info = file_fb_info(file);

	if (!info)
		return -ENODEV;

	if (fb_WARN_ON_ONCE(info, !info->fbops->fb_write))
		return -EINVAL;

	if (info->state != FBINFO_STATE_RUNNING)
		return -EPERM;

	return info->fbops->fb_write(info, buf, count, ppos);
}

static long do_fb_ioctl(struct fb_info *info, unsigned int cmd,
			unsigned long arg)
{
	const struct fb_ops *fb;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	struct fb_cmap cmap_from;
	struct fb_cmap_user cmap;
	void __user *argp = (void __user *)arg;
	long ret = 0;

	switch (cmd) {
	case FBIOGET_VSCREENINFO:
		lock_fb_info(info);
		var = info->var;
		unlock_fb_info(info);

		ret = copy_to_user(argp, &var, sizeof(var)) ? -EFAULT : 0;
		break;
	case FBIOPUT_VSCREENINFO:
		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;
		/* only for kernel-internal use */
		var.activate &= ~FB_ACTIVATE_KD_TEXT;
		console_lock();
		lock_fb_info(info);
		ret = fb_set_var_from_user(info, &var);
		unlock_fb_info(info);
		console_unlock();
		if (!ret && copy_to_user(argp, &var, sizeof(var)))
			ret = -EFAULT;
		break;
	case FBIOGET_FSCREENINFO:
		lock_fb_info(info);
		memcpy(&fix, &info->fix, sizeof(fix));
		if (info->flags & FBINFO_HIDE_SMEM_START)
			fix.smem_start = 0;
		unlock_fb_info(info);

		ret = copy_to_user(argp, &fix, sizeof(fix)) ? -EFAULT : 0;
		break;
	case FBIOPUTCMAP:
		if (copy_from_user(&cmap, argp, sizeof(cmap)))
			return -EFAULT;
		ret = fb_set_user_cmap(&cmap, info);
		break;
	case FBIOGETCMAP:
		if (copy_from_user(&cmap, argp, sizeof(cmap)))
			return -EFAULT;
		lock_fb_info(info);
		cmap_from = info->cmap;
		unlock_fb_info(info);
		ret = fb_cmap_to_user(&cmap_from, &cmap);
		break;
	case FBIOPAN_DISPLAY:
		if (copy_from_user(&var, argp, sizeof(var)))
			return -EFAULT;
		console_lock();
		lock_fb_info(info);
		ret = fb_pan_display(info, &var);
		unlock_fb_info(info);
		console_unlock();
		if (ret == 0 && copy_to_user(argp, &var, sizeof(var)))
			return -EFAULT;
		break;
	case FBIO_CURSOR:
		ret = -EINVAL;
		break;
	case FBIOGET_CON2FBMAP:
		ret = fbcon_get_con2fb_map_ioctl(argp);
		break;
	case FBIOPUT_CON2FBMAP:
		ret = fbcon_set_con2fb_map_ioctl(argp);
		break;
	case FBIOBLANK:
		if (arg > FB_BLANK_POWERDOWN)
			return -EINVAL;
		console_lock();
		lock_fb_info(info);
		ret = fb_blank_from_user(info, arg);
		unlock_fb_info(info);
		console_unlock();
		break;
	default:
		lock_fb_info(info);
		fb = info->fbops;
		if (fb->fb_ioctl)
			ret = fb->fb_ioctl(info, cmd, arg);
		else
			ret = -ENOTTY;
		unlock_fb_info(info);
	}
	return ret;
}

static long fb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct fb_info *info = file_fb_info(file);

	if (!info)
		return -ENODEV;
	return do_fb_ioctl(info, cmd, arg);
}


static int fb_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct fb_info *info = file_fb_info(file);
	int res;

	if (!info)
		return -ENODEV;

	if (fb_WARN_ON_ONCE(info, !info->fbops->fb_mmap))
		return -ENODEV;

	mutex_lock(&info->mm_lock);
	res = info->fbops->fb_mmap(info, vma);
	mutex_unlock(&info->mm_lock);

	return res;
}

static int fb_open(struct inode *inode, struct file *file)
__acquires(&info->lock)
__releases(&info->lock)
{
	int fbidx = iminor(inode);
	struct fb_info *info;
	int res = 0;

	info = get_fb_info(fbidx);
	if (!info) {
		request_module("fb%d", fbidx);
		info = get_fb_info(fbidx);
		if (!info)
			return -ENODEV;
	}
	if (IS_ERR(info))
		return PTR_ERR(info);

	lock_fb_info(info);
	if (!try_module_get(info->fbops->owner)) {
		res = -ENODEV;
		goto out;
	}
	file->private_data = info;
	if (info->fbops->fb_open) {
		res = info->fbops->fb_open(info, 1);
		if (res)
			module_put(info->fbops->owner);
	}
#ifdef CONFIG_FB_DEFERRED_IO
	if (info->fbdefio)
		fb_deferred_io_open(info, inode, file);
#endif
out:
	unlock_fb_info(info);
	if (res)
		put_fb_info(info);
	return res;
}

static int fb_release(struct inode *inode, struct file *file)
__acquires(&info->lock)
__releases(&info->lock)
{
	struct fb_info * const info = file->private_data;

	lock_fb_info(info);
#if IS_ENABLED(CONFIG_FB_DEFERRED_IO)
	if (info->fbdefio)
		fb_deferred_io_release(info);
#endif
	if (info->fbops->fb_release)
		info->fbops->fb_release(info, 1);
	module_put(info->fbops->owner);
	unlock_fb_info(info);
	put_fb_info(info);
	return 0;
}

#if defined(CONFIG_FB_PROVIDE_GET_FB_UNMAPPED_AREA) && !defined(CONFIG_MMU)
static unsigned long get_fb_unmapped_area(struct file *filp,
				   unsigned long addr, unsigned long len,
				   unsigned long pgoff, unsigned long flags)
{
	struct fb_info * const info = filp->private_data;
	unsigned long fb_size = PAGE_ALIGN(info->fix.smem_len);

	if (pgoff > fb_size || len > fb_size - pgoff)
		return -EINVAL;

	return (unsigned long)info->screen_base + pgoff;
}
#endif

static const struct file_operations fb_fops = {
	.owner = THIS_MODULE,
	.read = fb_read,
	.write = fb_write,
	.unlocked_ioctl = fb_ioctl,
	.mmap = fb_mmap,
	.open = fb_open,
	.release = fb_release,
#if defined(HAVE_ARCH_FB_UNMAPPED_AREA) || \
	(defined(CONFIG_FB_PROVIDE_GET_FB_UNMAPPED_AREA) && \
	 !defined(CONFIG_MMU))
	.get_unmapped_area = get_fb_unmapped_area,
#endif
#ifdef CONFIG_FB_DEFERRED_IO
	.fsync = fb_deferred_io_fsync,
#endif
	.llseek = default_llseek,
};

int fb_register_chrdev(void)
{
	int ret;

	ret = register_chrdev(FB_MAJOR, "fb", &fb_fops);
	if (ret) {
		pr_err("Unable to get major %d for fb devs\n", FB_MAJOR);
		return ret;
	}

	return ret;
}

void fb_unregister_chrdev(void)
{
	unregister_chrdev(FB_MAJOR, "fb");
}
