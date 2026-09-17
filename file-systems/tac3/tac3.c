// SPDX-License-Identifier: GPL-2.0
/*
 * tac3.c — TAC3: Tripartite Addressable Cache, 3-Table Edition
 *
 * A mountable, N-way redundant filesystem whose reads and writes are serviced
 * ONLY through its own kernel-call handles. register_filesystem(&tac3_fs_type)
 * publishes the "tac3" mount; from then on every read()/write()/mmap()/fsync()
 * that lands on a TAC3 mount is dispatched through the operation tables defined
 * here (tac3_file_operations, tac3_*_inode_operations, tac3_aops,
 * tac3_super_operations). There is no side channel: the standard VFS entry
 * points are the new handles, and each serviced I/O feeds the wear/pressure/
 * health engine (tac3_record_access).
 *
 * TAC3 maintains three coordinated tables over an N-layer ("multitude")
 * redundant file table (default 10):
 *
 *   Table 1 (FILE)   - file entries replicated across N layers.
 *   Table 2 (HEALTH) - per-layer/per-region read/write/press/wear + disk health.
 *   Table 3 (ADMIN)  - administrative/state properties (facts + opaque values).
 *
 * Each read derives a QUALITY (how well it met the device spec) and a PRESSURE
 * (instantaneous load appealed to the device). Re-reading a region raises its
 * read-heat; heavy/jarring access records extra impact spread across the N
 * layers. Layer and file-table health are monitored using a green/white/yellow
 * model (no red-alarm), consistent with aptitude/health.
 *
 * ETHICS: Table 3 stores administrative facts and OPERATOR-SUPPLIED opaque
 * values only. The kernel does NOT compute, infer, or judge any person's
 * intelligence, worth, feelings, learning, friendships, or standing.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/statfs.h>
#include <linux/backing-dev.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>
#include "tac3.h"

/* Boot/module defaults; per-mount values live in struct tac3_sb_info. */
static u32 default_multitude = TAC3_MULT_DEFAULT;
module_param(default_multitude, uint, 0644);
MODULE_PARM_DESC(default_multitude,
		 "Default file-table redundancy factor (1,3,5,10..; default 10)");

static u32 default_device_class = TAC3_DEV_NVME_GEN4;
module_param(default_device_class, uint, 0644);
MODULE_PARM_DESC(default_device_class,
		 "Default backing storage device class (enum tac3_device_class)");

/* ==========================================================================
 * Wear / pressure / health engine (Tables 2 & 3)
 * ========================================================================== */

/* Device spec ceiling (MB/s) for the configured class. */
static u32 tac3_speed_ceiling(u32 cls)
{
	switch (cls) {
	case TAC3_DEV_IDE_HDD:   return TAC3_SPEED_IDE_HDD;
	case TAC3_DEV_SATA_HDD:  return TAC3_SPEED_SATA_HDD;
	case TAC3_DEV_SAS_HDD:   return TAC3_SPEED_SAS_HDD;
	case TAC3_DEV_SATA_SSD:  return TAC3_SPEED_SATA_SSD;
	case TAC3_DEV_NVME_GEN3: return TAC3_SPEED_NVME_GEN3;
	case TAC3_DEV_NVME_GEN4: return TAC3_SPEED_NVME_GEN4;
	case TAC3_DEV_NVME_GEN5: return TAC3_SPEED_NVME_GEN5;
	case TAC3_DEV_USB2:      return TAC3_SPEED_USB2;
	case TAC3_DEV_USB3:      return TAC3_SPEED_USB3;
	case TAC3_DEV_USB4:      return TAC3_SPEED_USB4;
	default:                 return TAC3_SPEED_SATA_SSD;
	}
}

/*
 * Read QUALITY (‰) — how well the observed throughput met the device spec.
 * quality = min(1000, observed*1000/ceiling).
 */
static u32 tac3_quality(u32 device_class, u32 observed_mbps)
{
	u32 ceil = tac3_speed_ceiling(device_class);
	u64 q;

	if (!ceil)
		return 0;
	q = (u64)observed_mbps * 1000ULL;
	do_div(q, ceil);
	return (q > 1000) ? 1000 : (u32)q;
}

/*
 * Read PRESSURE (‰) — instantaneous load appealed to the device, rising with
 * region read-heat. Bounded 0..1000. No floats: +50 per doubling of heat.
 */
static u32 tac3_pressure(u64 read_heat)
{
	u32 p = 0;
	u64 h = read_heat;
	while (h && p < 1000) { p += 50; h >>= 1; }
	return p;
}

/* Derive a layer's green/white/yellow state from its health value. */
static enum tac3_health_state tac3_state_of(u32 disk_health, u32 errors)
{
	if (errors > 0 || disk_health < 600)
		return TAC3_YELLOW;            /* attention recommended */
	if (disk_health < 850)
		return TAC3_WHITE;             /* informational / normal */
	return TAC3_GREEN;                 /* healthy / verified */
}

/*
 * Record one access against (layer, region) on a given TAC3 instance.
 * op: 0=read, 1=write. jarring != 0 marks heavy/abrupt access -> extra impact
 * spread across all N layers. observed_mbps is the read throughput (ignored
 * for writes).
 */
void tac3_record_access(struct tac3_sb_info *sbi, u32 layer, u32 region,
			int op, u32 observed_mbps, int jarring)
{
	struct tac3_layer_health *L;
	struct tac3_region_wear *R;
	unsigned long flags;
	u32 quality, pressure, i;

	if (!sbi || layer >= sbi->multitude || region >= TAC3_MAX_REGIONS)
		return;

	spin_lock_irqsave(&sbi->lock, flags);
	L = &sbi->layers[layer];
	R = &L->region[region];

	if (op == 0) {                     /* read */
		R->reads++; L->total_reads++;
		R->read_heat++; L->total_read_heat++;
		quality  = tac3_quality(sbi->device_class, observed_mbps);
		pressure = tac3_pressure(R->read_heat);
		R->last_quality = quality;
		R->last_pressure = pressure;
		if (pressure > R->peak_pressure) R->peak_pressure = pressure;
		L->avg_quality  += (quality  - L->avg_quality)  >> 3;
		L->avg_pressure += (pressure - L->avg_pressure) >> 3;
	} else {                           /* write */
		R->writes++; L->total_writes++;
		R->write_wear++; L->total_write_wear++;
	}

	if (jarring) {
		R->jarring_events++;
		L->total_jarring++;
		for (i = 0; i < sbi->multitude; i++)
			sbi->layers[i].total_jarring++;
	}

	{
		u64 penalty = (L->total_write_wear >> 6)
			    + (L->total_jarring >> 2)
			    + (u64)L->error_count * 20ULL;
		L->disk_health = (penalty >= 1000) ? 0 : (u32)(1000 - penalty);
		L->state = tac3_state_of(L->disk_health, L->error_count);
	}

	sbi->admin.updated_unix = ktime_get_real_seconds();
	spin_unlock_irqrestore(&sbi->lock, flags);
}
EXPORT_SYMBOL_GPL(tac3_record_access);

/* Aggregate file-table health across layers (min of layer healths). */
static u32 tac3_file_table_health(struct tac3_sb_info *sbi)
{
	u32 i, m = 1000;
	for (i = 0; i < sbi->multitude; i++)
		if (sbi->layers[i].disk_health < m)
			m = sbi->layers[i].disk_health;
	return m;
}

static const char *state_str(enum tac3_health_state s)
{
	switch (s) {
	case TAC3_GREEN:  return "GREEN";
	case TAC3_WHITE:  return "WHITE";
	case TAC3_YELLOW: return "YELLOW";
	default:          return "?";
	}
}

/*
 * Map a byte offset to (layer, region) for wear accounting. The primary layer
 * services the I/O; region is a coarse bucket over the file offset. A "jarring"
 * access is a large single transfer relative to a page-cluster.
 */
static void tac3_locate(struct inode *inode, loff_t pos, size_t len,
			u32 *layer, u32 *region, int *jarring)
{
	struct tac3_sb_info *sbi = TAC3_SB(inode->i_sb);
	struct tac3_inode_info *ci = TAC3_I(inode);
	u64 blk = (u64)pos >> PAGE_SHIFT;

	*layer  = ci->entry.primary_layer % (sbi->multitude ? sbi->multitude : 1);
	*region = (u32)((ci->region + blk) % TAC3_MAX_REGIONS);
	/* A transfer spanning many pages at once is treated as jarring. */
	*jarring = (len >= (16u << PAGE_SHIFT)) ? 1 : 0;
}

/*
 * Estimate observed throughput (MB/s) for a completed transfer of `bytes`
 * taking `ns` nanoseconds. Used to derive read quality against the spec.
 */
static u32 tac3_observed_mbps(size_t bytes, u64 ns)
{
	u64 mbps;
	if (!ns)
		ns = 1;
	/* bytes/ns * 1e9 / 1e6 == bytes*1000/ns  (MB/s, decimal MB) */
	mbps = (u64)bytes * 1000ULL;
	do_div(mbps, ns);
	return (mbps > 0xffffffffULL) ? 0xffffffffU : (u32)mbps;
}

/* ==========================================================================
 * address_space_operations  (page-cache backing for Table 1 data)
 * ==========================================================================
 * TAC3 keeps file data in the page cache; dirty pages are considered
 * up-to-date (memory-backed instance). All the standard read/write helpers
 * (generic_file_read_iter etc.) drive these hooks, so data movement is owned
 * by TAC3's own address_space_operations.
 */
static int tac3_readpage(struct file *file, struct page *page)
{
	/* Fresh pages read as zero-filled and up-to-date (memory-backed). */
	clear_highpage(page);
	flush_dcache_page(page);
	SetPageUptodate(page);
	unlock_page(page);
	return 0;
}

static int tac3_write_begin(struct file *file, struct address_space *mapping,
			    loff_t pos, unsigned int len, unsigned int flags,
			    struct page **pagep, void **fsdata)
{
	struct page *page;
	pgoff_t index = pos >> PAGE_SHIFT;

	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page)
		return -ENOMEM;
	*pagep = page;

	if (!PageUptodate(page) && (len != PAGE_SIZE)) {
		unsigned int from = pos & (PAGE_SIZE - 1);

		zero_user_segments(page, 0, from, from + len, PAGE_SIZE);
	}
	return 0;
}

static int tac3_write_end(struct file *file, struct address_space *mapping,
			  loff_t pos, unsigned int len, unsigned int copied,
			  struct page *page, void *fsdata)
{
	struct inode *inode = page->mapping->host;
	loff_t last = pos + copied;

	if (!PageUptodate(page)) {
		if (copied < len) {
			unsigned int from = pos & (PAGE_SIZE - 1);

			zero_user(page, from + copied, len - copied);
		}
		SetPageUptodate(page);
	}
	if (last > inode->i_size)
		i_size_write(inode, last);

	set_page_dirty(page);
	unlock_page(page);
	put_page(page);
	return copied;
}

/*
 * TAC3 keeps data in the page cache with no device writeback (memory-backed
 * instance), mirroring ramfs' ram_aops. Movement is owned by these hooks.
 */
const struct address_space_operations tac3_aops = {
	.readpage	= tac3_readpage,
	.write_begin	= tac3_write_begin,
	.write_end	= tac3_write_end,
	.set_page_dirty	= __set_page_dirty_no_writeback,
};
EXPORT_SYMBOL_GPL(tac3_aops);

/* ==========================================================================
 * file_operations  (the new read/write kernel-call handles)
 * ==========================================================================
 * These wrappers are the sole entry points for I/O on a TAC3 file. Each times
 * the transfer, feeds tac3_record_access() with the derived layer/region and
 * observed throughput, then defers to the generic page-cache movers.
 */
static ssize_t tac3_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	loff_t pos = iocb->ki_pos;
	size_t want = iov_iter_count(to);
	u64 t0 = ktime_get_ns();
	u32 layer, region; int jarring;
	ssize_t ret;

	ret = generic_file_read_iter(iocb, to);
	if (ret > 0) {
		u64 ns = ktime_get_ns() - t0;
		tac3_locate(inode, pos, (size_t)ret, &layer, &region, &jarring);
		tac3_record_access(TAC3_SB(inode->i_sb), layer, region,
				   /*read*/0, tac3_observed_mbps(ret, ns), jarring);
	}
	(void)want;
	return ret;
}

static ssize_t tac3_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	loff_t pos = iocb->ki_pos;
	size_t want = iov_iter_count(from);
	u32 layer, region; int jarring;
	ssize_t ret;

	ret = generic_file_write_iter(iocb, from);
	if (ret > 0) {
		tac3_locate(inode, pos, (size_t)ret, &layer, &region, &jarring);
		tac3_record_access(TAC3_SB(inode->i_sb), layer, region,
				   /*write*/1, 0, jarring);
	}
	(void)want;
	return ret;
}

static int tac3_file_open(struct inode *inode, struct file *file)
{
	return generic_file_open(inode, file);
}

static int tac3_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int tac3_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	/*
	 * Memory-backed instance: there is no device to flush, so this is a
	 * no-op sync (as in ramfs). Routed through our own handle so the flush
	 * path is owned by TAC3 rather than the generic default.
	 */
	return noop_fsync(file, start, end, datasync);
}

static int tac3_mmap(struct file *file, struct vm_area_struct *vma)
{
	return generic_file_mmap(file, vma);
}

const struct file_operations tac3_file_operations = {
	.owner		= THIS_MODULE,
	.llseek		= generic_file_llseek,
	.read_iter	= tac3_read_iter,
	.write_iter	= tac3_write_iter,
	.mmap		= tac3_mmap,
	.open		= tac3_file_open,
	.release	= tac3_release,
	.fsync		= tac3_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
};
EXPORT_SYMBOL_GPL(tac3_file_operations);

const struct file_operations tac3_dir_operations = {
	.owner		= THIS_MODULE,
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.iterate_shared	= dcache_readdir,
	.fsync		= noop_fsync,
};
EXPORT_SYMBOL_GPL(tac3_dir_operations);

/* ==========================================================================
 * inode allocation + inode_operations
 * ========================================================================== */
static struct inode *tac3_alloc_inode(struct super_block *sb);
static void tac3_free_inode(struct inode *inode);

static struct inode *tac3_get_inode(struct super_block *sb,
				    const struct inode *dir, umode_t mode,
				    dev_t dev)
{
	struct tac3_sb_info *sbi = TAC3_SB(sb);
	struct inode *inode = new_inode(sb);
	struct tac3_inode_info *ci;
	unsigned long flags;

	if (!inode)
		return NULL;

	ci = TAC3_I(inode);
	inode->i_ino = get_next_ino();
	inode_init_owner(&init_user_ns, inode, dir, mode);
	inode->i_mapping->a_ops = &tac3_aops;
	mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);

	/* Table 1 entry for this inode, spread across the N layers. */
	spin_lock_irqsave(&sbi->lock, flags);
	ci->entry.ino = inode->i_ino;
	ci->entry.size = 0;
	ci->entry.primary_layer = (u32)(sbi->next_ino % sbi->multitude);
	ci->entry.layer_mask = (sbi->multitude >= 32)
			     ? 0xffffffffu : ((1u << sbi->multitude) - 1);
	ci->region = (u32)(sbi->next_ino % TAC3_MAX_REGIONS);
	sbi->next_ino++;
	spin_unlock_irqrestore(&sbi->lock, flags);

	switch (mode & S_IFMT) {
	case S_IFREG:
		inode->i_op = &tac3_file_inode_operations;
		inode->i_fop = &tac3_file_operations;
		break;
	case S_IFDIR:
		inode->i_op = &tac3_dir_inode_operations;
		inode->i_fop = &tac3_dir_operations;
		inc_nlink(inode);       /* account for "." */
		break;
	case S_IFLNK:
		inode->i_op = &page_symlink_inode_operations;
		inode_nohighmem(inode);
		break;
	default:
		init_special_inode(inode, mode, dev);
		break;
	}
	return inode;
}

static int tac3_mknod(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode *inode = tac3_get_inode(dir->i_sb, dir, mode, dev);

	if (!inode)
		return -ENOSPC;
	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = current_time(dir);
	return 0;
}

static int tac3_create(struct user_namespace *mnt_userns, struct inode *dir,
		       struct dentry *dentry, umode_t mode, bool excl)
{
	return tac3_mknod(mnt_userns, dir, dentry, mode | S_IFREG, 0);
}

static int tac3_mkdir(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode)
{
	int ret = tac3_mknod(mnt_userns, dir, dentry, mode | S_IFDIR, 0);

	if (!ret)
		inc_nlink(dir);
	return ret;
}

static int tac3_symlink(struct user_namespace *mnt_userns, struct inode *dir,
			struct dentry *dentry, const char *symname)
{
	struct inode *inode = tac3_get_inode(dir->i_sb, dir, S_IFLNK | 0777, 0);
	int len, ret;

	if (!inode)
		return -ENOSPC;
	len = strlen(symname) + 1;
	ret = page_symlink(inode, symname, len);
	if (ret) {
		iput(inode);
		return ret;
	}
	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = current_time(dir);
	return 0;
}

const struct inode_operations tac3_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};
EXPORT_SYMBOL_GPL(tac3_file_inode_operations);

const struct inode_operations tac3_dir_inode_operations = {
	.create		= tac3_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= tac3_symlink,
	.mkdir		= tac3_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= tac3_mknod,
	.rename		= simple_rename,
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};
EXPORT_SYMBOL_GPL(tac3_dir_inode_operations);

/* ==========================================================================
 * super_operations
 * ========================================================================== */
static struct kmem_cache *tac3_inode_cachep;

static struct inode *tac3_alloc_inode(struct super_block *sb)
{
	struct tac3_inode_info *ci;

	ci = kmem_cache_alloc(tac3_inode_cachep, GFP_KERNEL);
	if (!ci)
		return NULL;
	memset(&ci->entry, 0, sizeof(ci->entry));
	ci->region = 0;
	return &ci->vfs_inode;
}

static void tac3_free_inode(struct inode *inode)
{
	kmem_cache_free(tac3_inode_cachep, TAC3_I(inode));
}

static int tac3_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct super_block *sb = dentry->d_sb;
	struct tac3_sb_info *sbi = TAC3_SB(sb);

	buf->f_type = TAC3_MAGIC;
	buf->f_bsize = PAGE_SIZE;
	buf->f_namelen = NAME_MAX;
	/* Redundancy factor is surfaced in fsid[0] for observability. */
	buf->f_fsid.val[0] = sbi->multitude;
	buf->f_fsid.val[1] = sbi->device_class;
	return 0;
}

static int tac3_show_options(struct seq_file *m, struct dentry *root)
{
	struct tac3_sb_info *sbi = TAC3_SB(root->d_sb);

	seq_printf(m, ",multitude=%u", sbi->multitude);
	seq_printf(m, ",device_class=%u", sbi->device_class);
	return 0;
}

/* Weak observability ref to the most-recently-mounted instance. */
static struct tac3_sb_info *tac3_last_sbi;
static DEFINE_SPINLOCK(tac3_last_lock);

static void tac3_put_super(struct super_block *sb)
{
	struct tac3_sb_info *sbi = TAC3_SB(sb);
	unsigned long flags;

	if (sbi) {
		spin_lock_irqsave(&tac3_last_lock, flags);
		if (tac3_last_sbi == sbi)
			tac3_last_sbi = NULL;
		spin_unlock_irqrestore(&tac3_last_lock, flags);
		kvfree(sbi->layers);
		kfree(sbi);
		sb->s_fs_info = NULL;
	}
}

const struct super_operations tac3_super_operations = {
	.alloc_inode	= tac3_alloc_inode,
	.free_inode	= tac3_free_inode,
	.statfs		= tac3_statfs,
	.drop_inode	= generic_delete_inode,
	.show_options	= tac3_show_options,
	.put_super	= tac3_put_super,
};
EXPORT_SYMBOL_GPL(tac3_super_operations);

/* ==========================================================================
 * mount options (multitude=, device_class=)
 * ========================================================================== */
enum tac3_param { Opt_multitude, Opt_device_class };

static const struct fs_parameter_spec tac3_param_specs[] = {
	fsparam_u32("multitude",    Opt_multitude),
	fsparam_u32("device_class", Opt_device_class),
	{}
};

struct tac3_mount_opts {
	u32 multitude;
	u32 device_class;
};

static int tac3_parse_param(struct fs_context *fc, struct fs_parameter *param)
{
	struct tac3_mount_opts *opts = fc->fs_private;
	struct fs_parse_result result;
	int opt = fs_parse(fc, tac3_param_specs, param, &result);

	if (opt < 0)
		return opt;

	switch (opt) {
	case Opt_multitude:
		if (result.uint_32 < TAC3_MULT_MIN ||
		    result.uint_32 > TAC3_MULT_MAX)
			return invalf(fc, "tac3: multitude out of range [%u,%u]",
				      TAC3_MULT_MIN, TAC3_MULT_MAX);
		opts->multitude = result.uint_32;
		break;
	case Opt_device_class:
		opts->device_class = result.uint_32;
		break;
	}
	return 0;
}

static int tac3_fill_super(struct super_block *sb, struct fs_context *fc)
{
	struct tac3_mount_opts *opts = fc->fs_private;
	struct tac3_sb_info *sbi;
	struct inode *root;
	u32 i;

	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;

	sbi->multitude = opts->multitude ? opts->multitude : default_multitude;
	if (sbi->multitude < TAC3_MULT_MIN) sbi->multitude = TAC3_MULT_MIN;
	if (sbi->multitude > TAC3_MULT_MAX) sbi->multitude = TAC3_MULT_MAX;
	sbi->device_class = opts->device_class ? opts->device_class
					       : default_device_class;
	spin_lock_init(&sbi->lock);
	sbi->next_ino = 1;

	sbi->layers = kvcalloc(TAC3_MULT_MAX, sizeof(*sbi->layers), GFP_KERNEL);
	if (!sbi->layers) {
		kfree(sbi);
		return -ENOMEM;
	}
	for (i = 0; i < TAC3_MULT_MAX; i++) {
		sbi->layers[i].layer_index = i;
		sbi->layers[i].disk_health = 1000;
		sbi->layers[i].state = TAC3_GREEN;
		sbi->layers[i].avg_quality = 1000;
	}

	sbi->admin.table_multitude = sbi->multitude;
	sbi->admin.monitor_health = TAC3_GREEN;
	sbi->admin.file_table_health = 1000;
	sbi->admin.created_unix = sbi->admin.updated_unix =
		ktime_get_real_seconds();

	sb->s_fs_info = sbi;
	sb->s_magic = TAC3_MAGIC;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_op = &tac3_super_operations;
	sb->s_time_gran = 1;

	root = tac3_get_inode(sb, NULL, S_IFDIR | 0755, 0);
	if (!root) {
		tac3_put_super(sb);
		return -ENOMEM;
	}
	sb->s_root = d_make_root(root);
	if (!sb->s_root) {
		tac3_put_super(sb);
		return -ENOMEM;
	}

	{
		unsigned long flags;
		spin_lock_irqsave(&tac3_last_lock, flags);
		tac3_last_sbi = sbi;
		spin_unlock_irqrestore(&tac3_last_lock, flags);
	}

	pr_info("tac3: mounted %u-way redundant file table; device ceiling %u MB/s\n",
		sbi->multitude, tac3_speed_ceiling(sbi->device_class));
	return 0;
}

static int tac3_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, tac3_fill_super);
}

static void tac3_free_fc(struct fs_context *fc)
{
	kfree(fc->fs_private);
}

static const struct fs_context_operations tac3_context_ops = {
	.parse_param	= tac3_parse_param,
	.get_tree	= tac3_get_tree,
	.free		= tac3_free_fc,
};

static int tac3_init_fs_context(struct fs_context *fc)
{
	struct tac3_mount_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return -ENOMEM;
	opts->multitude = default_multitude;
	opts->device_class = default_device_class;
	fc->fs_private = opts;
	fc->ops = &tac3_context_ops;
	return 0;
}

struct file_system_type tac3_fs_type = {
	.owner			= THIS_MODULE,
	.name			= TAC3_NAME,
	.init_fs_context	= tac3_init_fs_context,
	.parameters		= tac3_param_specs,
	.kill_sb		= kill_litter_super,
	.fs_flags		= FS_USERNS_MOUNT,
};
EXPORT_SYMBOL_GPL(tac3_fs_type);

/* ==========================================================================
 * Observability: /proc/tac3/* aggregated across mounted instances
 * ==========================================================================
 * These proc files describe the module and its control device. Per-mount
 * health is authoritative in each superblock; the control device (/dev/tac3)
 * lets an operator query/configure the most-recently-mounted instance.
 */
static int status_show(struct seq_file *m, void *v)
{
	struct tac3_sb_info *sbi;
	unsigned long flags;

	seq_puts(m, "TAC3 — Tripartite Addressable Cache (3-Table Edition)\n");
	seq_puts(m, "filesystem type          : tac3 (register_filesystem)\n");
	seq_puts(m, "I/O routing              : via tac3_file_operations only\n");

	spin_lock_irqsave(&tac3_last_lock, flags);
	sbi = tac3_last_sbi;
	if (!sbi) {
		spin_unlock_irqrestore(&tac3_last_lock, flags);
		seq_puts(m, "instance                 : (none mounted)\n");
		return 0;
	}
	seq_printf(m, "multitude (redundancy)   : %u layers\n", sbi->multitude);
	seq_printf(m, "device speed ceiling     : %u MB/s (class %u)\n",
		   tac3_speed_ceiling(sbi->device_class), sbi->device_class);
	seq_printf(m, "file-table health        : %u/1000 (%s)\n",
		   tac3_file_table_health(sbi),
		   state_str(tac3_state_of(tac3_file_table_health(sbi), 0)));
	seq_printf(m, "monitor health           : %s\n",
		   state_str(sbi->admin.monitor_health));
	seq_puts(m, "tables                   : 1=FILE  2=HEALTH  3=ADMIN\n");
	spin_unlock_irqrestore(&tac3_last_lock, flags);
	return 0;
}

static int health_show(struct seq_file *m, void *v)
{
	struct tac3_sb_info *sbi;
	unsigned long flags;
	u32 i;

	spin_lock_irqsave(&tac3_last_lock, flags);
	sbi = tac3_last_sbi;
	if (!sbi) {
		spin_unlock_irqrestore(&tac3_last_lock, flags);
		seq_puts(m, "(no tac3 instance mounted)\n");
		return 0;
	}
	seq_puts(m, "layer  state   reads      writes     read_heat  write_wear jarring   quality‰ pressure‰ health‰\n");
	for (i = 0; i < sbi->multitude; i++) {
		struct tac3_layer_health *L = &sbi->layers[i];
		seq_printf(m, "%-5u  %-6s  %-9llu  %-9llu  %-9llu  %-9llu  %-8llu  %-8u %-9u %u\n",
			   i, state_str(L->state), L->total_reads, L->total_writes,
			   L->total_read_heat, L->total_write_wear, L->total_jarring,
			   L->avg_quality, L->avg_pressure, L->disk_health);
	}
	spin_unlock_irqrestore(&tac3_last_lock, flags);
	return 0;
}

static int admin_show(struct seq_file *m, void *v)
{
	struct tac3_sb_info *sbi;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&tac3_last_lock, flags);
	sbi = tac3_last_sbi;
	if (!sbi) {
		spin_unlock_irqrestore(&tac3_last_lock, flags);
		seq_puts(m, "(no tac3 instance mounted)\n");
		return 0;
	}
	seq_puts(m, "TAC3 Table 3 — Administrative / State\n");
	seq_printf(m, "tech_id                  : %llu\n", sbi->admin.tech_id);
	seq_printf(m, "monitor_health           : %s\n",
		   state_str(sbi->admin.monitor_health));
	seq_printf(m, "table_multitude          : %u\n", sbi->admin.table_multitude);
	seq_printf(m, "file_table_health        : %u/1000\n",
		   tac3_file_table_health(sbi));
	seq_printf(m, "admin_table_revision     : %u\n", sbi->admin.admin_table_revision);
	seq_printf(m, "created_unix             : %llu\n", sbi->admin.created_unix);
	seq_printf(m, "updated_unix             : %llu\n", sbi->admin.updated_unix);
	seq_printf(m, "special_use_permit_mask  : 0x%08x\n",
		   sbi->admin.special_use_permit_mask);
	seq_puts(m, "operator reference slots (opaque; operator-defined, no kernel meaning):\n");
	for (i = 0; i < 8; i++)
		seq_printf(m, "  slot[%d] %-24s = %d\n", i,
			   sbi->admin.operator_ref_label[i][0]
				? sbi->admin.operator_ref_label[i] : "(unset)",
			   sbi->admin.operator_ref[i]);
	spin_unlock_irqrestore(&tac3_last_lock, flags);
	return 0;
}

static int status_open(struct inode *i, struct file *f){ return single_open(f, status_show, NULL);} 
static int health_open(struct inode *i, struct file *f){ return single_open(f, health_show, NULL);} 
static int admin_open(struct inode *i, struct file *f){ return single_open(f, admin_show, NULL);} 

static const struct proc_ops status_ops = { .proc_open=status_open, .proc_read=seq_read, .proc_lseek=seq_lseek, .proc_release=single_release };
static const struct proc_ops health_ops = { .proc_open=health_open, .proc_read=seq_read, .proc_lseek=seq_lseek, .proc_release=single_release };
static const struct proc_ops admin_ops  = { .proc_open=admin_open,  .proc_read=seq_read, .proc_lseek=seq_lseek, .proc_release=single_release };

/* ---- /dev/tac3 ioctl (config + admin set/get, targets last instance) ---- */
static long tac3_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	void __user *p = (void __user *)arg;
	struct tac3_config cfg;
	struct tac3_sb_info *sbi;
	unsigned long flags;

	spin_lock_irqsave(&tac3_last_lock, flags);
	sbi = tac3_last_sbi;
	spin_unlock_irqrestore(&tac3_last_lock, flags);
	if (!sbi)
		return -ENODEV;

	switch (cmd) {
	case TAC3_IOC_SET_CONFIG:
		if (copy_from_user(&cfg, p, sizeof(cfg))) return -EFAULT;
		if (cfg.multitude < TAC3_MULT_MIN || cfg.multitude > TAC3_MULT_MAX)
			return -EINVAL;
		spin_lock_irqsave(&sbi->lock, flags);
		sbi->multitude = cfg.multitude;
		sbi->admin.table_multitude = cfg.multitude;
		sbi->device_class = cfg.device_class;
		sbi->admin.updated_unix = ktime_get_real_seconds();
		spin_unlock_irqrestore(&sbi->lock, flags);
		return 0;
	case TAC3_IOC_GET_CONFIG:
		cfg.multitude = sbi->multitude; cfg.device_class = sbi->device_class;
		return copy_to_user(p, &cfg, sizeof(cfg)) ? -EFAULT : 0;
	case TAC3_IOC_SET_ADMIN:
		/* Operator sets Table 3 verbatim; kernel assigns no meaning. */
		spin_lock_irqsave(&sbi->lock, flags);
		if (copy_from_user(&sbi->admin, p, sizeof(sbi->admin))) {
			spin_unlock_irqrestore(&sbi->lock, flags);
			return -EFAULT;
		}
		sbi->admin.admin_table_revision++;
		sbi->admin.updated_unix = ktime_get_real_seconds();
		spin_unlock_irqrestore(&sbi->lock, flags);
		return 0;
	case TAC3_IOC_GET_ADMIN:
		return copy_to_user(p, &sbi->admin, sizeof(sbi->admin)) ? -EFAULT : 0;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations tac3_ctl_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tac3_ioctl,
};
static struct miscdevice tac3_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR, .name = "tac3", .fops = &tac3_ctl_fops,
};

static struct proc_dir_entry *tac3_proc;

/* ==========================================================================
 * module init / exit
 * ========================================================================== */
static void tac3_init_once(void *p)
{
	struct tac3_inode_info *ci = p;
	inode_init_once(&ci->vfs_inode);
}

static int __init tac3_init(void)
{
	int err;

	tac3_inode_cachep = kmem_cache_create("tac3_inode_cache",
			sizeof(struct tac3_inode_info), 0,
			SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD | SLAB_ACCOUNT,
			tac3_init_once);
	if (!tac3_inode_cachep)
		return -ENOMEM;

	err = register_filesystem(&tac3_fs_type);
	if (err) {
		kmem_cache_destroy(tac3_inode_cachep);
		return err;
	}

	tac3_proc = proc_mkdir("tac3", NULL);
	if (tac3_proc) {
		proc_create("status", 0444, tac3_proc, &status_ops);
		proc_create("health", 0444, tac3_proc, &health_ops);
		proc_create("admin",  0444, tac3_proc, &admin_ops);
	}
	misc_register(&tac3_ctl_dev);

	pr_info("tac3: registered filesystem '%s'; reads/writes route through tac3_file_operations\n",
		TAC3_NAME);
	return 0;
}

static void __exit tac3_exit(void)
{
	misc_deregister(&tac3_ctl_dev);
	if (tac3_proc) {
		remove_proc_entry("status", tac3_proc);
		remove_proc_entry("health", tac3_proc);
		remove_proc_entry("admin",  tac3_proc);
		remove_proc_entry("tac3", NULL);
	}
	unregister_filesystem(&tac3_fs_type);
	/*
	 * Ensure all delayed rcu free's are flushed before destroying cache.
	 */
	rcu_barrier();
	kmem_cache_destroy(tac3_inode_cachep);
	pr_info("tac3: unloaded\n");
}

module_init(tac3_init);
module_exit(tac3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maximilian Eric Alexander Rupplin von Keffikon / MEARVK LLC");
MODULE_DESCRIPTION("TAC3 — N-way redundant filesystem; reads/writes routed through its own VFS handles, with wear/pressure/health (Tables 1/2/3)");
MODULE_ALIAS_FS("tac3");
