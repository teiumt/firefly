# ifndef __ffly__slurry__connection__h
# define __ffly__slurry__connection__h
# include "../ffint.h"
# ifndef __fflib
# include <netinet/in.h>
# else
# include "../linux/in.h"
# endif
typedef struct s_conn {
	int sock;
	struct sockaddr_in addr;
} *s_connp;

s_connp s_open(void);
void s_connect(s_connp, ff_u16_t, char const*);
void s_close(s_connp);
void s_test(void);
ff_u16_t s_window_new(s_connp);
void s_read(s_connp, ff_u16_t, ff_uint_t, void*);
void s_write(s_connp, ff_u16_t, ff_uint_t, void*);
void s_window_init(s_connp, ff_u16_t, ff_u16_t, ff_u16_t, ff_u16_t);
void s_window_open(s_connp, ff_u16_t);
void s_window_close(s_connp, ff_u16_t);
void s_window_destroy(s_connp, ff_u16_t);
void s_window_display(s_connp, ff_u16_t);
void s_window_frame(s_connp, ff_u16_t, ff_u8_t*, ff_uint_t, ff_uint_t);
void s_disconnect(s_connp);
# endif /*__ffly__slurry__connection__h*/
