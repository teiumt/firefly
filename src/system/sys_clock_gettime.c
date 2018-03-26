# include <mdlint.h>
# include "../linux/time.h"
mdl_s32_t clock_gettime(__linux_clockid_t __clock, struct timespec *__tp) {
	mdl_s32_t ret;
	__asm__("xorq %%rdi, %%rdi\n\t"
			"mov %1, %%edi\n\t"
			"mov %2, %%rsi\n\t"
			"call __clock_gettime\n\t"
			"mov %%eax, %0" : "=m"(ret) : "m"(__clock), "m"(__tp) : "rdi", "rsi", "rax");
	return ret;
}