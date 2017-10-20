# ifndef __ffly__audio__h
# define __ffly__audio__h
# include <mdlint.h>
# include "types/err_t.h"
# include "types/aud_fformat_t.h"
# include "types/aud_format_t.h"
# include "types/audio/wav_spec_t.h"
# include "types/aud_spec_t.h"
# include "types/byte_t.h"
# include "types/size_t.h"
# include "audio/pulse.h"
# include "audio/alsa.h"
# include "data/pair.h"

# ifdef __cplusplus
extern "C" {
# endif
ffly_err_t ffly_audio_init(ffly_aud_spec_t*);
ffly_err_t ffly_audio_de_init();
ffly_err_t ffly_ld_aud_file(char*, char*, ffly_aud_fformat_t, ffly_byte_t**, ffly_size_t*);
ffly_wav_spec_t ffly_extract_wav_spec(ffly_byte_t*);
ffly_err_t ffly_aud_play(ffly_byte_t*, ffly_size_t, ffly_aud_fformat_t);
ffly_err_t ffly_aud_raw_play(ffly_byte_t*, ffly_size_t, ffly_aud_spec_t*);

ffly_err_t ffly_aud_write(ffly_byte_t*, ffly_size_t);
ffly_err_t ffly_aud_drain();
ffly_err_t ffly_aud_read(ffly_byte_t*, ffly_size_t);
# ifdef __cplusplus
}
# endif
# endif /*__ffly__audio__h*/
