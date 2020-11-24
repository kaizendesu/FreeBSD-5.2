# Walthrough of FreeBSD Unix File System (UFS) API

## Overview

1. _ufsread.c:_ Structures, Data Types, and Definitions
2. _ufsread.c_: _fsread_

## _ufsread.c:_ Structures, Data Types, and Definitions

### _dmadat_ Buffer Structure
```c
#define VBLKSHIFT	12
#define VBLKSIZE	(1 << VBLKSHIFT)

/* Buffers that must not span a 64k boundary. */
struct dmadat {
	char blkbuf[VBLKSIZE];	/* filesystem blocks */
	char indbuf[VBLKSIZE];	/* indir blocks */
	char sbbuf[SBLOCKSIZE];	/* superblock */
	char secbuf[DEV_BSIZE];	/* for MBR/disklabel */
};
static struct dmadat *dmadat;
```

### Disk i-node Structures

```c
/*
 * A UFS1 dinode contains all the meta-data associated with a UFS1 file.
 * This structure defines the on-disk format of a UFS1 dinode. Since
 * this structure describes an on-disk structure, all its fields
 * are defined by types with precise widths.
 */
struct ufs1_dinode {
	u_int16_t	di_mode;	/*   0: IFMT, permissions; see below. */
	int16_t		di_nlink;	/*   2: File link count. */
	union {
		u_int16_t oldids[2];	/*   4: Ffs: old user and group ids. */
	} di_u;
	u_int64_t	di_size;			/*   8: File byte count. */
	int32_t		di_atime;			/*  16: Last access time. */
	int32_t		di_atimensec;		/*  20: Last access time. */
	int32_t		di_mtime;			/*  24: Last modified time. */
	int32_t		di_mtimensec;		/*  28: Last modified time. */
	int32_t		di_ctime;			/*  32: Last inode change time. */
	int32_t		di_ctimensec;		/*  36: Last inode change time. */
	ufs1_daddr_t	di_db[NDADDR];	/*  40: Direct disk blocks. */
	ufs1_daddr_t	di_ib[NIADDR];	/*  88: Indirect disk blocks. */
	u_int32_t	di_flags;			/* 100: Status flags (chflags). */
	int32_t		di_blocks;			/* 104: Blocks actually held. */
	int32_t		di_gen;				/* 108: Generation number. */
	u_int32_t	di_uid;				/* 112: File owner. */
	u_int32_t	di_gid;				/* 116: File group. */
	int32_t		di_spare[2];		/* 120: Reserved; currently unused */
};

struct ufs2_dinode {
	u_int16_t	di_mode;		/*   0: IFMT, permissions; see below. */
	int16_t		di_nlink;		/*   2: File link count. */
	u_int32_t	di_uid;			/*   4: File owner. */
	u_int32_t	di_gid;			/*   8: File group. */
	u_int32_t	di_blksize;		/*  12: Inode blocksize. */
	u_int64_t	di_size;		/*  16: File byte count. */
	u_int64_t	di_blocks;		/*  24: Bytes actually held. */
	ufs_time_t	di_atime;		/*  32: Last access time. */
	ufs_time_t	di_mtime;		/*  40: Last modified time. */
	ufs_time_t	di_ctime;		/*  48: Last inode change time. */
	ufs_time_t	di_birthtime;	/*  56: Inode creation time. */
	int32_t		di_mtimensec;	/*  64: Last modified time. */
	int32_t		di_atimensec;	/*  68: Last access time. */
	int32_t		di_ctimensec;	/*  72: Last inode change time. */
	int32_t		di_birthnsec;	/*  76: Inode creation time. */
	int32_t		di_gen;			/*  80: Generation number. */
	u_int32_t	di_kernflags;	/*  84: Kernel flags. */
	u_int32_t	di_flags;		/*  88: Status flags (chflags). */
	int32_t		di_extsize;		/*  92: External attributes block. */
	ufs2_daddr_t	di_extb[NXADDR];/*  96: External attributes block. */
	ufs2_daddr_t	di_db[NDADDR];	/* 112: Direct disk blocks. */
	ufs2_daddr_t	di_ib[NIADDR];	/* 208: Indirect disk blocks. */
	int64_t		di_spare[3];		/* 232: Reserved; currently unused */
};
```

### _fs_ Structure

```c
/*
 * Super block for an FFS filesystem.
 */
struct fs {
	int32_t	 fs_firstfield;		/* historic filesystem linked list, */
	int32_t	 fs_unused_1;		/*     used for incore super blocks */
	int32_t	 fs_sblkno;		/* offset of super-block in filesys */
	int32_t	 fs_cblkno;		/* offset of cyl-block in filesys */
	int32_t	 fs_iblkno;		/* offset of inode-blocks in filesys */
	int32_t	 fs_dblkno;		/* offset of first data after cg */
	int32_t	 fs_old_cgoffset;	/* cylinder group offset in cylinder */
	int32_t	 fs_old_cgmask;		/* used to calc mod fs_ntrak */
	int32_t  fs_old_time;		/* last time written */
	int32_t	 fs_old_size;		/* number of blocks in fs */
	int32_t	 fs_old_dsize;		/* number of data blocks in fs */
	int32_t	 fs_ncg;		/* number of cylinder groups */
	int32_t	 fs_bsize;		/* size of basic blocks in fs */
	int32_t	 fs_fsize;		/* size of frag blocks in fs */
	int32_t	 fs_frag;		/* number of frags in a block in fs */
/* these are configuration parameters */
	int32_t	 fs_minfree;		/* minimum percentage of free blocks */
	int32_t	 fs_old_rotdelay;	/* num of ms for optimal next block */
	int32_t	 fs_old_rps;		/* disk revolutions per second */
/* these fields can be computed from the others */
	int32_t	 fs_bmask;		/* ``blkoff'' calc of blk offsets */
	int32_t	 fs_fmask;		/* ``fragoff'' calc of frag offsets */
	int32_t	 fs_bshift;		/* ``lblkno'' calc of logical blkno */
	int32_t	 fs_fshift;		/* ``numfrags'' calc number of frags */
/* these are configuration parameters */
	int32_t	 fs_maxcontig;		/* max number of contiguous blks */
	int32_t	 fs_maxbpg;		/* max number of blks per cyl group */
/* these fields can be computed from the others */
	int32_t	 fs_fragshift;		/* block to frag shift */
	int32_t	 fs_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
	int32_t	 fs_sbsize;		/* actual size of super block */
	int32_t	 fs_spare1[2];		/* old fs_csmask */
					/* old fs_csshift */
	int32_t	 fs_nindir;		/* value of NINDIR */
	int32_t	 fs_inopb;		/* value of INOPB */
	int32_t	 fs_old_nspf;		/* value of NSPF */
/* yet another configuration parameter */
	int32_t	 fs_optim;		/* optimization preference, see below */
	int32_t	 fs_old_npsect;		/* # sectors/track including spares */
	int32_t	 fs_old_interleave;	/* hardware sector interleave */
	int32_t	 fs_old_trackskew;	/* sector 0 skew, per track */
	int32_t	 fs_id[2];		/* unique filesystem id */
/* sizes determined by number of cylinder groups and their sizes */
	int32_t	 fs_old_csaddr;		/* blk addr of cyl grp summary area */
	int32_t	 fs_cssize;		/* size of cyl grp summary area */
	int32_t	 fs_cgsize;		/* cylinder group size */
	int32_t	 fs_spare2;		/* old fs_ntrak */
	int32_t	 fs_old_nsect;		/* sectors per track */
	int32_t  fs_old_spc;		/* sectors per cylinder */
	int32_t	 fs_old_ncyl;		/* cylinders in filesystem */
	int32_t	 fs_old_cpg;		/* cylinders per group */
	int32_t	 fs_ipg;		/* inodes per group */
	int32_t	 fs_fpg;		/* blocks per group * fs_frag */
/* this data must be re-computed after crashes */
	struct	csum fs_old_cstotal;	/* cylinder summary information */
/* these fields are cleared at mount time */
	int8_t   fs_fmod;		/* super block modified flag */
	int8_t   fs_clean;		/* filesystem is clean flag */
	int8_t 	 fs_ronly;		/* mounted read-only flag */
	int8_t   fs_old_flags;		/* old FS_ flags */
	u_char	 fs_fsmnt[MAXMNTLEN];	/* name mounted on */
	u_char	 fs_volname[MAXVOLLEN];	/* volume name */
	u_int64_t fs_swuid;		/* system-wide uid */
	int32_t  fs_pad;		/* due to alignment of fs_swuid */
/* these fields retain the current block allocation info */
	int32_t	 fs_cgrotor;		/* last cg searched */
	void 	*fs_ocsp[NOCSPTRS];	/* padding; was list of fs_cs buffers */
	u_int8_t *fs_contigdirs;	/* # of contiguously allocated dirs */
	struct	csum *fs_csp;		/* cg summary info buffer for fs_cs */
	int32_t	*fs_maxcluster;		/* max cluster in each cyl group */
	u_int	*fs_active;		/* used by snapshots to track fs */
	int32_t	 fs_old_cpc;		/* cyl per cycle in postbl */
	int32_t	 fs_maxbsize;		/* maximum blocking factor permitted */
	int64_t	 fs_sparecon64[17];	/* old rotation block list head */
	int64_t	 fs_sblockloc;		/* byte offset of standard superblock */
	struct	csum_total fs_cstotal;	/* cylinder summary information */
	ufs_time_t fs_time;		/* last time written */
	int64_t	 fs_size;		/* number of blocks in fs */
	int64_t	 fs_dsize;		/* number of data blocks in fs */
	ufs2_daddr_t fs_csaddr;		/* blk addr of cyl grp summary area */
	int64_t	 fs_pendingblocks;	/* blocks in process of being freed */
	int32_t	 fs_pendinginodes;	/* inodes in process of being freed */
	int32_t	 fs_snapinum[FSMAXSNAP];/* list of snapshot inode numbers */
	int32_t	 fs_avgfilesize;	/* expected average file size */
	int32_t	 fs_avgfpdir;		/* expected # of files per directory */
	int32_t	 fs_save_cgsize;	/* save real cg size to use fs_bsize */
	int32_t	 fs_sparecon32[26];	/* reserved for future constants */
	int32_t  fs_flags;		/* see FS_ flags below */
	int32_t	 fs_contigsumsize;	/* size of cluster summary array */ 
	int32_t	 fs_maxsymlinklen;	/* max length of an internal symlink */
	int32_t	 fs_old_inodefmt;	/* format of on-disk inodes */
	u_int64_t fs_maxfilesize;	/* maximum representable file size */
	int64_t	 fs_qbmask;		/* ~fs_bmask for use with 64-bit size */
	int64_t	 fs_qfmask;		/* ~fs_fmask for use with 64-bit size */
	int32_t	 fs_state;		/* validate fs_clean field */
	int32_t	 fs_old_postblformat;	/* format of positional layout tables */
	int32_t	 fs_old_nrpos;		/* number of rotational positions */
	int32_t	 fs_spare5[2];		/* old fs_postbloff */
					/* old fs_rotbloff */
	int32_t	 fs_magic;		/* magic number */
};
```

### _fs.h_ Definitions

```c
/*
 * Each disk drive contains some number of filesystems.
 * A filesystem consists of a number of cylinder groups.
 * Each cylinder group has inodes and data.
 *
 * A filesystem is described by its super-block, which in turn
 * describes the cylinder groups.  The super-block is critical
 * data and is replicated in each cylinder group to protect against
 * catastrophic loss.  This is done at `newfs' time and the critical
 * super-block data does not change, so the copies need not be
 * referenced further unless disaster strikes.
 *
 * For filesystem fs, the offsets of the various blocks of interest
 * are given in the super block as:
 *	[fs->fs_sblkno]		Super-block
 *	[fs->fs_cblkno]		Cylinder group block
 *	[fs->fs_iblkno]		Inode blocks
 *	[fs->fs_dblkno]		Data blocks
 * The beginning of cylinder group cg in fs, is given by
 * the ``cgbase(fs, cg)'' macro.
 *
 * Depending on the architecture and the media, the superblock may
 * reside in any one of four places. For tiny media where every block 
 * counts, it is placed at the very front of the partition. Historically,
 * UFS1 placed it 8K from the front to leave room for the disk label and
 * a small bootstrap. For UFS2 it got moved to 64K from the front to leave
 * room for the disk label and a bigger bootstrap, and for really piggy
 * systems we check at 256K from the front if the first three fail. In
 * all cases the size of the superblock will be SBLOCKSIZE. All values are
 * given in byte-offset form, so they do not imply a sector size. The
 * SBLOCKSEARCH specifies the order in which the locations should be searched.
 */
#define SBLOCK_FLOPPY	0
#define SBLOCK_UFS1		8192
#define SBLOCK_UFS2		65536
#define SBLOCK_PIGGY	262144
#define SBLOCKSIZE		8192
#define SBLOCKSEARCH \
	{ SBLOCK_UFS2, SBLOCK_UFS1, SBLOCK_FLOPPY, SBLOCK_PIGGY, -1 }
```

## _ufsread.c_: _fsread_

```c
/*
 * Possible superblock locations ordered from most to least likely.
 */
static int sblock_try[] = SBLOCKSEARCH;

static ssize_t
fsread(ino_t inode, void *buf, size_t nbyte)
{
#ifndef UFS2_ONLY
	static struct ufs1_dinode dp1;
#endif
#ifndef UFS1_ONLY
	static struct ufs2_dinode dp2;
#endif
	static ino_t inomap;
	char *blkbuf;
	void *indbuf;
	struct fs *fs;
	char *s;
	size_t n, nb, size, off, vboff;
	ufs_lbn_t lbn;
	ufs2_daddr_t addr, vbaddr;
	static ufs2_daddr_t blkmap, indmap;
	u_int u;

	blkbuf = dmadat->blkbuf;
	indbuf = dmadat->indbuf;
	fs = (struct fs *)dmadat->sbbuf;

	/* We take this if statement for the first call to fsread */
	if (!dsk_meta) {
		inomap = 0;
		for (n = 0; sblock_try[n] != -1; n++) {
			if (dskread(fs, sblock_try[n] / DEV_BSIZE,
			    SBLOCKSIZE / DEV_BSIZE))
				return -1;
			if ((
#if defined(UFS1_ONLY)
			     fs->fs_magic == FS_UFS1_MAGIC
#elif defined(UFS2_ONLY)
			    (fs->fs_magic == FS_UFS2_MAGIC &&
			    fs->fs_sblockloc == sblock_try[n])
#else
			     fs->fs_magic == FS_UFS1_MAGIC ||
			    (fs->fs_magic == FS_UFS2_MAGIC &&
			    fs->fs_sblockloc == sblock_try[n])
#endif
			    ) &&
			    fs->fs_bsize <= MAXBSIZE &&
			    fs->fs_bsize >= sizeof(struct fs))
				break;
		}
		if (sblock_try[n] == -1) {
			printf("Not ufs\n");
			return -1;
		}
		dsk_meta++;
	}
	if (!inode)
		return 0;
	if (inomap != inode) {
		n = IPERVBLK(fs);
		if (dskread(blkbuf, INO_TO_VBA(fs, n, inode), DBPERVBLK))
			return -1;
		n = INO_TO_VBO(n, inode);
#if defined(UFS1_ONLY)
		dp1 = ((struct ufs1_dinode *)blkbuf)[n];
#elif defined(UFS2_ONLY)
		dp2 = ((struct ufs2_dinode *)blkbuf)[n];
#else
		if (fs->fs_magic == FS_UFS1_MAGIC)
			dp1 = ((struct ufs1_dinode *)blkbuf)[n];
		else
			dp2 = ((struct ufs2_dinode *)blkbuf)[n];
#endif
		inomap = inode;
		fs_off = 0;
		blkmap = indmap = 0;
	}
	s = buf;
	size = DIP(di_size);
	n = size - fs_off;
	if (nbyte > n)
		nbyte = n;
	nb = nbyte;
	while (nb) {
		lbn = lblkno(fs, fs_off);
		off = blkoff(fs, fs_off);
		if (lbn < NDADDR) {
			addr = DIP(di_db[lbn]);
		} else if (lbn < NDADDR + NINDIR(fs)) {
			n = INDIRPERVBLK(fs);
			addr = DIP(di_ib[0]);
			u = (u_int)(lbn - NDADDR) / (n * DBPERVBLK);
			vbaddr = fsbtodb(fs, addr) + u;
			if (indmap != vbaddr) {
				if (dskread(indbuf, vbaddr, DBPERVBLK))
					return -1;
				indmap = vbaddr;
			}
			n = (lbn - NDADDR) & (n - 1);
#if defined(UFS1_ONLY)
			addr = ((ufs1_daddr_t *)indbuf)[n];
#elif defined(UFS2_ONLY)
			addr = ((ufs2_daddr_t *)indbuf)[n];
#else
			if (fs->fs_magic == FS_UFS1_MAGIC)
				addr = ((ufs1_daddr_t *)indbuf)[n];
			else
				addr = ((ufs2_daddr_t *)indbuf)[n];
#endif
		} else {
			return -1;
		}
		vbaddr = fsbtodb(fs, addr) + (off >> VBLKSHIFT) * DBPERVBLK;
		vboff = off & VBLKMASK;
		n = sblksize(fs, size, lbn) - (off & ~VBLKMASK);
		if (n > VBLKSIZE)
			n = VBLKSIZE;
		if (blkmap != vbaddr) {
			if (dskread(blkbuf, vbaddr, n >> DEV_BSHIFT))
				return -1;
			blkmap = vbaddr;
		}
		n -= vboff;
		if (n > nb)
			n = nb;
		memcpy(s, blkbuf + vboff, n);
		s += n;
		fs_off += n;
		nb -= n;
	}
	return nbyte;
}
```

### Structures for _dskread_

#### _dos\_partition_
```c
/* Located /sys/sys/diskmbr.h */

#define	DOSBBSECTOR	0	/* DOS boot block relative sector number */
#define	DOSPARTOFF	446
#define	DOSPARTSIZE	16
#define	NDOSPART	4
#define	NEXTDOSPART	32
#define	DOSMAGICOFFSET	510
#define	DOSMAGIC	0xAA55

#define	DOSPTYP_386BSD	0xa5	/* 386BSD partition type */
#define	DOSPTYP_LINSWP	0x82	/* Linux swap partition */
#define	DOSPTYP_LINUX	0x83	/* Linux partition */
#define	DOSPTYP_PMBR	0xee	/* GPT Protective MBR */
#define	DOSPTYP_EXT	5	/* DOS extended partition */
#define	DOSPTYP_EXTLBA	15	/* DOS extended partition */

struct dos_partition {
	unsigned char	dp_flag;	/* bootstrap flags */
	unsigned char	dp_shd;		/* starting head */
	unsigned char	dp_ssect;	/* starting sector */
	unsigned char	dp_scyl;	/* starting cylinder */
	unsigned char	dp_typ;		/* partition type */
	unsigned char	dp_ehd;		/* end head */
	unsigned char	dp_esect;	/* end sector */
	unsigned char	dp_ecyl;	/* end cylinder */
	u_int32_t	dp_start;	/* absolute starting sector number */
	u_int32_t	dp_size;	/* partition size in sectors */
};
```
#### _disklabel_ Structure

```c
/* Located /sys/sys/disklabel.h */

/*
 * Each disk has a label which includes information about the hardware
 * disk geometry, filesystem partitions, and drive specific information.
 * The label is in block 0 or 1, possibly offset from the beginning
 * to leave room for a bootstrap, etc.
 */

/* XXX these should be defined per controller (or drive) elsewhere, not here! */
#if defined(__i386__) || defined(__amd64__) || defined(__ia64__)
#define LABELSECTOR	1			/* sector containing label */
#define LABELOFFSET	0			/* offset of label in sector */
#endif

#ifdef __alpha__
#define LABELSECTOR	0
#define LABELOFFSET	64
#endif

#define DISKMAGIC	((u_int32_t)0x82564557)	/* The disk magic number */
#ifndef MAXPARTITIONS
#define	MAXPARTITIONS	8
#endif

/* Size of bootblock area in sector-size neutral bytes */
#define BBSIZE		8192

#define	LABEL_PART	2		/* partition containing label */
#define	RAW_PART	2		/* partition containing whole disk */
#define	SWAP_PART	1		/* partition normally containing swap */

struct disklabel {
	u_int32_t d_magic;		/* the magic number */
	u_int16_t d_type;		/* drive type */
	u_int16_t d_subtype;		/* controller/d_type specific */
	char	  d_typename[16];	/* type name, e.g. "eagle" */

	char      d_packname[16];	/* pack identifier */

			/* disk geometry: */
	u_int32_t d_secsize;		/* # of bytes per sector */
	u_int32_t d_nsectors;		/* # of data sectors per track */
	u_int32_t d_ntracks;		/* # of tracks per cylinder */
	u_int32_t d_ncylinders;		/* # of data cylinders per unit */
	u_int32_t d_secpercyl;		/* # of data sectors per cylinder */
	u_int32_t d_secperunit;		/* # of data sectors per unit */

	/*
	 * Spares (bad sector replacements) below are not counted in
	 * d_nsectors or d_secpercyl.  Spare sectors are assumed to
	 * be physical sectors which occupy space at the end of each
	 * track and/or cylinder.
	 */
	u_int16_t d_sparespertrack;	/* # of spare sectors per track */
	u_int16_t d_sparespercyl;	/* # of spare sectors per cylinder */
	/*
	 * Alternate cylinders include maintenance, replacement, configuration
	 * description areas, etc.
	 */
	u_int32_t d_acylinders;		/* # of alt. cylinders per unit */

			/* hardware characteristics: */
	/*
	 * d_interleave, d_trackskew and d_cylskew describe perturbations
	 * in the media format used to compensate for a slow controller.
	 * Interleave is physical sector interleave, set up by the
	 * formatter or controller when formatting.  When interleaving is
	 * in use, logically adjacent sectors are not physically
	 * contiguous, but instead are separated by some number of
	 * sectors.  It is specified as the ratio of physical sectors
	 * traversed per logical sector.  Thus an interleave of 1:1
	 * implies contiguous layout, while 2:1 implies that logical
	 * sector 0 is separated by one sector from logical sector 1.
	 * d_trackskew is the offset of sector 0 on track N relative to
	 * sector 0 on track N-1 on the same cylinder.  Finally, d_cylskew
	 * is the offset of sector 0 on cylinder N relative to sector 0
	 * on cylinder N-1.
	 */
	u_int16_t d_rpm;		/* rotational speed */
	u_int16_t d_interleave;		/* hardware sector interleave */
	u_int16_t d_trackskew;		/* sector 0 skew, per track */
	u_int16_t d_cylskew;		/* sector 0 skew, per cylinder */
	u_int32_t d_headswitch;		/* head switch time, usec */
	u_int32_t d_trkseek;		/* track-to-track seek, usec */
	u_int32_t d_flags;		/* generic flags */
#define NDDATA 5
	u_int32_t d_drivedata[NDDATA];	/* drive-type specific information */
#define NSPARE 5
	u_int32_t d_spare[NSPARE];	/* reserved for future use */
	u_int32_t d_magic2;		/* the magic number (again) */
	u_int16_t d_checksum;		/* xor of data incl. partitions */

			/* filesystem and partition information: */
	u_int16_t d_npartitions;	/* number of partitions in following */
	u_int32_t d_bbsize;		/* size of boot area at sn0, bytes */
	u_int32_t d_sbsize;		/* max size of fs superblock, bytes */
	struct partition {		/* the partition table */
		u_int32_t p_size;	/* number of sectors in partition */
		u_int32_t p_offset;	/* starting sector */
		u_int32_t p_fsize;	/* filesystem basic fragment size */
		u_int8_t p_fstype;	/* filesystem type, see below */
		u_int8_t p_frag;	/* filesystem fragments per block */
		u_int16_t p_cpg;	/* filesystem cylinders per group */
	} d_partitions[MAXPARTITIONS];	/* actually may be more */
};
```

#### _dsk_ Structure

```c
/* Located in boot2.c */

static struct dsk {
    unsigned drive;
    unsigned type;
    unsigned unit;
    unsigned slice;
    unsigned part;
    unsigned start;
    int init;
} dsk;
```

### _boot2.c_: _dskread_ and _drvread_

```c
static int
dskread(void *buf, unsigned lba, unsigned nblk)
{
    struct dos_partition *dp;
    struct disklabel *d;
    char *sec;
    unsigned sl, i;

    if (!dsk_meta) {	/* True for the first read */
	sec = dmadat->secbuf;
	dsk.start = 0;
	if (drvread(sec, DOSBBSECTOR, 1))	/* drvread(sec, 0, 1) */
	    return -1;
	dp = (void *)(sec + DOSPARTOFF);
	sl = dsk.slice;
	if (sl < BASE_SLICE) {	/* sl < 2 ? */
	    for (i = 0; i < NDOSPART; i++)	/* i = 0; i < 4; i++ */
			if (dp[i].dp_typ == DOSPTYP_386BSD &&
		    	(dp[i].dp_flag & 0x80 || sl < BASE_SLICE)) {
		    		sl = BASE_SLICE + i;
		    		if (dp[i].dp_flag & 0x80 ||
						dsk.slice == COMPATIBILITY_SLICE)	/* COMPATIBILITY_SLICE == 0 */
						break;
			}
	    if (dsk.slice == WHOLE_DISK_SLICE)
			dsk.slice = sl;
	}
	if (sl != WHOLE_DISK_SLICE) {
	    if (sl != COMPATIBILITY_SLICE)
		dp += sl - BASE_SLICE;
	    if (dp->dp_typ != DOSPTYP_386BSD) {
		printf("Invalid %s\n", "slice");
		return -1;
	    }
	    dsk.start = dp->dp_start;
	}
	if (drvread(sec, dsk.start + LABELSECTOR, 1))
		return -1;
	d = (void *)(sec + LABELOFFSET);
	if (d->d_magic != DISKMAGIC || d->d_magic2 != DISKMAGIC) {
	    if (dsk.part != RAW_PART) {
		printf("Invalid %s\n", "label");
		return -1;
	    }
	} else {
	    if (!dsk.init) {
		if (d->d_type == DTYPE_SCSI)
		    dsk.type = TYPE_DA;
		dsk.init++;
	    }
	    if (dsk.part >= d->d_npartitions ||
		!d->d_partitions[dsk.part].p_size) {
		printf("Invalid %s\n", "partition");
		return -1;
	    }
	    dsk.start += d->d_partitions[dsk.part].p_offset;
	    dsk.start -= d->d_partitions[RAW_PART].p_offset;
	}
    }
    return drvread(buf, dsk.start + lba, nblk);
}

static int
drvread(void *buf, unsigned lba, unsigned nblk)
{
    static unsigned c = 0x2d5c7c2f;

    printf("%c\b", c = c << 8 | c >> 24);		/* c = 0x5c7c2fd5 */
    v86.ctl = V86_ADDR | V86_CALLF | V86_FLAGS;
    v86.addr = XREADORG;		/* call to xread in boot1 */
    v86.es = VTOPSEG(buf);
    v86.eax = lba;
    v86.ebx = VTOPOFF(buf);
    v86.ecx = lba >> 16;
    v86.edx = nblk << 8 | dsk.drive;
    v86int();
    v86.ctl = V86_FLAGS;
    if (V86_CY(v86.efl)) {
		printf("error %u lba %u\n", v86.eax >> 8 & 0xff, lba);
		return -1;
    }
    return 0;
}
```
