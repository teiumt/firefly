# include "image.h"
# include "../system/err.h"
# include "png.h"
# include "jpeg.h"
# include "../system/io.h"
# include "../system/err.h"
ffly_err_t ffly_ld_img(ffly_imagep __image, char *__dir, char *__name,  mdl_uint_t __format) {
	ffly_err_t err = FFLY_SUCCESS;
	switch(__format) {
		case _ffly_img_png:
			if (_err(err = ffly_ld_png_img(__dir, __name, __image))) {
				ffly_fprintf(ffly_err, "failed to load png image.\n");
				return err;
			}
		break;
		case _ffly_img_jpeg:
			if (_err(err = ffly_ld_jpeg_img(__dir, __name, __image))) {
				ffly_fprintf(ffly_err, "failed to load jpeg image.\n");
			}
		break;
	}
    if (_err(err))
        return err;
	return FFLY_SUCCESS;
}

# include "../memory/mem_free.h"
ffly_err_t ffly_free_img(ffly_imagep __image) {
    if (__image->pixels != NULL)
        __ffly_mem_free(__image->pixels);
}
