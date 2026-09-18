// SPDX-License-Identifier: GPL-2.0
/*
 * tac3_persistent.c — TAC3 persistent-format mount layer.
 *
 * Phase 1 of persistent TAC3 mounting validates and consumes the on-disk
 * superblock created by mkfs.tac3, then exposes a read-only TAC3 mount.
 * File payload persistence remains a later phase; this fail-closed boundary
 * prevents the older memory-backed data path from pretending to be durable.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/fs_context.h>
#include <linux/fs_parser.h>
#include <linux/uuid.h>
#include "tac3.h"
#include "tac3_format.h"

static u32 default_multitude = TAC3_MULT_DEFAULT;
module_param(default_multitude, uint, 0644);
MODULE_PARM_DESC(default_multitude, "Default TAC3 redundancy factor");

static u32 default_device_class = TAC3_DEV_NVME_GEN4;
module_param(default_device_class, uint, 0644);
MODULE_PARM_DESC(default_device_class, "Default TAC3 device class");

struct tac3_disk_state {
	struct tac3_sb_info core;
	u8 uuid[16];
	u64 generation;
	u64 total_blocks;
	u64 file_table_start;
	u64 file_table_blocks;
	u64 health_table_start;
	u64 health_table_blocks;
	u64 admin_table_start;
	u64 admin_table_blocks;
	u64 recovery_start;
	u64 recovery_blocks;
	u64 data_start;
	u32 disk_state;
};

static inline struct tac3_disk_state *TAC3_DISK(struct super_block *sb)
{
	return container_of(TAC3_SB(sb), struct tac3_disk_state, core);
}

static u32 tac3_speed_ceiling(u32 cls)
{
	switch (cls) {
	case TAC3_DEV_IDE_HDD: return TAC3_SPEED_IDE_HDD;
	case TAC3_DEV_SATA_HDD: return TAC3_SPEED_SATA_HDD;
	case TAC3_DEV_SAS_HDD: return TAC3_SPEED_SAS_HDD;
	case TAC3_DEV_SATA_SSD: return TAC3_SPEED_SATA_SSD;
	case TAC3_DEV_NVME_GEN3: return TAC3_SPEED_NVME_GEN3;
	case TAC3_DEV_NVME_GEN4: return TAC3_SPEED_NVME_GEN4;
	case TAC3_DEV_NVME_GEN5: return TAC3_SPEED_NVME_GEN5;
	case TAC3_DEV_USB2: return TAC3_SPEED_USB2;
	case TAC3_DEV_USB3: return TAC3_SPEED_USB3;
	case TAC3_DEV_USB4: return TAC3_SPEED_USB4;
	default: return TAC3_SPEED_SATA_SSD;
	}
}

static u32 tac3_quality(u32 cls, u32 observed)
{
	u32 ceiling = tac3_speed_ceiling(cls);
	u64 q;
	if (!ceiling)
		return 0;
	q = (u64)observed * 1000ULL;
	do_div(q, ceiling);
	return q > 1000 ? 1000 : (u32)q;
}

static u32 tac3_pressure(u64 heat)
{
	u32 p = 0;
	while (heat && p < 1000) {
		p += 50;
		heat >>= 1;
	}
	return p;
}

static enum tac3_health_state tac3_state_of(u32 health, u32 errors)
{
	if (errors || health < 600)
		return TAC3_YELLOW;
	if (health < 850)
		return TAC3_WHITE;
	return TAC3_GREEN;
}

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
	if (op == 0) {
		R->reads++;
		L->total_reads++;
		R->read_heat++;
		L->total_read_heat++;
		quality = tac3_quality(sbi->device_class, observed_mbps);
		pressure = tac3_pressure(R->read_heat);
		R->last_quality = quality;
		R->last_pressure = pressure;
		if (pressure > R->peak_pressure)
			R->peak_pressure = pressure;
		L->avg_quality += (quality - L->avg_quality) >> 3;
		L->avg_pressure += (pressure - L->avg_pressure) >> 3;
	} else {
		R->writes++;
		L->total_writes++;
		R->write_wear++;
		L->total_write_wear++;
	}
	if (jarring) {
		R->jarring_events++;
		L->total_jarring++;
		for (i = 0; i < sbi->multitude; i++)
			sbi->layers[i].total_jarring++;
	}
	{
		u64 penalty = (L->total_write_wear >> 6) +
			(L->total_jarring >> 2) + (u64)L->error_count * 20ULL;
		L->disk_health = penalty >= 1000 ? 0 : (u32)(1000 - penalty);
		L->state = tac3_state_of(L->disk_health, L->error_count);
	}
	sbi->admin.updated_unix = ktime_get_real_seconds();
	spin_unlock_irqrestore(&sbi->lock, flags);
}
EXPORT_SYMBOL_GPL(tac3_record_access);

static u32 tac3_crc32c(const u8 *data, size_t len)
{
	u32 crc = 0xffffffffU;
	size_t i;
	for (i = 0; i < len; i++) {
		int bit;
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0x82f63b78U & -(crc & 1U));
	}
	return ~crc;
}

static u16 tac3_get16(const u8 *p)
{
	return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 tac3_get32(const u8 *p)
{
	return (u32)p[0] | ((u32)p[1] << 8) |
		((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static u64 tac3_get64(const u8 *p)
{
	return (u64)tac3_get32(p) | ((u64)tac3_get32(p + 4) << 32);
}

static int tac3_validate_superblock(struct super_block *sb,
				    struct buffer_head *bh,
				    struct tac3_disk_state *disk)
{
	const u8 *p = (const u8 *)bh->b_data;
	u8 saved[4];
	u32 expected, actual;
	u64 end;

	if (memcmp(p + TAC3_SB_OFF_MAGIC, TAC3_DISK_MAGIC,
		   TAC3_DISK_MAGIC_SIZE))
		return -EINVAL;
	if (tac3_get16(p + TAC3_SB_OFF_MAJOR) != TAC3_DISK_FORMAT_MAJOR)
		return -EPROTONOSUPPORT;
	if (tac3_get16(p + TAC3_SB_OFF_MINOR) > TAC3_DISK_FORMAT_MINOR)
		return -EPROTONOSUPPORT;
	if (tac3_get32(p + TAC3_SB_OFF_BLOCK_SIZE) != TAC3_DISK_BLOCK_SIZE)
		return -EINVAL;
	if (tac3_get32(p + TAC3_SB_OFF_CHECKSUM_ALGORITHM) !=
	    TAC3_DISK_CHECKSUM_CRC32C)
		return -EOPNOTSUPP;

	memcpy(saved, p + TAC3_SB_OFF_CHECKSUM, sizeof(saved));
	memset((u8 *)p + TAC3_SB_OFF_CHECKSUM, 0, sizeof(saved));
	actual = tac3_crc32c(p, TAC3_DISK_OFF_CHECKSUM);
	memcpy((u8 *)p + TAC3_SB_OFF_CHECKSUM, saved, sizeof(saved));
	expected = tac3_get32(saved);
	if (actual != expected)
		return -EUCLEAN;

	disk->total_blocks = tac3_get64(p + TAC3_SB_OFF_TOTAL_BLOCKS);
	if (disk->total_blocks < TAC3_DISK_MIN_BLOCKS ||
	    disk->total_blocks != div_u64(i_size_read(sb->s_bdev->bd_inode),
					   TAC3_DISK_BLOCK_SIZE))
		return -EINVAL;

	disk->generation = tac3_get64(p + TAC3_SB_OFF_GENERATION);
	disk->core.multitude = tac3_get32(p + TAC3_SB_OFF_MULTITUDE);
	disk->core.device_class = tac3_get32(p + TAC3_SB_OFF_DEVICE_CLASS);
	if (disk->core.multitude < TAC3_MULT_MIN ||
	    disk->core.multitude > TAC3_MULT_MAX)
		return -EINVAL;
	if (disk->core.device_class > TAC3_DEV_USB4)
		return -EINVAL;

	memcpy(disk->uuid, p + TAC3_SB_OFF_UUID, sizeof(disk->uuid));
	disk->file_table_start = tac3_get64(p + TAC3_SB_OFF_FILE_START);
	disk->file_table_blocks = tac3_get64(p + TAC3_SB_OFF_FILE_BLOCKS);
	disk->health_table_start = tac3_get64(p + TAC3_SB_OFF_HEALTH_START);
	disk->health_table_blocks = tac3_get64(p + TAC3_SB_OFF_HEALTH_BLOCKS);
	disk->admin_table_start = tac3_get64(p + TAC3_SB_OFF_ADMIN_START);
	disk->admin_table_blocks = tac3_get64(p + TAC3_SB_OFF_ADMIN_BLOCKS);
	disk->recovery_start = tac3_get64(p + TAC3_SB_OFF_RECOVERY_START);
	disk->recovery_blocks = tac3_get64(p + TAC3_SB_OFF_RECOVERY_BLOCKS);
	disk->data_start = tac3_get64(p + TAC3_SB_OFF_DATA_START);
	disk->disk_state = tac3_get32(p + TAC3_SB_OFF_STATE);

	if (disk->disk_state > TAC3_DISK_STATE_RECOVERY_REQUIRED)
		return -EINVAL;
	if (!disk->file_table_blocks || !disk->health_table_blocks ||
	    !disk->admin_table_blocks || !disk->recovery_blocks ||
	    disk->data_start >= disk->total_blocks)
		return -EINVAL;

	end = disk->file_table_start + disk->file_table_blocks;
	if (end > disk->total_blocks) return -EINVAL;
	end = disk->health_table_start + disk->health_table_blocks;
	if (end > disk->total_blocks) return -EINVAL;
	end = disk->admin_table_start + disk->admin_table_blocks;
	if (end > disk->total_blocks) return -EINVAL;
	end = disk->recovery_start + disk->recovery_blocks;
	if (end > disk->total_blocks) return -EINVAL;

	return 0;
}

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
	if (!inode) return NULL;
	ci = TAC3_I(inode);
	inode->i_ino = get_next_ino();
	inode_init_owner(&init_user_ns, inode, dir, mode);
	inode->i_mapping->a_ops = &tac3_aops;
	mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	spin_lock_irqsave(&sbi->lock, flags);
	ci->entry.ino = inode->i_ino;
	ci->entry.primary_layer = sbi->next_ino % sbi->multitude;
	ci->entry.layer_mask = (1U << sbi->multitude) - 1U;
	ci->region = sbi->next_ino % TAC3_MAX_REGIONS;
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
		inc_nlink(inode);
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

static int tac3_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	ssize_t ret = generic_file_read_iter(iocb, to);
	if (ret > 0)
		tac3_record_access(TAC3_SB(inode->i_sb),
			TAC3_I(inode)->entry.primary_layer,
			TAC3_I(inode)->region, 0, 0, 0);
	return ret;
}

static ssize_t tac3_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	/* Persistent data I/O is intentionally not enabled in phase 1. */
	return -EROFS;
}

const struct file_operations tac3_file_operations = {
	.owner = THIS_MODULE,
	.llseek = generic_file_llseek,
	.read_iter = tac3_read_iter,
	.write_iter = tac3_write_iter,
	.mmap = generic_file_mmap,
	.open = generic_file_open,
	.fsync = noop_fsync,
	.splice_read = generic_file_splice_read,
};
EXPORT_SYMBOL_GPL(tac3_file_operations);

const struct file_operations tac3_dir_operations = {
	.owner = THIS_MODULE,
	.llseek = generic_file_llseek,
	.read = generic_read_dir,
	.iterate_shared = dcache_readdir,
	.fsync = noop_fsync,
};
EXPORT_SYMBOL_GPL(tac3_dir_operations);

static int tac3_mknod(struct user_namespace *ns, struct inode *dir,
			      struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode *inode = tac3_get_inode(dir->i_sb, dir, mode, dev);
	if (!inode) return -ENOSPC;
	d_instantiate(dentry, inode);
	dget(dentry);
	dir->i_mtime = dir->i_ctime = current_time(dir);
	return 0;
}

static int tac3_create(struct user_namespace *ns, struct inode *dir,
			       struct dentry *dentry, umode_t mode, bool excl)
{
	return tac3_mknod(ns, dir, dentry, mode | S_IFREG, 0);
}

static int tac3_mkdir(struct user_namespace *ns, struct inode *dir,
			      struct dentry *dentry, umode_t mode)
{
	int ret = tac3_mknod(ns, dir, dentry, mode | S_IFDIR, 0);
	if (!ret) inc_nlink(dir);
	return ret;
}

static int tac3_symlink(struct user_namespace *ns, struct inode *dir,
			struct dentry *dentry, const char *symname)
{
	struct inode *inode = tac3_get_inode(dir->i_sb, dir, S_IFLNK | 0777, 0);
	int ret;
	if (!inode) return -ENOSPC;
	ret = page_symlink(inode, symname, strlen(symname) + 1);
	if (ret) { iput(inode); return ret; }
	d_instantiate(dentry, inode);
	dget(dentry);
	return 0;
}

const struct inode_operations tac3_file_inode_operations = {
	.setattr = simple_setattr,
	.getattr = simple_getattr,
};
EXPORT_SYMBOL_GPL(tac3_file_inode_operations);

const struct inode_operations tac3_dir_inode_operations = {
	.create = tac3_create,
	.lookup = simple_lookup,
	.link = simple_link,
	.unlink = simple_unlink,
	.symlink = tac3_symlink,
	.mkdir = tac3_mkdir,
	.rmdir = simple_rmdir,
	.mknod = tac3_mknod,
	.rename = simple_rename,
	.setattr = simple_setattr,
	.getattr = simple_getattr,
};
EXPORT_SYMBOL_GPL(tac3_dir_inode_operations);

static struct kmem_cache *tac3_inode_cachep;

static struct inode *tac3_alloc_inode(struct super_block *sb)
{
	struct tac3_inode_info *ci = kmem_cache_alloc(tac3_inode_cachep, GFP_KERNEL);
	if (!ci) return NULL;
	memset(&ci->entry, 0, sizeof(ci->entry));
	ci->region = 0;
	return &ci->vfs_inode;
}

static void tac3_free_inode(struct inode *inode)
{
	kmem_cache_free(tac3_inode_cachep, TAC3_I(inode));
}

static void tac3_put_super(struct super_block *sb)
{
	struct tac3_sb_info *sbi = TAC3_SB(sb);
	if (sbi) {
		kvfree(sbi->layers);
		kfree(TAC3_DISK(sb));
		sb->s_fs_info = NULL;
	}
}

const struct super_operations tac3_super_operations = {
	.alloc_inode = tac3_alloc_inode,
	.free_inode = tac3_free_inode,
	.drop_inode = generic_delete_inode,
	.put_super = tac3_put_super,
};
EXPORT_SYMBOL_GPL(tac3_super_operations);

enum tac3_param { Opt_multitude, Opt_device_class };
static const struct fs_parameter_spec tac3_param_specs[] = {
	fsparam_u32("multitude", Opt_multitude),
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
	if (opt < 0) return opt;
	switch (opt) {
	case Opt_multitude:
		if (result.uint_32 < TAC3_MULT_MIN || result.uint_32 > TAC3_MULT_MAX)
			return invalf(fc, "tac3: multitude out of range");
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
	struct buffer_head *bh;
	struct tac3_disk_state *disk;
	struct inode *root;
	u32 i;
	int ret;

	if (!sb_set_blocksize(sb, TAC3_DISK_BLOCK_SIZE))
		return -EINVAL;
	bh = sb_bread(sb, 0);
	if (!bh)
		return -EIO;

	disk = kzalloc(sizeof(*disk), GFP_KERNEL);
	if (!disk) { brelse(bh); return -ENOMEM; }
	spin_lock_init(&disk->core.lock);
	disk->core.next_ino = 1;
	ret = tac3_validate_superblock(sb, bh, disk);
	brelse(bh);
	if (ret) {
		kfree(disk);
		return ret;
	}

	if (opts->multitude && opts->multitude != disk->core.multitude) {
		kfree(disk);
		return -EINVAL;
	}
	if (opts->device_class && opts->device_class != disk->core.device_class) {
		kfree(disk);
		return -EINVAL;
	}

	disk->core.layers = kvcalloc(TAC3_MULT_MAX, sizeof(*disk->core.layers), GFP_KERNEL);
	if (!disk->core.layers) { kfree(disk); return -ENOMEM; }
	for (i = 0; i < TAC3_MULT_MAX; i++) {
		disk->core.layers[i].layer_index = i;
		disk->core.layers[i].disk_health = 1000;
		disk->core.layers[i].state = TAC3_GREEN;
		disk->core.layers[i].avg_quality = 1000;
	}
	disk->core.admin.table_multitude = disk->core.multitude;
	disk->core.admin.monitor_health =
		disk->disk_state == TAC3_DISK_STATE_RECOVERY_REQUIRED ? TAC3_YELLOW : TAC3_GREEN;
	disk->core.admin.file_table_health = 1000;
	disk->core.admin.created_unix = disk->core.admin.updated_unix =
		ktime_get_real_seconds();

	sb->s_fs_info = &disk->core;
	sb->s_magic = TAC3_MAGIC;
	sb->s_blocksize = TAC3_DISK_BLOCK_SIZE;
	sb->s_blocksize_bits = 12;
	sb->s_maxbytes = MAX_LFS_FILESIZE;
	sb->s_op = &tac3_super_operations;
	sb->s_time_gran = 1;
	/* Phase 1 is deliberately read-only until durable FILE data exists. */
	sb->s_flags |= SB_RDONLY;

	root = tac3_get_inode(sb, NULL, S_IFDIR | 0555, 0);
	if (!root) { tac3_put_super(sb); return -ENOMEM; }
	sb->s_root = d_make_root(root);
	if (!sb->s_root) { tac3_put_super(sb); return -ENOMEM; }

	pr_info("tac3: persistent format %u.%u mounted read-only; generation %llu; %u-way; device class %u\n",
		TAC3_DISK_FORMAT_MAJOR, TAC3_DISK_FORMAT_MINOR,
		(unsigned long long)disk->generation, disk->core.multitude,
		disk->core.device_class);
	return 0;
}

static int tac3_get_tree(struct fs_context *fc)
{
	return get_tree_bdev(fc, tac3_fill_super);
}

static void tac3_free_fc(struct fs_context *fc)
{
	kfree(fc->fs_private);
}

static const struct fs_context_operations tac3_context_ops = {
	.parse_param = tac3_parse_param,
	.get_tree = tac3_get_tree,
	.free = tac3_free_fc,
};

static int tac3_init_fs_context(struct fs_context *fc)
{
	struct tac3_mount_opts *opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts) return -ENOMEM;
	opts->multitude = default_multitude;
	opts->device_class = default_device_class;
	fc->fs_private = opts;
	fc->ops = &tac3_context_ops;
	return 0;
}

struct file_system_type tac3_fs_type = {
	.owner = THIS_MODULE,
	.name = TAC3_NAME,
	.init_fs_context = tac3_init_fs_context,
	.parameters = tac3_param_specs,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};
EXPORT_SYMBOL_GPL(tac3_fs_type);

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
		SLAB_RECLAIM_ACCOUNT | SLAB_ACCOUNT, tac3_init_once);
	if (!tac3_inode_cachep)
		return -ENOMEM;
	err = register_filesystem(&tac3_fs_type);
	if (err) {
		kmem_cache_destroy(tac3_inode_cachep);
		return err;
	}
	pr_info("tac3: persistent on-disk mount layer registered\n");
	return 0;
}

static void __exit tac3_exit(void)
{
	unregister_filesystem(&tac3_fs_type);
	rcu_barrier();
	kmem_cache_destroy(tac3_inode_cachep);
	pr_info("tac3: unloaded\n");
}

module_init(tac3_init);
module_exit(tac3_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maximilian Eric Alexander Rupplin von Keffikon / MEARVK LLC");
MODULE_DESCRIPTION("TAC3 persistent on-disk format mount layer");
MODULE_ALIAS_FS("tac3");
