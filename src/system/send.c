# include "../ffint.h"
# include "../linux/socket.h"
# include "../ffly_def.h"
ff_s32_t send(ff_u32_t __fd, void *__buf, ff_u32_t __size, ff_u32_t __flags) {
	return sendto(__fd, __buf, __size, __flags, NULL, 0);
}
