# include "bond.h"
# include "../types/err_t.h"
# include "../stdio.h"
# include "../malloc.h"
# include "../system/err.h"
# include "../opt.h"
ffly_err_t ffmain(int __argc, char const *__argv[]) {
	ffoe_prep();
	char const *s = NULL;
	char const *d = NULL;

	struct ffpcll pcl;
	pcl.cur = __argv+1;
	pcl.end = __argv+__argc;
	ffoe(ffoe_pcll, (void*)&pcl);

	s = ffoptval(ffoe_get("s"));
	d = ffoptval(ffoe_get("d"));

	ffoe_end();

	if (!s || !d) {
		printf("please provide source and dest.\n");
		reterr;
	}

	printf("src: %s, dst: %s\n", s, d);

		


	// for now
	free(s);
	free(d);
	retok;
}