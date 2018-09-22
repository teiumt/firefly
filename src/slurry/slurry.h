# ifndef __ffly__slurry__h
# define __ffly__slurry__h
# include "../ffint.h"
# include "window.h"
# include "../graphics/window.h"
struct s_window {
	struct s_window *next;
	struct ffly_wd fw;
};

typedef struct s_tape {
	void *text;
	ff_uint_t len;
} *s_tapep;

s_tapep s_tape_new(ff_uint_t);
void s_tape_destroy(s_tapep);
void sse_open(void);
void sse_run(void);
void sse_close(void);
# endif /*__ffly__slurry__h*/
