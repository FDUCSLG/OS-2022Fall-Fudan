#pragma once

#define EXTMEM  0x80000    /* Start of extended memory */
#define PHYSTOP 0x3f000000 /* Top physical memory */

#define KSPACE_MASK 0xffff000000000000
#define KERNLINK    (KSPACE_MASK + EXTMEM) /* Address where kernel is linked */

#define K2P_WO(x) ((x) - (KSPACE_MASK)) /* Same as V2P, but without casts */
#define P2K_WO(x) ((x) + (KSPACE_MASK)) /* Same as P2V, but without casts */
