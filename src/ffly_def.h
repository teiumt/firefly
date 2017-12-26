# ifndef __ffly__def__h
# define __ffly__def__h

# ifndef FF_ZERO
#	define FF_ZERO 0
# endif
# ifndef NULL
#	define NULL (void*)0
# endif

# ifndef ffly_null_id
#	define ffly_null_id NULL
# endif

# ifndef ffly_true
#	define ffly_true 1
# endif

# ifndef ffly_false
#	define ffly_false 0
# endif

# define force_inline __attribute__((always_inline)) inline
# endif /*__ffly__def__h*/
