# include "brew.h"
# include "../linux/unistd.h"
# include "../linux/fcntl.h"
# include "../linux/types.h"
# include "../linux/stat.h"
# include "../malloc.h"
# include "../stdio.h"
# include "../types.h"
mdl_u8_t *p = NULL;
mdl_u8_t static *eof;
mdl_u8_t at_eof() {
	return (p >= eof || *p == '\0');
}
ffly_err_t ffmain(int __argc, char const *__argv[]) {
	char const *file;
	if (__argc < 2) {
		fprintf(stderr, "please provide file.\n");
		return -1;
	}

	file = __argv[1];
	mdl_s32_t fd;
	mdl_u8_t *base;
	if ((fd = open(file, O_RDONLY, 0)) == -1) {
		fprintf(stderr, "failed to open file.\n");
		return -1;
	}

	struct stat st;
	if (fstat(fd, &st) == -1) {
		fprintf(stderr, "failed to stat file.\n");
		goto _fault;	
	}

	if ((p = (mdl_u8_t*)malloc(200)) == NULL) {
		fprintf(stderr, "failed to allocate memory.\n");
		goto _fault;
	}	

	base = p;
	read(fd, p, st.st_size);
	
	eof = p+st.st_size;
	bucketp top = NULL;
	printf("filesize: %u\n", st.st_size);
	parse(&top);

	lexer_cleanup();

	_fault:
	close(fd);
	if (base != NULL)
		free(base);
	return 0;
}