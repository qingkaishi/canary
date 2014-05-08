#ifndef PNSCAN_BM_H
#define PNSCAN_BM_H

#define BM_ASIZE 256 /* Allowed character code values */

typedef struct
{
    int *bmGs;
    int bmBc[BM_ASIZE];
    unsigned char *saved_x;
    int saved_m;
    int icase;
} BM;


extern int
bm_init(BM *bmp,
	unsigned char *x,
	int m,
	int icase);

extern int
bm_search(BM *bmp,
	  unsigned char *y,
	  size_t n,
	  int (*mfun)(unsigned char *buf, size_t n, size_t pos, void *misc),
	  void *misc);

extern void
bm_destroy(BM *bmp);

#endif
