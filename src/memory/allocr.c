# include "allocr.h"
# include "../linux/mman.h"
# include "../linux/unistd.h"
# include "../linux/signal.h"
# include "../system/util/checksum.h"
# include "../system/thread.h"
# include "../system/mutex.h"
# include "../mode.h"
# include "../rat.h"
# include "../system/tls.h"
# include "../system/string.h"
# include "../dep/str_len.h"
# include "../dep/str_cpy.h"
# include "../system/io.h"
//	might want to keep track of mmaps

/*
	not using this until finished
	as it may hide issues.
*/
# define ALIGN 1
# define align_to(__no, __to)(((__no)+((__to)-1))&((~(__to))+1))
# define is_aligned_to(__no, __align)(((__no&__align) == __align)||!(__no&__align))
# define PAGE_SHIFT 8
# define PAGE_SIZE (1<<PAGE_SHIFT)
typedef ff_u32_t ar_off_t;
typedef ff_u32_t ar_size_t;
typedef ff_u32_t ar_uint_t;
typedef ff_s32_t ar_int_t;

/* recycle old spots
*/
# ifndef NULL
#	define NULL ((void*)0)
# endif

# define BLK_FREE 0x1
# define BLK_USED 0x2
# define USE_BRK 0x1
# define MMAPED 0x4

# define TRIM_MIN 0x13

# define MAX_SH 0xf
# define MAX_GR 0xf

# define is_flag(__flags, __flag) \
	(((__flags)&(__flag))==(__flag))

# define is_free(__blk) \
	is_flag((__blk)->flags, BLK_FREE)
# define is_used(__blk) \
	is_flag((__blk)->flags, BLK_USED)

# define get_blk(__pot, __off) \
	((blkdp)(((ff_u8_t*)(__pot)->end)+(__off)))

# define next_blk(__pot, __blk) \
	get_blk(__pot, (__blk)->next)
# define prev_blk(__pot, __blk) \
	get_blk(__pot, (__blk)->prev)

# define AR_NULL ((ar_off_t)~0)
# define is_null(__p) ((__p)==AR_NULL) 
# define not_null(__p) ((__p)!=AR_NULL)

# define POT_SIZE 0x986f
# define lkpot(__pot) \
	ffly_mutex_lock(&(__pot)->lock)
# define ulpot(__pot) \
	ffly_mutex_unlock(&(__pot)->lock)

# define lkrod(__rod) \
	ffly_mutex_lock(&(__rod)->lock)
# define ulrod(__rod) \
	ffly_mutex_unlock(&(__rod)->lock)

# define no_bins 81
# define bin_no(__bc) \
	(((ff_u32_t)(__bc))>>3 > 16?16+(((ff_u32_t)(__bc))>>7 > 16?16+(((ff_u32_t)(__bc))>>11 > 16?16+(((ff_u32_t)(__bc))>>15 > 16?16+(((ff_u32_t)(__bc))>>19 > 16?16: \
	(((ff_u32_t)(__bc))>>19)):(((ff_u32_t)(__bc))>>15)):(((ff_u32_t)(__bc))>>11)):(((ff_u32_t)(__bc))>>7)):(((ff_u32_t)(__bc))>>3))

# define get_bin(__pot, __bc) \
	(*((__pot)->bins+bin_no(__bc)))
# define bin_at(__pot, __bc) \
	((__pot)->bins+bin_no(__bc))
# include "../system/err.h"
# include <stdarg.h>
//# define DEBUG
/* not the best but will do */
void static
copy(void *__dst, void *__src, ff_uint_t __bc) {
	ff_u8_t *p = (ff_u8_t*)__dst;
	ff_u8_t *end = p+__bc;
	ff_uint_t left;
	while(p != end) {
		left = __bc-(p-(ff_u8_t*)__dst);
		if (left>>3 > 0) {
			*(ff_u64_t*)p = *(ff_u64_t*)((ff_u8_t*)__src+(p-(ff_u8_t*)__dst));
			p+=sizeof(ff_u64_t);
		} else {
			*p = *((ff_u8_t*)__src+(p-(ff_u8_t*)__dst));
			p++;
		}
	}
}

/*
	using real one will only take more time and cause more obstacles in the future
*/
void static
out(char const *__format, va_list __args) {
	char buf[2048];
	char *bufp = buf;
	char const *p = __format;
	char *end = p+ffly_str_len(__format);
	while(p != end) {
		if (*(p++) == '%') {
			if (*p == 'u') {
				ff_u32_t v = va_arg(__args, ff_u32_t);
				bufp+=ffly_nots(v, bufp);
			} else if (*p == 'x') {
				ff_u32_t v = va_arg(__args, ff_u32_t);
				bufp+=ffly_noths((ff_u64_t)v, bufp);
			} else if (*p == 'p') { 
				void *v = va_arg(__args, void*);
				*(bufp++) = '0';
				*(bufp++) = 'x';
				bufp+=ffly_noths((ff_u64_t)v, bufp);
			} else if (*p == 's') {
				char *s = va_arg(__args, char*);
				bufp+=ffly_str_cpy(bufp, s);
			}
			p++;
		} else
			*(bufp++) = *(p-1);
	}
	*bufp = '\0';
	ffly_fwrite(ffly_out, buf, bufp-buf);	
}

void static
printf(char const *__format, ...) {
	va_list args;
	va_start(args, __format);
	out(__format, args);
	va_end(args);
}

typedef struct rod *rodp;
typedef struct pot {
	rodp r;
	ar_uint_t page_c, blk_c;

	ar_off_t off, top_blk, end_blk;
	void *top, *end;
	ff_u8_t flags;
	ar_off_t bins[no_bins];
	struct pot *next, *fd, *bk;

	struct pot *previous;
	ar_uint_t buried, total;
	ff_mlock_t lock;
} *potp;

typedef struct rod {
	struct rod *next;
	ff_mlock_t lock;
    potp p;
	ff_u8_t no;
} *rodp;

# define rodno(__p) \
	(((((ff_u64_t)(__p))&0xff)^(((ff_u64_t)(__p))>>8&0xff)^(((ff_u64_t)(__p))>>16&0xff)^(((ff_u64_t)(__p))>>24&0xff)^\
	(((ff_u64_t)(__p))>>32&0xff)^(((ff_u64_t)(__p))>>40&0xff)^(((ff_u64_t)(__p))>>48&0xff)^(((ff_u64_t)(__p))>>56&0xff))>>2)
# define rod_at(__p) \
	(rods+rodno(__p))
rodp rods[64] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static struct pot main_pot;

// rename
static void *spot[30];

static ff_uint_t arena_tls;
static ff_uint_t temp_tls;
/*
	dont use pointers when we can use offsets to where the block is located 
	as it will take of less space in the header.
*/

// keep packed as we want the header to be as small as possible
typedef struct blkd {
	ar_off_t prev, next, fd, bk;
	ar_size_t size; // total size, without blkd
	/*
		junk part of block that wont be used.
		but can be used.

		zeroed when freed.
	*/
	ar_uint_t pad;
	ar_off_t off, end;
	ff_u8_t flags;
} __attribute__((packed)) *blkdp;

# define pot_size sizeof(struct pot)
# define blkd_size sizeof(struct blkd)
void static* _ffly_alloc(potp, ff_uint_t);
void static _ffly_free(potp, void*);

# define cur_arena \
	((potp)*(spot+ffly_tls_get(arena_tls)))
void static
abort(void) {
	printf("ar, abort.\n");
	exit(SIGKILL);
}

ff_uint_t static spot_id = 0;

ff_mlock_t static plock = FFLY_MUTEX_INIT;
void ffly_process_prep(void) {
	ffly_mutex_lock(&plock);
	ffly_tls_set(spot_id++, arena_tls);
	ffly_tls_set(spot_id++, temp_tls);
	*(spot+(spot_id-1)) = NULL;
	*(spot+(spot_id-2)) = NULL;
	ffly_mutex_unlock(&plock);
}

void ffly_arctl(ff_u8_t __req, ff_u64_t __val) {
	void **arena_spot = spot+ffly_tls_get(arena_tls);
	void **temp_spot = spot+ffly_tls_get(temp_tls);
	potp arena = (potp)*arena_spot;
	potp temp = (potp)*temp_spot;
	switch(__req) {
		case _ar_unset:
			arena = temp;
		break;
		case _ar_setpot:
			temp = arena;
			arena = (potp)__val;			
		break;
		case _ar_getpot:
			*(potp*)__val = arena;
		break;
	}

	*arena_spot = arena;
	*temp_spot = temp;
}

// add to rod
void static
atr(potp __pot, rodp __rod) {
	lkrod(__rod);
	__pot->r = __rod;
	__pot->next = __rod->p;
	if (__rod->p != NULL) {
		lkpot(__rod->p);
		__rod->p->previous = __pot;
		ulpot(__rod->p);
	}
	__rod->p = __pot;
	ulrod(__rod);
}

// remove from rod
void static
rfr(potp __pot) {
	rodp r = __pot->r;

	potp ft = __pot->next;
	potp rr = __pot->previous;
	lkrod(r);
	if (__pot == r->p) {
		if ((r->p = __pot->next) != NULL) {
			lkpot(r->p);
			r->p->previous = NULL;
			ulpot(r->p);
		}
	} else {
		if (ft != NULL) {
			lkpot(ft);
			ft->previous = rr;
		}
		if (rr != NULL) {
			lkpot(rr);
			rr->next = ft;
		}

		if (ft != NULL)
			ulpot(ft);
		if (rr != NULL)
			ulpot(rr);
	}
	ulrod(r);
}

void static
detatch(potp __pot, blkdp __blk) {
	ar_off_t *bin = bin_at(__pot, __blk->size);
	ar_off_t fwd = __blk->fd;
	ar_off_t bck = __blk->bk;
# ifdef __ffly_debug
	if (is_null(*bin))
		printf("bin at this location is dead.\n");
# endif
	if (__blk->off == *bin) {
# ifdef __ffly_debug
		if (not_null(bck))
			printf("block seems to have a back link.\n");
# endif
		if (not_null(*bin = fwd))
			get_blk(__pot, fwd)->bk = AR_NULL; 
	} else {
		if (not_null(fwd))
			get_blk(__pot, fwd)->bk = bck;
		if (not_null(bck))
			get_blk(__pot, bck)->fd = fwd;
	}

	// clear
	__blk->fd = AR_NULL;
	__blk->bk = AR_NULL;
}

void static
recouple(potp __pot, blkdp __blk) {
	ar_off_t *top = &__pot->top_blk;
	ar_off_t *end = &__pot->end_blk;
	ar_off_t off = __blk->off;
	if (not_null(*top)) {
		if (off < get_blk(__pot, *top)->off)
			*top = off;
	} else if (__pot->end == (void*)__blk)
		*top = off;

	if (not_null(*end)) {
		if (off > get_blk(__pot, *end)->off)
			*end = off;
	} else if (__pot->off == __blk->end)
		*end = off;

	if (not_null(__blk->next))
		next_blk(__pot, __blk)->prev = off;
	if (not_null(__blk->prev))
		prev_blk(__pot, __blk)->next = off; 
}

void static
decouple(potp __pot, blkdp __blk) {
	ar_off_t *top = &__pot->top_blk;
	ar_off_t *end = &__pot->end_blk;
	ar_off_t off = __blk->off;

	ar_off_t rr = __blk->prev;
	ar_off_t ft = __blk->next;

	if (off == *top) {
		if (not_null(*top = ft))
			get_blk(__pot, *top)->prev = AR_NULL;
		else //make sure
			*end = AR_NULL;
		return;
	}

	if (off == *end) {
		if (not_null(*end = rr))
			get_blk(__pot, *end)->next = AR_NULL;
		return;
	}

	if (not_null(ft))
		next_blk(__pot, __blk)->prev = rr;
	if (not_null(rr))
		prev_blk(__pot, __blk)->next = ft;
}

/* dead memory */
/*
	memory that hasent been touched yet
*/
ff_u64_t static
ffly_ardead(potp __pot) {
	return __pot->top-(__pot->end+__pot->off);
}

/* used memory */
ff_u64_t static
ffly_arused(potp __pot) {
	return __pot->off-((__pot->blk_c*blkd_size)+__pot->buried);
}

/* buried memory */
/*
	memory that is occupied but not being used at this moment
*/
ff_u64_t static
ffly_arburied(potp __pot) {
	return __pot->buried;
}

# include "../system/nanosleep.h"
void ffly_arstat(void) {
	potp arena = cur_arena;
	ff_uint_t no = 0;
	rodp r = *rods;
	potp cur = &main_pot;
_next:
	if (cur != NULL) {
		printf("potno: %u, rodno: %u - %s, off{%u}, no mans land{from: 0x%x, to: 0x%x}, pages{%u}, blocks{%u}, used{%u}, buried{%u}, dead{%u}\n",
			no++, !cur->r?0:cur->r->no, !cur->r?"bad":"ok", cur->off, cur->off, cur->top-cur->end, cur->page_c, cur->blk_c, ffly_arused(cur), ffly_arburied(cur), ffly_ardead(cur));

		if ((cur = cur->next) != NULL)
			goto _next;
	}

	if (cur == &main_pot) {
		cur = r->p;
		goto _next;
	}

	if ((r = r->next) != *rods) {
		cur = r->p;
		goto _next;
	}
}

ff_err_t init_pot(potp __pot) {
	__pot->top_blk = AR_NULL;
	__pot->end_blk = AR_NULL;
	__pot->off = 0;
	__pot->r = NULL;
	__pot->page_c = 0;
	__pot->blk_c = 0;
	__pot->flags = 0;
	__pot->next = NULL;
	__pot->previous = NULL;
	__pot->flags = 0x0;
	__pot->fd = NULL;
	__pot->bk = NULL;
	__pot->buried = 0;
	__pot->lock = FFLY_MUTEX_INIT;
	ar_off_t *bin = __pot->bins;
	while(bin != __pot->bins+no_bins)
		*(bin++) = AR_NULL;
}

void static tls_init(void) {
	ffly_tls_set(0, arena_tls);
	ffly_tls_set(0, temp_tls);
}

ff_err_t ffly_ar_init(void) {
	arena_tls = ffly_tls_alloc();
	temp_tls = ffly_tls_alloc();
	ffly_tls_toinit(tls_init);
	tls_init();
	ffly_process_prep();
	init_pot(&main_pot);
	main_pot.end = brk((void*)0);
	main_pot.top = main_pot.end;
	main_pot.flags |= USE_BRK;

	rodp *p = rods;
	while(p != rods+64) {
		*(*(p++) = (rodp)_ffly_alloc(&main_pot, sizeof(struct rod))) =
			(struct rod){.lock=FFLY_MUTEX_INIT,.p=NULL, .no=(p-rods)-1};
	}

	while(p != rods+1)
		(*--p)->next = *(p-1);
	(*rods)->next = *(rods+63);
}

ff_err_t _ffly_ar_cleanup(potp __pot) {
	brk(__pot->end);
}

ff_err_t ffly_ar_cleanup(void) {
	_ffly_ar_cleanup(&main_pot);
}

void pot_pr(potp __pot) {
	potp p = __pot;
	blkdp blk = get_blk(p, p->top_blk);
	ff_uint_t static depth = 0;
	if (is_null(p->top_blk)) return;
	if (p->fd != NULL) {
		printf("\ndepth, %u\n", depth++);
		pot_pr(p->fd);
		printf("**end\n");
	}
_next:
	printf("/-----------------------\\\n");
	printf("| size: %u, off: %x, pad: %u, inuse: %s\n", blk->size, blk->off, blk->pad, is_used(blk)?"yes":"no");
	printf("| prev: %x{%s}, next: %x{%s}, fd: %x{%s}, bk: %x{%s}\n", is_null(blk->prev)?0:blk->prev, is_null(blk->prev)?"dead":"alive", is_null(blk->next)?0:blk->next, is_null(blk->next)?"dead":"alive",
		is_null(blk->fd)?0:blk->fd, is_null(blk->fd)?"dead":"alive", is_null(blk->bk)?0:blk->bk, is_null(blk->bk)?"dead":"alive");
	printf("\\----------------------/\n");
	if (not_null(blk->next)) {
		blk = get_blk(p, blk->next);
		goto _next;		   
	}
}
# include "../maths/abs.h"
void pot_pf(potp __pot) {
	potp p = __pot;
	ar_off_t *bin = p->bins;
	blkdp blk, bk;
	ff_uint_t static depth = 0;
	if (p->fd != NULL) {
		if (!depth)
			printf("**start\n");
		printf("\ndepth, %u, %u\n", depth++, p->fd->off);
		pot_pf(p->fd);
		printf("**end\n");
	}

_next:
	bk = NULL;
	if (is_null(*bin)) goto _sk;
	if (*bin >= p->off) {
		printf("bin is messed up, at: %u, off: %u\n", bin-p->bins, *bin);
		goto _sk;
	}

	blk = get_blk(p, *bin);
_fwd:
	if (bk != NULL) {
		printf("\\\n");
		printf(" > %u-bytes\n", ffly_abs((ff_int_t)bk->off-(ff_int_t)blk->off));
		printf("/\n");
	}
	printf("/-----------------------\\\n");
	printf("| size: %u\t\t| 0x%x, pad: %u\n", blk->size, blk->off, blk->pad);
	printf("\\----------------------/\n");
	if (not_null(blk->fd)) {
		bk = blk;
		blk = get_blk(p, blk->fd);
		goto _fwd;
	}

_sk:
	if (bin != p->bins+(no_bins-1)) {
		bin++;
		goto _next;
	}
}

void ffly_arbl(void *__p) {
	blkdp blk = (blkdp)((ff_u8_t*)__p-blkd_size);
	printf("block: off: %u, size: %u, pad: %u\n", blk->off, blk->size, blk->pad);
}

void pr(void) {
	potp arena = cur_arena;
	printf("\n**** all ****\n");
	rodp r = *rods;
	potp p = &main_pot;
	ff_uint_t no = 0;
_next:
	if (p != NULL) {
		if (p == &main_pot)
			printf("~: main pot, no{%u}\n", no);
		printf("\npot, %u, used: %u, buried: %u, dead: %u, off: %u, pages: %u, blocks: %u, total: %u\n", no++, ffly_arused(p), ffly_arburied(p), ffly_ardead(p), p->off, p->page_c, p->blk_c, p->total);
		pot_pr(p);
		if ((p = p->next) != NULL)
			goto _next;
	}

	if (p == &main_pot) {
		p = r->p;
		goto _next;
	}

	if ((r = r->next) != *rods) {
		p = r->p;
		goto _next;
	}
}

void pf(void) {
	potp arena = cur_arena;
	printf("\n**** free ****\n");
	rodp r = *rods;
	potp p = &main_pot;
	ff_uint_t no = 0;
_next:
	if (p != NULL) {
		printf("\npot, %u, used: %u, buried: %u, dead: %u, off: %u, pages: %u, blocks: %u, total: %u\n", no++, ffly_arused(p), ffly_arburied(p), ffly_ardead(p), p->off, p->page_c, p->blk_c, p->total);
		pot_pf(p);

		if ((p = p->next) != NULL)
			goto _next;
	}

	if (p == &main_pot) {
		p = r->p;		
		goto _next;
	}

	if ((r = r->next) != *rods) {
		p = r->p;
		goto _next;
	}
}


void free_pot(potp);

void ffly_arhang(void) {
	void **arena_spot = spot+ffly_tls_get(arena_tls);
	potp arena = (potp)*arena_spot;
	potp p;
	if (!(p = arena)) {
# ifdef __ffly_debug
		printf("dead pot.\n");
# endif
		return;
	}

	potp bk;
	while(p != NULL) {
		bk = p;
		lkpot(p);
		p = p->fd;
		if (bk->off>0) {
			if (bk == arena) {
				potp prev = arena->previous;
				potp next = arena->next;
				if ((arena = bk->fd) != NULL) {
					arena->bk = NULL;
					if (prev != NULL) {
						lkpot(prev);
						prev->next = arena;	
						ulpot(prev);
					}

					if (next != NULL) {
						lkpot(next);
						next->previous = arena;
						ulpot(next);
					}
				}
			}

			if (bk->fd != NULL)
				bk->fd->bk = bk->bk;
			if (bk->bk != NULL)
				bk->bk->fd = bk->fd;
			bk->fd = NULL;
			bk->bk = NULL;
		}
		ulpot(bk);
	}
	*arena_spot = arena;
}

void ffly_araxe(void) {
	void **arena_spot = spot+ffly_tls_get(arena_tls);
	potp arena = (potp)*arena_spot;

	potp p;
	if (!(p = arena)) {
# ifdef __ffly_debug
		printf("ar, pot has already been axed, or has been like this from the start.\n");
# endif
		return;
	}

	rfr(p);
	potp bk;
	while(p != NULL) {
		bk = p;
		p = p->fd;
		rfr(bk);
		free_pot(bk);
	}
	arena = NULL;
	*arena_spot = arena;
}

/*
	has not been finished
	TODO:
*/

void static*
shrink_blk(potp __pot, blkdp __blk, ar_uint_t __size) {
	if (__blk->size <= __size || is_free(__blk)) {
# ifdef __ffly_debug
		printf("forbidden alteration.\n");
# endif
		return NULL;
	}

	ar_uint_t size;
	void *p;
	if (!(p = ffly_alloc(size = (__blk->size-__blk->pad)))) {
		//error
	}

	lkpot(__pot);
	blkdp rr = prev_blk(__pot, __blk); //rear
	blkdp ft = next_blk(__pot, __blk); //front
//	blkdp fwd = get_blk(__pot, __blk->fd);
//	blkdp bck = get_blk(__pot, __blk->bk);
	struct blkd blk;

	void *ret = NULL;

	// how much to shrink
	ar_uint_t dif = (__blk->size-__blk->pad)-__size;
	ar_off_t *off = &__blk->off;
	ff_u8_t freed, inuse;
# ifdef __ffly_debug
	__ffmod_debug
		ff_rat(_ff_rat_1, out, "attempting to shrink block by %u bytes.\n", dif);
# endif
	if (dif>MAX_SH) {
# ifdef __ffly_debug
		__ffmod_debug
			ff_rat(_ff_rat_1, out, "can't strink block, %u bytes is too much to cutoff.\n", dif);
# endif
		goto _r;	
	}

	if (dif <= 0x4 && __blk->pad < 0x4) {
		__blk->pad+=dif;
		ret = (void*)((ff_u8_t*)__blk+blkd_size);
		goto _r;
	}

	if (not_null(__blk->prev)) {
		if ((freed = is_free(rr)) || ((inuse = is_used(rr)) && rr->size <= 0xf)) {
			if (freed) {
				__pot->buried+=dif;
				detatch(__pot, rr);
			}

			copy(p, (ff_u8_t*)__blk+blkd_size, size);
			*off+=dif;
			if (*off-dif == __pot->end_blk)
				__pot->end_blk = *off;
			rr->next = *off;
			if (not_null(__blk->next))
				ft->prev = *off;
			__blk->size-=dif;
			if (inuse)
				rr->pad+=dif;

			rr->size+=dif;
			rr->end+=dif;
			copy(&blk, __blk, blkd_size);
			__blk = (blkdp)((ff_u8_t*)__blk+dif);
			copy(__blk, &blk, blkd_size);
			ret = (void*)((ff_u8_t*)__blk+blkd_size);
			copy(ret, p, size-dif);
			if (freed) {
				ar_off_t *bin;
				if (not_null(rr->fd = *(bin = bin_at(__pot, rr->size)))) {
					get_blk(__pot, *bin)->bk = rr->off;		
				}

				*bin = rr->off;
			}
			goto _r;
		}
	}
# ifdef __ffly_debug
	ff_rat(_ff_rat_1, out, "can't shrink block.\n");
# endif
_r:
	ulpot(__pot);
	ffly_free(p);
	return ret;
}

void static*
grow_blk(potp __pot, blkdp __blk, ar_uint_t __size) {
	if (__blk->size-__blk->pad >= __size || is_free(__blk)) {
# ifdef __ffly_debug
		printf("forbidden alteration.\n");
# endif
		return NULL;
	}

	ar_uint_t size;
	void *p;
	if (!(p = ffly_alloc(size = (__blk->size-__blk->pad)))) {
		//error
	}

	lkpot(__pot);
	blkdp rr = prev_blk(__pot, __blk); //rear
	blkdp ft = next_blk(__pot, __blk); //front
	struct blkd blk;

	void *ret = NULL;

	ar_uint_t dif = __size-(__blk->size-__blk->pad);
	ar_off_t *off = &__blk->off;
	ff_u8_t freed, inuse;
# ifdef __ffly_debug
	__ffmod_debug
		ff_rat(_ff_rat_1, out, "attempting to grow block by %u bytes.\n", dif);
# endif
	if (dif>MAX_GR) {
# ifdef __ffly_debug
		__ffmod_debug
			ff_rat(_ff_rat_1, out, "can't grow block, %u bytes is to much to add on.\n", dif);
# endif
		goto _r;
	}

	printf("at: %u\n", rr->flags);

	if (__blk->pad >= dif) {
		__blk->pad-=dif;
		ret = (void*)((ff_u8_t*)__blk+blkd_size);
		goto _r;
	}

	if (not_null(__blk->prev)) {
		if (((freed = is_free(rr)) && rr->size > dif<<1) || ((inuse = is_used(rr)) && rr->pad >= dif)) {
			if (freed) {
				__pot->buried-=dif;
				detatch(__pot, rr);
			}
			copy(p, (ff_u8_t*)__blk+blkd_size, size);
			*off-=dif;
			if (*off+dif == __pot->end_blk)
				__pot->end_blk = *off;
			rr->next = *off;
			if (not_null(__blk->next))
				ft->prev = *off;
			__blk->size+=dif;
			rr->size-=dif;
			rr->end-=dif;
			if (inuse)
				rr->pad-=dif;
			copy(&blk, __blk, blkd_size);
			__blk = (blkdp)((ff_u8_t*)__blk-dif);
			copy(__blk, &blk, blkd_size);
			ret = (void*)((ff_u8_t*)__blk+blkd_size);
			copy(ret, p, size);
			if (freed) {
				ar_off_t *bin;
				if (not_null(rr->fd = *(bin = bin_at(__pot, rr->size)))) {
					get_blk(__pot, *bin)->bk = rr->off;
				}

				*bin = rr->off;
			}
			goto _r;
		}
	}
# ifdef __ffly_debug
	ff_rat(_ff_rat_1, out, "can't grow block.\n");
# endif
_r:
	ulpot(__pot);
	ffly_free(p);
	return ret;
}

// user level
void*
ffly_arsh(void *__p, ff_uint_t __to) {
	potp arena = cur_arena;
	blkdp blk = (blkdp)((ff_u8_t*)__p-blkd_size);
	potp p = arena, bk;
	lkpot(p);
	while(!(__p >= p->end && __p < p->top)) {
		bk = p;
		p = p->fd;
		ulpot(bk);
		if (!p) {
# ifdef __ffly_debug
			printf("ar, address '%p' can't be found.\n", __p);
# endif
			return NULL;
		}
		lkpot(p);
	}
	ulpot(p);
	void *ret;
	
	ret = shrink_blk(p, blk, __to);
	return ret;
}

void*
ffly_argr(void *__p, ff_uint_t __to) {
	potp arena = cur_arena;
	blkdp blk = (blkdp)((ff_u8_t*)__p-blkd_size);
	potp p = arena, bk;
	lkpot(p);
	while(!(__p >= p->end && __p < p->top)) {
		bk = p;
		p = p->fd;
		ulpot(bk);	
		if (!p) {
# ifdef __ffly_debug
			printf("ar, address '%p' can't be found.\n", __p);
# endif
			return NULL;
		}
		lkpot(p);
	}
	ulpot(p);
	void *ret; 
	
	ret = grow_blk(p, blk, __to);
	return ret;
}

potp alloc_pot(ff_uint_t __size) {
	potp p;
	if (!(p = (potp)_ffly_alloc(&main_pot, pot_size))) {
		// err
	}

	init_pot(p);
	p->page_c = (__size>>PAGE_SHIFT)+((__size&((~(ff_u64_t)0)>>(64-PAGE_SHIFT)))>0);

	ff_uint_t size = p->page_c*PAGE_SIZE;
	p->end = _ffly_alloc(&main_pot, size);
	p->top = (void*)((ff_u8_t*)p->end+size);
	p->total = p->top-p->end;
	return p;
}

void free_pot(potp __pot) {
	_ffly_free(&main_pot, __pot->end);
	_ffly_free(&main_pot, __pot);
}

void*
_ffly_alloc(potp __pot, ff_uint_t __bc) {
	lkpot(__pot);
	ar_off_t *bin;
	bin = bin_at(__pot, __bc);
_bk:
	if (not_null(*bin)) {
		ar_off_t cur = *bin;
		while(not_null(cur)) {
			blkdp blk;
			if ((blk = get_blk(__pot, cur))->size >= __bc) {
				detatch(__pot, blk);
				blk->pad = blk->size-__bc;
				blk->flags = (blk->flags&~BLK_FREE)|BLK_USED;
				__pot->buried-=blk->size;

				/*
				* if block exceeds size then trim it down and split the block into two parts.
				*/
				ar_uint_t junk;
				if ((junk = (blk->size-__bc)) > blkd_size+TRIM_MIN) {
					__pot->blk_c++;

					ar_off_t off = blk->off+blkd_size+__bc;
					blkdp p; 
					*(p = (blkdp)((ff_u8_t*)__pot->end+off)) = (struct blkd){
						.prev = blk->off, .next = blk->next,
						.size = junk-blkd_size, .off = off, .end = blk->end,
						.fd = AR_NULL, .bk = AR_NULL,
						.flags = BLK_USED,
						.pad = 0
					};

					blk->pad = 0;
					blk->size = __bc;
					blk->next = p->off;
					blk->end = off;

					if (__pot->off == p->end)
						__pot->end_blk = p->off;
			  
					if (not_null(p->next))
						get_blk(__pot, p->next)->prev = p->off;
					ulpot(__pot);
					_ffly_free(__pot, (void*)((ff_u8_t*)p+blkd_size));
					goto _sk;
				}
				ulpot(__pot);
			_sk:
				return (void*)((ff_u8_t*)blk+blkd_size);
			}
			cur = blk->fd;
		}
	}

	// for now
	if (bin != __pot->bins+(no_bins-1)) {
		bin++;
		goto _bk;
	}

	ar_uint_t size = align_to(blkd_size+__bc, ALIGN);
	ar_off_t top = __pot->off+size;
	if (is_flag(__pot->flags, USE_BRK)) {
		ar_uint_t page_c;
		if ((page_c = ((top>>PAGE_SHIFT)+((top&((~(ff_u64_t)0)>>(64-PAGE_SHIFT)))>0))) > __pot->page_c) {
		//if ((page_c = ((top>>PAGE_SHIFT)+((top-((top>>PAGE_SHIFT)*PAGE_SIZE))>0))) > __pot->page_c) {
			if (brk(__pot->top = (void*)((ff_u8_t*)__pot->end+((__pot->page_c = page_c)*PAGE_SIZE))) == (void*)-1) {
# ifdef __ffly_debug
				printf("error: brk.\n");
# endif
				abort();
			}
		}
	} else {
		if (top >= __pot->top-__pot->end) {
			ulpot(__pot);
			return NULL;
		}
	}

	blkdp blk;
	*(blk = (blkdp)((ff_u8_t*)__pot->end+__pot->off)) = (struct blkd) {
		.prev = __pot->end_blk, .next = AR_NULL,
		.size = size-blkd_size, .off = __pot->off, .end = top,
		.fd = AR_NULL, .bk = AR_NULL,
		.flags = BLK_USED
	};

	blk->pad = blk->size-__bc;
	if (is_null(__pot->top_blk))
		__pot->top_blk = __pot->off;
		
	if (not_null(__pot->end_blk))
		get_blk(__pot, __pot->end_blk)->next = __pot->off;
	__pot->end_blk = __pot->off;
	__pot->off = top;

	__pot->blk_c++;
	ulpot(__pot);
	return (void*)((ff_u8_t*)blk+blkd_size);
}

# include "../dep/bzero.h"
void*
ffly_alloc(ff_uint_t __bc) {
	void **arena_spot = spot+ffly_tls_get(arena_tls);
	potp arena = (potp)*arena_spot;
	if (!__bc) return NULL;
	if (__bc+blkd_size >= POT_SIZE) {
		void *p;
		if ((p = mmap(NULL, blkd_size+__bc, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0)) == (void*)-1) {
			// failure
			abort();
		}
		blkdp blk = (blkdp)p;
		blk->size = __bc;
		blk->flags = MMAPED;
		return (void*)((ff_u8_t*)p+blkd_size);
	}

	if (!arena) {
		arena = alloc_pot(POT_SIZE);
		atr(arena, *rod_at(arena->end));
	}

	potp p = arena, t;
	void *ret;
_again:
	if (!(ret = _ffly_alloc(p, __bc))) {
		lkpot(p);
		if (p->fd != NULL){
			t = p->fd;
			ulpot(p);
			p = t;
			goto _again;
		}
		ulpot(p);
	}

	if (!ret) {
		if (!(t = alloc_pot(POT_SIZE))) {
			abort();
		}

		atr(t, *rod_at(t->end));

		lkpot(p);
		p->fd = t;
		lkpot(p->fd);
		p->fd->bk = p;
		ulpot(p->fd);
		t = p->fd;
		ulpot(p);
		p = t;
		goto _again;  
	}	

	*arena_spot = arena;
	return ret;
}

void
_ffly_free(potp __pot, void *__p) {
	lkpot(__pot);

	blkdp blk = (blkdp)((ff_u8_t*)__p-blkd_size);
	if (is_free(blk)) {
# ifdef __ffly_debug
		printf("error: block has already been freed, at: %p\n", __p);
# endif
		goto _end;
	}

	decouple(__pot, blk);
# ifdef __ffly_debug
	__ffmod_debug
		ff_rat(_ff_rat_0, out, "to free: %u\n", blk->size);
# endif
	blkdp prev, next, top = NULL, end = NULL;
	if (not_null(blk->prev)) {
		prev = prev_blk(__pot, blk);
		while(is_free(prev)) {
			__pot->blk_c--;
			__pot->buried-=prev->size;
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_0, out, "found free space above, %u\n", prev->size);
# endif
			detatch(__pot, prev);
			decouple(__pot, prev);
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_0, out, "total freed: %u\n", prev->end-prev->off); 
# endif
			top = prev;
			if (is_null(prev->prev)) break;
			prev = prev_blk(__pot, prev);
		}
	}

	if (not_null(blk->next)) {
		next = next_blk(__pot, blk);
		while(is_free(next)) {
			__pot->blk_c--;
			__pot->buried-=next->size;
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_0, out, "found free space below, %u\n", next->size);
# endif
			detatch(__pot, next);
			decouple(__pot, next);
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_0, out, "total freed: %u\n", next->end-next->off);
# endif
			end = next;
			if (is_null(next->next)) break;
			next = next_blk(__pot, next);
		}
	}

	if (top != NULL) {
		top->size = ((top->end = blk->end)-top->off)-blkd_size;
		top->next = blk->next;
		blk = top;
	}

	if (end != NULL) {
		blk->size = ((blk->end = end->end)-blk->off)-blkd_size;
		blk->next = end->next;
	}

	// only used when used
	blk->pad = 0;
# ifdef __ffly_debug
	__ffmod_debug
		ff_rat(_ff_rat_0, out, "freed: %u, %u\n", blk->size, blk->off);
# endif
	if (blk->off>=__pot->off) {
# ifdef __ffly_debug
		printf("somthing is wong.\n");
# endif
	}

	if (blk->end == __pot->off) {
		__pot->blk_c--;
		__pot->off = blk->off;
		if (is_flag(__pot->flags, USE_BRK)) { 
			ff_uint_t page_c;
			if ((page_c = ((__pot->off>>PAGE_SHIFT)+((__pot->off&((~(ff_u64_t)0)>>(64-PAGE_SHIFT)))>0))) < __pot->page_c) {
//			if ((page_c = ((__pot->off>>PAGE_SHIFT)+((__pot->off-((__pot->off>>PAGE_SHIFT)*PAGE_SIZE))>0))) < __pot->page_c) {
				if (brk(__pot->top = (void*)((ff_u8_t*)__pot->end+((__pot->page_c = page_c)*PAGE_SIZE))) == (void*)-1) {
# ifdef __ffly_debug
					printf("error: brk.\n");
# endif
					abort();
				}
			}
		}
		goto _end;
	}

	__pot->buried+=blk->size;

	recouple(__pot, blk);
	blk->flags = (blk->flags&~BLK_USED)|BLK_FREE;
	ar_off_t bin;
	if (not_null(bin = get_bin(__pot, blk->size))) {
		get_blk(__pot, bin)->bk = blk->off;
		blk->fd = bin;
	}
	*(__pot->bins+bin_no(blk->size)) = blk->off;

_end:
	ulpot(__pot);
}

void
ffly_free(void *__p) {
	void **arena_spot = spot+ffly_tls_get(arena_tls);
	potp arena = (potp)*arena_spot;
	if (!__p) {
# ifdef __ffly_debug
		printf("error: got null ptr.\n");
# endif
		return;
	}
	blkdp blk = (blkdp)((ff_u8_t*)__p-blkd_size);
	if (is_flag(blk->flags, MMAPED)) {
		munmap((void*)blk, blkd_size+blk->size);
		return;
	}
	potp p = arena, bk;
	rodp r = NULL, beg;
_bk:
	if (!p) {
		r = *rod_at(__p);
		beg = r;
		lkrod(r);
		rodp bk;
		while(!(bk = r)->p) {
			if ((r = r->next) == beg) {
# ifdef __ffly_debug
				printf("error.\n");
# endif
				ulrod(bk);
				abort();
			}
			ulrod(bk);
			lkrod(r);
		}
	
		if (!(p = r->p)) {
# ifdef __ffly_debug
			printf("error.\n");
# endif
			ulrod(r);
			abort();
		}

		ulrod(r);
	}

	// look for pot associated with pointer
	lkpot(p);
	while(!(__p >= p->end && __p < p->top)) {
		bk = p;
		p = !r?p->fd:p->next;
		ulpot(bk);
		if (!p) {
			if (r != NULL) {
				rodp bk;
				lkrod(r);
				while(!(p = (r = (bk = r)->next)->p) && r != beg) {
					ulrod(bk);
					lkrod(r);
				}
				ulrod(bk);

				if (r == beg) {
# ifdef __ffly_debug
					printf("error: could not find pot associated with pointer.\n");
# endif
					abort();
				}
			}
			if (!p) {
				if (!r)
					goto _bk;
				else {
# ifdef __ffly_debug
					printf("error.\n");
# endif
					return;
				}
			}
		}

		lkpot(p);
	}
	ulpot(p);

	_ffly_free(p, __p);
	// free pot as we don't need it
	lkpot(p);
	if (!p->off && p != arena && !r) {
		if (p->bk != NULL) {
			lkpot(p->bk);
			p->bk->fd = p->fd;
			ulpot(p->bk);
		}
		if (p->fd != NULL) {
			lkpot(p->fd);
			p->fd->bk = p->bk;
			ulpot(p->fd);
		}
		ulpot(p);

		rfr(p);
		free_pot(p);
		return;
	}
	ulpot(p);
	*arena_spot = arena;
}

void*
ffly_realloc(void *__p, ff_uint_t __bc) {
	if (!__p) {
# ifdef __ffly_debug
		printf("error: got null ptr.\n");
# endif
		return NULL;
	}

	blkdp blk = (blkdp)((ff_u8_t*)__p-blkd_size);
	ar_uint_t size = blk->size-blk->pad;
	ar_int_t dif = (ar_int_t)size-(ar_int_t)__bc;
	void *p;
	if (!is_flag(blk->flags, MMAPED)) {
		if (dif>0) {
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_1, out, "shrink.\n");
# endif
/*			if ((p = ffly_arsh(__p, __bc)) != NULL) {
				ff_rat(_ff_rat_1, "shrunk %u bytes.\n", dif);
				return p;
			}*/
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_1, out, "can't shrink block.\n");
# endif
		} else if (dif<0) {
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_1, out, "grow.\n");
# endif
/*			if ((p = ffly_argr(__p, __bc)) != NULL) {
				ff_rat(_ff_rat_1, "grew %u bytes.\n", -dif);
				return p;
			}*/
# ifdef __ffly_debug
			__ffmod_debug
				ff_rat(_ff_rat_1, out, "can't grow block.\n");
# endif
		}
	}

	if (!(p = ffly_alloc(__bc))) {
		// failure
	}

# ifdef __ffly_debug
	__ffmod_debug
		ff_rat(_ff_rat_1, out, "prev size: %u, new size: %u, copysize: %u\n", size, __bc, size<__bc?size:__bc);
# endif
	copy(p, __p, size<__bc?size:__bc);
	ffly_free(__p);
	return p;
}

