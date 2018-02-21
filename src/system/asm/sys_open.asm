%include "syscall.mac"
%include "err.mac"
extern set_errno
global __open
section .text
__open:
    mov rax, sys_open
    syscall
	cmp rax, -MAX_ERRNO
	jae _fault
    ret
	_fault:
	call set_errno
	mov rax, -1
	ret
