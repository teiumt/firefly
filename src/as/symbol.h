# ifndef __ffly__as__symbol__h
# define __ffly__as__symbol__h
# include <mdlint.h>
# define SY_STR 0x1
# define SY_MAC 0x2
# define SY_CHR 0x3
# define SY_INT 0x4
# define SY_LABEL 0x5
# define SY_DIR 0x6
# define SY_REG 0x7

# define is_syreg(__st) \
	((__sy)->sort==SY_REG)
# define is_symac(__sy) \
	((__sy)->sort==SY_MAC)
# define is_systr(__sy) \
    ((__sy)->sort==SY_STR)
# define is_sychr(__sy) \
    ((__sy)->sort==SY_CHR)
# define is_syint(__sy) \
    ((__sy)->sort==SY_INT)
# define is_sylabel(__sy) \
    ((__sy)->sort==SY_LABEL)
# define is_sydir(__sy) \
    ((__sy)->sort==SY_DIR)

typedef struct symbol {
	void *p;
	
	mdl_u8_t len:6;
	mdl_u8_t flags:4;
	mdl_u8_t sign:1;
	mdl_u8_t sort:4;

	struct symbol *next;
} *symbolp;


struct hash extern symbols;
void putsymbol(symbolp);
symbolp getsymbol(char const*);
symbolp syt(char const*);
void syt_drop();
# endif /*__ffly__as__symbol__h*/
