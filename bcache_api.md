# Walthrough of FreeBSD Block Cache API

## Overview

1. _bcache\_init_
2. _read\_strategy_
3. _write\_strategy_
4. _bcache\_flush_
5. _bcache\_strategy_
6. _bcache\_insert_
7. _bcachce\_lookup_
8. _bcache\_invalidate_

## _bcache\_init_

```c
struct bcachectl
{
    daddr_t	bc_blkno;
    time_t	bc_stamp;
    int		bc_count;
};

static struct bcachectl	*bcache_ctl;
static caddr_t		bcache_data;
static bitstr_t		*bcache_miss;
static u_int		bcache_nblks;
static u_int		bcache_blksize;
static u_int		bcache_hits, bcache_misses, bcache_ops, bcache_bypasses;
static u_int		bcache_flushes;
static u_int		bcache_bcount;

static void	bcache_invalidate(daddr_t blkno);
static void	bcache_insert(caddr_t buf, daddr_t blkno);
static int	bcache_lookup(caddr_t buf, daddr_t blkno);

/*
 * Initialise the cache for (nblks) of (bsize).
 */
int
bcache_init(u_int nblks, size_t bsize)
{
    /* discard any old contents */
    if (bcache_data != NULL) {
	free(bcache_data);
	bcache_data = NULL;
	free(bcache_ctl);
    }

    /* Allocate control structures */
    bcache_nblks = nblks;
    bcache_blksize = bsize;
    bcache_data = malloc(bcache_nblks * bcache_blksize);
    bcache_ctl = (struct bcachectl *)malloc(bcache_nblks * sizeof(struct bcachectl));
    bcache_miss = bit_alloc((bcache_nblks + 1) / 2);
    if ((bcache_data == NULL) || (bcache_ctl == NULL) || (bcache_miss == NULL)) {
	if (bcache_miss)
	    free(bcache_miss);
	if (bcache_ctl)
	    free(bcache_ctl);
	if (bcache_data)
	    free(bcache_data);
	bcache_data = NULL;
	return(ENOMEM);
    }

    return(0);
}
```
