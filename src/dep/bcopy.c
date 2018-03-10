# include <mdlint.h>
void ffly_bcopy(void *__dst, void const *__src, mdl_u32_t __bc) {
	__asm__("mov %0, %%rdi\n\t"
			"mov %1, %%rsi\n\t"
			"xorq %%rbx, %%rbx\n\t"
			"mov %2, %%ebx\n\t"
			"call __ffly_bcopy" : : "m"(__dst), "m"(__src), "m"(__bc) : "rdi", "rsi", "rbx");
}