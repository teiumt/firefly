section .text
global ffly_round
ffly_round:
	push rbp
	mov rbp,rsp
	sub rsp,16

	movsd [rbp-8],xmm0
	pxor xmm0,xmm0

	fld qword[rbp-8]
	fstcw word[rbp-10]
	fclex

	mov word[rbp-12],867
	frndint
	fclex

	fldcw word[rbp-10]
	fstp qword[rbp-16]

	movsd xmm0,[rbp-16]

	mov rsp,rbp
	pop rbp
	ret