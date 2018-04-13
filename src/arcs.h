# ifndef __ffly__arcs__h
# define __ffly__arcs__h
# include "ffint.h"
# include "system/lat.h"
# include "system/flags.h"
# define REC_FLG_TOFREE 0x1

/*
	write to record
	read to record
*/
# define ffly_arcs_recw(__name, ...) \
	ffly_arc_recw(ffly_arc_lookup(__ffly_arccur__, *ffly_arcs_alias(__name)), no, __VA_ARGS__)
# define ffly_arcs_recr(__name, ...) \
	ffly_arc_recr(ffly_arc_lookup(__ffly_arccur__, *ffly_arcs_alias(__name)), __VA_ARGS__)

// create record
# define ffly_arcs_creatrec(__name, ...) \
	ffly_arc_creatrec(__ffly_arccur__, (*ffly_arcs_alias(__name) = ffly_arcs_alno()), __VA_ARGS__)

// delete record
# define ffly_arcs_delrec(__name) \
	ff_u64_t no = *ffly_arcs_alias(__name); \
	ffly_arc_delrec(__ffly_arccur__, ffly_arc_lookup(__ffly_arccur__, no)); \
	ffly_arcs_frno(no)

// create archive
# define ffly_arcs_creatarc(__name) \
	ffly_creatarc(__ffly_arccur__, (*ffly_arcs_alias(__name) = ffly_arcs_alno()))

// delete archive
# define ffly_arcs_delarc(__name) \
	ff_u64_t no = *ffly_arcs_alias(__name); \
	ffly_delarc((ffly_arcp)ffly_arc_lookup(__ffly_arccur__, no))->p; \
	ffly_arcs_frno(no)

// tunnel into archive
# define ffly_arcs_tun(__name) \
	__ffly_arccur__ = (ffly_arcp)ffly_arc_lookup(__ffly_arccur__, *ffly_arcs_alias(__name))->p

// go back - undo tunnel
# define ffly_arcs_bk(__name) \
	__ffly_arccur__ = __ffly_arccur__->bk;

enum {
	_ffly_rec_arc,
	_ffly_rec_def
};

typedef struct ffly_arc_rec {
	ff_u64_t no;
	ff_u8_t sort;
	ff_flag_t flags;
	struct ffly_arc_rec *fd, *bk;
	void *p;
} *ffly_arc_recp;

typedef struct ffly_arc {
	ffly_arc_recp *rr, p;
	struct ffly_arc *bk;
} *ffly_arcp;

ffly_arc_recp ffly_arc_lookup(ffly_arcp, ff_u64_t);
void ffly_arc_prepare(ffly_arcp);
void ffly_arc_free(ffly_arcp);

ffly_arc_recp ffly_arc_creatrec(ffly_arcp, ff_u64_t, void*, ff_u8_t, ff_flag_t);
void ffly_arc_delrec(ffly_arcp, ffly_arc_recp);

ffly_arcp ffly_creatarc(ffly_arcp, ff_u64_t);
void ffly_delarc(ffly_arcp);

void ffly_arc_recw(ffly_arc_recp, void*, ff_uint_t, ff_uint_t);
void ffly_arc_recr(ffly_arc_recp, void*, ff_uint_t, ff_uint_t);


ff_u64_t* ffly_arcs_alias(char const*);

void ffly_arcs_init();
void ffly_arcs_de_init();

ff_u64_t ffly_arcs_alno();
void ffly_arcs_frno(ff_u64_t);

extern struct ffly_arc __ffly_arcroot__;
ffly_arcp extern __ffly_arccur__;
# endif /*__ffly__arcs__h*/
