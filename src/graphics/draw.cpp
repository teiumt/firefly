# include "draw.h"
extern "C" {
ffly_err_t ffly_pixdraw(ffly_off_t __x, ffly_off_t __y, ffly_byte_t *__buff, mdl_uint_t __bufw, ffly_byte_t *__pixels, mdl_uint_t __width, mdl_uint_t __height) {
	return mdl::firefly::graphics::gpu_pixdraw(__x, __y, __buff, __bufw, __pixels, __width, __height);
}
}
