# Walthrough of Matt Dillon's _malloc_ (libstand)

### Overview

1. _zalloc_ Amalgamated Header File

### _zalloc_ Amalgamated Header File

```c
#ifdef __i386__
typedef unsigned int iaddr_t;	/* unsigned int same size as pointer	*/
typedef int saddr_t;		/* signed int same size as pointer	*/
#endif

/*
 * H/MEM.H
 *
 * Basic memory pool / memory node structures.
 */

typedef struct MemNode {
    struct MemNode	*mr_Next;
    iaddr_t		mr_Bytes;
} MemNode;

typedef struct MemPool {
    void		*mp_Base;  
    void		*mp_End;
    MemNode		*mp_First; 
    iaddr_t		mp_Size;
    iaddr_t		mp_Used;
} MemPool;

#define MEMNODE_SIZE_MASK       ((sizeof(MemNode) <= 8) ? 7 : 15)

#define ZNOTE_FREE	0
#define ZNOTE_REUSE	1

/*
 * required malloc alignment.  Use sizeof(long double) for architecture
 * independance.
 *
 * Note: if we implement a more sophisticated realloc, we should ensure that
 * MALLOCALIGN is at least as large as MemNode.
 */

typedef struct Guard {
    size_t	ga_Bytes;
    size_t	ga_Magic;	/* must be at least 32 bits */
} Guard;
```
