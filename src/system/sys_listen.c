# include "../ffint.h"
ff_s32_t listen(ff_u32_t __fd, ff_u32_t __backlog) {
	ff_s32_t ret;
	__asm__("xorq %%rdi, %%rdi\n\t"
			"mov %1, %%edi\n\t"
			"xorq %%rsi, %%rsi\n\t"
			"mov %2, %%esi\n\t"
			"call __listen\n\t"
			"mov %%eax, %0" : "=m"(ret) : "m"(__fd), "m"(__backlog) : "rdi", "rsi", "rax");
	return ret;
}
