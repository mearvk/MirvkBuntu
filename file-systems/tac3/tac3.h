/* SPDX-License-Identifier: GPL-2.0 */
/*
 * tac3.h — TAC3: Tripartite Addressable Cache, 3-Table Edition
 *
 * TAC3 is an N-way redundant filesystem whose reads and writes are serviced
 * ONLY through its own kernel-call handles (the VFS operation tables declared
 * below). Standard file read()/write()/mmap()/fsync() paths that land on a
 * TAC3-mounted filesystem are dispatched through tac3_file_operations,
 * tac3_inode_operations, tac3_aops and tac3_super_operations; there is no
 * side channel. Every serviced I/O feeds the wear/pressure/health engine.
 *
 * TAC3 maintains three coordinated tables over an N-layer ("multitude")
 * redundant file table:
 *
 *   Table 1 (FILE)   - file entries replicated across N layers.
 *   Table 2 (HEALTH) - per-layer/per-region read/write/press/wear + disk health.
 *   Table 3 (ADMIN)  - administrative/state properties (facts + opaque values).
 *
 * The engine derives a per-read QUALITY and PRESSURE from the device spec so
 * that read frequency accrues wear that is noted across the N layers.
 *
 * Copyright (C) 2026 MEARVK LLC
 * Author: Maximilian Eric Alexander Rupplin von Keffikon
 */
#ifndef __LINUX_TAC3_H
#define __LINUX_TAC3_H

#include <linux/types.h>
#include <linux/limits.h>
#include <linux/ioctl.h>
#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/spinlock.h>
#endif

/* On-disk / on-wire brand identifiers. */
#define TAC3_NAME              "tac3"
#define TAC3_MAGIC             0x54414333  /* "TAC3" */

/* ---- Multitude (redundancy factor) -------------------------------------- */
#define TAC3_MULT_MIN          1
#define TAC3_MULT_DEFAULT      10      /* "10 of file redundancy" */
#define TAC3_MULT_MAX          16      /* hard ceiling on layers */
/* Common configured multitudes: 1, 3, 5, 10 (any value in [MIN,MAX] allowed) */

#define TAC3_MAX_REGIONS       4096    /* wear-tracked disk regions per layer */
#define TAC3_IOCTL_MAGIC       'T'

/* ---- Device class + spec (mirrors pcopy device model) ------------------- */
enum tac3_device_class {
	TAC3_DEV_UNKNOWN = 0,
	TAC3_DEV_IDE_HDD, TAC3_DEV_SATA_HDD, TAC3_DEV_SAS_HDD,
	TAC3_DEV_SATA_SSD,
	TAC3_DEV_NVME_GEN3, TAC3_DEV_NVME_GEN4, TAC3_DEV_NVME_GEN5,
	TAC3_DEV_USB2, TAC3_DEV_USB3, TAC3_DEV_USB4,
};

/* Practical throughput ceilings (MB/s), used to scale quality/pressure. */
#define TAC3_SPEED_IDE_HDD     80
#define TAC3_SPEED_SATA_HDD    150
#define TAC3_SPEED_SAS_HDD     200
#define TAC3_SPEED_SATA_SSD    550
#define TAC3_SPEED_NVME_GEN3   3500
#define TAC3_SPEED_NVME_GEN4   7000
#define TAC3_SPEED_NVME_GEN5   14000
#define TAC3_SPEED_USB2        35
#define TAC3_SPEED_USB3        400
#define TAC3_SPEED_USB4        3000

/*
 * Health color model (from aptitude/health): green/white/yellow, no red-alarm.
 *   GREEN  - healthy / verified
 *   WHITE  - informational / normal
 *   YELLOW - attention recommended (not failure)
 */
enum tac3_health_state { TAC3_GREEN = 0, TAC3_WHITE, TAC3_YELLOW };

/* ===========================================================================
 * TABLE 2 — HEALTH / WEAR (about Table 1)
 * ===========================================================================
 * Per read, the engine computes:
 *   quality  = how well the read met the device spec (0..1000, fixed-point ‰)
 *   pressure = instantaneous load appealed to the device (0..1000 ‰)
 * Read-heat and write-wear accrue per region; heavy/jarring access adds extra
 * impact spread across the N layers.
 */
struct tac3_region_wear {
	u64 reads;             /* read touches */
	u64 writes;            /* write touches */
	u64 read_heat;         /* cumulative read-heat (grows with re-reads) */
	u64 write_wear;        /* cumulative write-wear */
	u64 jarring_events;    /* heavy/abrupt access recorded as extra impact */
	u32 last_quality;      /* last read quality, ‰ (0..1000) */
	u32 last_pressure;     /* last read pressure, ‰ (0..1000) */
	u32 peak_pressure;     /* peak pressure observed on this region */
};

struct tac3_layer_health {
	u32 layer_index;                 /* 0 .. multitude-1 */
	enum tac3_health_state state;    /* green/white/yellow */
	u64 total_reads;
	u64 total_writes;
	u64 total_read_heat;
	u64 total_write_wear;
	u64 total_jarring;               /* extra-impact events on this layer */
	u32 avg_quality;                 /* rolling avg read quality ‰ */
	u32 avg_pressure;                /* rolling avg pressure ‰ */
	u32 disk_health;                 /* 0..1000 ‰ derived health of the layer */
	u32 error_count;                 /* IO errors seen on this layer */
	struct tac3_region_wear region[TAC3_MAX_REGIONS];
};

/* ===========================================================================
 * TABLE 1 — FILE (n-way redundant)
 * ===========================================================================
 */
struct tac3_file_entry {
	u64 ino;                         /* logical entry id */
	u64 size;
	u32 layer_mask;                  /* which layers hold a good copy */
	u32 primary_layer;               /* current authoritative layer */
	u8  present[TAC3_MULT_MAX];      /* 1 if replica valid on layer i */
};

/* ===========================================================================
 * TABLE 3 — ADMIN / STATE
 * ===========================================================================
 * Administrative FACTS + OPERATOR-SUPPLIED OPAQUE reference values.
 *
 * The kernel does not compute, infer, or judge any person's intelligence,
 * worth, feelings, learning, friendships, or standing. The "operator reference"
 * fields below are stored verbatim exactly as the operator sets them and carry
 * NO kernel-assigned meaning. They exist as an opaque state record only.
 */
struct tac3_admin_state {
	/* --- objective administrative facts --- */
	u64 tech_id;                     /* technical/asset identifier */
	enum tac3_health_state monitor_health; /* health of the monitor itself */
	u32 table_multitude;             /* configured redundancy (1/3/5/10..) */
	u32 file_table_health;           /* 0..1000 ‰ health of Table 1 */
	u32 admin_table_revision;        /* schema/state revision */
	u64 created_unix;                /* creation timestamp */
	u64 updated_unix;                /* last update timestamp */
	u32 special_use_permit_mask;     /* bitmask of granted operational permits */

	/*
	 * --- operator-supplied OPAQUE reference values ---
	 * Verbatim numbers set by the operator via ioctl. No meaning is computed
	 * or asserted by the kernel. Named generically to avoid encoding any
	 * judgement about a person; see TAC3 ethics note in Kconfig.
	 */
	s32 operator_ref[8];             /* opaque operator-defined reference slots */
	char operator_ref_label[8][32];  /* operator's own labels for each slot */
};

/* ---- ioctl surface ------------------------------------------------------ */
struct tac3_config {
	u32 multitude;                   /* desired redundancy factor */
	u32 device_class;                /* enum tac3_device_class */
};

#define TAC3_IOC_SET_CONFIG  _IOW(TAC3_IOCTL_MAGIC, 1, struct tac3_config)
#define TAC3_IOC_GET_CONFIG  _IOR(TAC3_IOCTL_MAGIC, 2, struct tac3_config)
#define TAC3_IOC_SET_ADMIN   _IOW(TAC3_IOCTL_MAGIC, 3, struct tac3_admin_state)
#define TAC3_IOC_GET_ADMIN   _IOR(TAC3_IOCTL_MAGIC, 4, struct tac3_admin_state)

#ifdef __KERNEL__
/* ===========================================================================
 * VFS integration surface
 * ===========================================================================
 * These are the ONLY kernel-call handles through which a TAC3 filesystem is
 * addressed. register_filesystem(&tac3_fs_type) publishes the mount entry;
 * every dispatched read/write flows through tac3_file_operations and lands in
 * tac3_record_access() below.
 */

/* Per-superblock private state (one live TAC3 instance). */
struct tac3_sb_info {
	u32 multitude;                          /* active redundancy factor */
	u32 device_class;                       /* enum tac3_device_class */
	struct tac3_layer_health *layers;       /* [TAC3_MULT_MAX] Table 2 */
	struct tac3_admin_state admin;          /* Table 3 */
	spinlock_t lock;                        /* guards layers + admin */
	u64 next_ino;                           /* inode allocator */
};

/* Per-inode private state (Table 1 entry the inode maps to). */
struct tac3_inode_info {
	struct tac3_file_entry entry;           /* Table 1 record */
	u32 region;                             /* wear region this inode maps to */
	struct inode vfs_inode;                 /* embedded VFS inode (must be last) */
};

static inline struct tac3_inode_info *TAC3_I(struct inode *inode)
{
	return container_of(inode, struct tac3_inode_info, vfs_inode);
}

static inline struct tac3_sb_info *TAC3_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

/*
 * Record one access against (layer, region). op: 0=read, 1=write.
 * jarring != 0 marks heavy/abrupt access -> extra impact spread across layers.
 * observed_mbps is the measured throughput for a read (ignored for writes).
 * Operates on the given superblock's TAC3 instance.
 */
void tac3_record_access(struct tac3_sb_info *sbi, u32 layer, u32 region,
			int op, u32 observed_mbps, int jarring);

/* Published VFS operation tables (defined in tac3.c). */
extern const struct file_operations       tac3_file_operations;
extern const struct file_operations       tac3_dir_operations;
extern const struct inode_operations       tac3_file_inode_operations;
extern const struct inode_operations       tac3_dir_inode_operations;
extern const struct address_space_operations tac3_aops;
extern const struct super_operations       tac3_super_operations;
extern struct file_system_type             tac3_fs_type;
#endif /* __KERNEL__ */

#endif /* __LINUX_TAC3_H */
