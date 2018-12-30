# ifndef __ffly__nt__raise__h
# define __ffly__nt__raise__h
# include "../ffint.h"
# define STACK_SIZE 2048
#define _nt_op_raster_tri2		0x00
#define _nt_op_ctx_new			0x01
#define _nt_op_ctx_destroy		0x02
#define _nt_op_putframe			0x03
#define _nt_op_setctx			0x04
#define _nt_op_start			0x05
#define _nt_op_finish			0x06
#define _nt_op_pixcopy			0x07
#define _nt_op_pixdraw			0x08
#define _nt_op_pixfill			0x09
#define _nt_op_fb_set			0x0a
#define _nt_op_fb_new			0x0b
#define _nt_op_fb_destroy		0x0c
#define _nt_op_ptile_new		0x0d
#define _nt_op_ptile_destroy	0x0e
#define _nt_op_tdraw			0x0f
#define _nt_op_sput				0x10
#define _nt_op_sget				0x11
#define _nt_op_sb				0x12
#define _nt_op_cb				0x13
#define _nt_op_obn				0x14
#define _nt_op_obd				0x15
#define _nt_op_obm				0x16
#define _nt_op_obum				0x17
#define _nt_op_obw				0x18
#define _nt_op_obr				0x19
#define _nt_op_draw				0x1a
#define _nt_op_txbn				0x1b
#define _nt_op_txbd				0x1c
#define _nt_op_txbm				0x1d
#define _nt_op_txbum			0x1e
#define _nt_op_txbw				0x1f
#define _nt_op_txbr				0x20
#define	_nt_op_txn				0x21
#define _nt_op_txd				0x22
#define stack_at(__adr) \
	(nt_raise_stack+(__adr))
extern ff_u8_t *nt_raise_p;
extern ff_u8_t nt_raise_stack[STACK_SIZE];
extern ff_u16_t nt_raise_sp;
void nt_raise(ff_u8_t*, ff_uint_t);
# endif /*__ffly__nt__raise__h*/
