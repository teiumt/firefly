# ifndef __aformat__h
# define __aformat__h
# ifdef __cplusplus
# include <eint_t.hpp>
namespace mdl {
namespace firefly {
namespace system {
enum aformat : u16_t {
	S16_LE,
	FLOAT32_LE
};
}
}
}
# endif
# endif /*__aformat__h*/