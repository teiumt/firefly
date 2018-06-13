# include "types.h"
# include "linux/unistd.h"

# include "memory/allocr.h"
# include "system/io.h"
# include "system/thread.h"
# include "system/config.h"
# include "arcs.h"
# include "memory/alloca.h"
# include "dep/str_cmp.h"
# include "dep/mem_dup.h"
# include "dep/mem_cpy.h"
# include "dep/mem_cmp.h"
# include "memory/mem_alloc.h"
# include "memory/mem_free.h"
# include "hatch.h"
# include "version.h"
# include "system/util/ff6.h"
# include "system/error.h"
/*
extern void*(*ffly_allocp)(ff_uint_t);
extern void(*ffly_freep)(void*);
extern void*(*ffly_reallocp)(void*, ff_uint_t);
*/

typedef struct _ffopt {
	char const *name, *val;
	struct _ffopt *next;
} ffopt, *ffoptp;

# define ffopt_size sizeof(struct _ffopt)

ffopt static *optbed = NULL;
// rename
void evalopts(char const *__s) {
	char const *p = __s;
	char buf[128];
	char *bufp;
	ffoptp opt, end = NULL;
_again:
	bufp = buf;
	opt = (ffoptp)__ffly_mem_alloc(ffopt_size);
	if (!optbed) 
		optbed = opt;
	if (end != NULL)
		end->next = opt;
	end = opt;
	while(*p == ' ') p++;
	while((*p >= 'a' && *p <= 'z') || *p == '-')
		*(bufp++) = *(p++);
	*bufp = '\0';
	ffly_mem_dup((void**)&opt->name, buf, (bufp-buf)+1);
	bufp = buf;

	if (*p == ':') {
		p++;
		while(*p == ' ') p++;
		while(*p != ',' && *p != '\0')
			*(bufp++) = *(p++);
		*bufp = '\0';
		ffly_mem_dup((void**)&opt->val, buf, (bufp-buf)+1);
		if (*p == ',') {
			p++;
			goto _again;
		}
	} else
		opt->val = NULL;
	end->next = NULL;
}

/*
	alloca will use this until config is loaded.
*/
# define TMP_ALSSIZE 500

# include "mode.h"
# include "system/config.h"
ff_err_t ffmain(int, char const**);
char const static *by = "mrdoomlittle"; 
# include "dep/str_cpy.h"
# include "mod.h"

# ifdef __ffly_mal_track
# include "system/mal_track.h"
# endif

# include "system/tls.h"
# include "piston.h"
# include "corrode.h"
# include "location.h"
ff_i8_t static
check_conf_version(void) {
	ff_i8_t ret;
	char p[] = ffly_version;
	if ((ret = ffly_mem_cmp(__ffly_sysconf__.version, p, ffly_version_len))<0) {
		ffly_printf("sysconf out of date.\n");
		ff_u64_t a;
		ffly_ff6_dec(__ffly_sysconf__.version, &a, ffly_version_len);
		if (a>ffly_version_int) {
			ffly_printf("config version too in date.\n");
		} else if (a<ffly_version_int) {
			ffly_printf("config version out of date.\n");
		}
	}
	return ret;
}

void ffly_string_init(void);
ff_err_t static
init() {
	ff_err_t err;

	err = ffly_tls_new();
	if (_err(err)) {
		_ret;
	}

	err = ffly_ar_init();
	if (_err(err)) {
		_ret;
	}

	ffly_string_init();
	ff_location_init();
# ifndef __ffly_crucial
# ifdef __ffly_mal_track
	ffly_mal_track_init(&__ffly_mal_track__);
# endif
# endif
	ffly_io_init();
# ifndef __ffly_crucial
	ffly_arcs_init();
# endif
	ffly_thread_init();
}

# include "init.h"

void ffly_bog_start(void);
void ffly_bog_stop(void);
void static
prep() {
# ifndef __ffly_crucial
	void **p = ffly_alloca(sizeof(void*), NULL);
	*p = (void*)by;
	ffly_arcs_creatarc("info");
	ffly_arcs_tun("info");
	ffly_arcs_creatrec("created-by", NULL, _ffly_rec_def, 0);
	ffly_arcs_bk();
	ffly_init_run();

	/* pistons are on even if sched has not been inited
	*/
	ffly_bog_start();
	ffly_piston();
# endif
//	ff_mod_init();
//	ff_mod_handle();
//	ffly_mod();
}

# include "linux/stat.h"
# include "linux/fcntl.h"
# include "system/string.h"
void static
fini() {
# ifndef __ffly_crucial
	ffly_corrode_start();
	ffly_pistons_stall();
	ffly_bog_stop();
# endif
//	ff_mod_de_init();
# ifndef __ffly_crucial
	ffly_arcs_de_init();
# endif
	ffly_alloca_cleanup();
	ffly_thread_cleanup();
//	pr();
//	pf();
# ifndef __ffly_crucial
# ifdef __ffly_mal_track
	ffly_mal_track_dump(&__ffly_mal_track__);
# endif
# endif

	ffly_io_closeup();

# ifndef __ffly_crucial
# ifdef __ffly_mal_track
	ffly_mal_track_de_init(&__ffly_mal_track__);
# endif
# endif
# ifdef __ffly_debug
	/*
		if any leaks then say so.
	*/
	ff_uint_t leak;
	if ((leak = (ffly_mem_alloc_bc-ffly_mem_free_bc))>0) {
		int fd;
		fd = open("/dev/tty", O_WRONLY, 0);
		char buf[512];
		char *p = buf;
		p+=ffly_str_cpy(p, "memory leakage, ");
		p+=ffly_nots(leak, p);
		p+=ffly_str_cpy(p, "-bytes.");
		*p = '\n';
		write(fd, buf, (p-buf)+1);
		close(fd);
	}
# endif
	ffly_ar_cleanup();
	ffly_tls_destroy();
}

# include "env.h"
void _start(void) {
	init();
	int long argc;
	char const **argv;
	char const **envp;
	__asm__("mov 8(%%rbp), %0\t\n"
			"mov %%rbp, %%rax\n\t"
			"add $16, %%rax\n\t"
			"mov %%rax, %1\n\t"
			"add $16, %%rax\n\t"
			"mov %%rax, %2" : "=r"(argc), "=r"(argv), "=r"(envp) : : "rax");
	char const **argp = argv;
	char const **end = argp+argc;

	envinit();
	envload(envp);
# ifndef __ffly_crucial
	void *frame;
	void *tmp;

	tmp = __ffly_mem_alloc(TMP_ALSSIZE);
	ffly_alss(tmp, TMP_ALSSIZE);

	frame = ffly_frame();
	char const **argl = ffly_alloca(argc*sizeof(char const*), NULL);
	char const **arg = argl;
	*(arg++) = *(argp++); 

	/*
		move to its own function
	*/
	ff_i8_t conf = -1;
	ff_i8_t hatch = -1;
	if (argc > 1) {
		if (!ffly_str_cmp(*argp, "-proc")) {
			evalopts(*(++argp));
			argp++;
			ffly_trim(2*sizeof(char const*)); // alloca stack trim
		}

		ffoptp cur = optbed;
		while(cur != NULL) {
			ffly_printf("name: '%s', val: '%s'\n", cur->name, !cur->val?"null":cur->val);
			if (!ffly_str_cmp(cur->name, "-sysconf")) {
				if (!cur->val) {
					ffly_printf("error.\n");
				}
				ffly_printf("loaded sysconfig.\n");
				ffly_ld_sysconf(cur->val);
				check_conf_version();
				conf = 0;
			} else if (!ffly_str_cmp(cur->name, "-mode")) {
				if (!ffly_str_cmp(cur->val, "debug"))
					ffset_mode(_ff_mod_debug);	
			} else if (!ffly_str_cmp(cur->name, "-hatch")) {
				if (!ffly_str_cmp(cur->val, "enable")) {
					ffly_printf("hatch enabled.\n");
					hatch = 0;
				}
			}
			cur = cur->next;
		}
		//ffly_printf("please provide sysconf.\n");
		//goto _end;
	}

	if (conf<0)
		// load default builtin config if no file was provided
		ffly_ld_sysconf_def();
	/*
		alloca is giving pointer from the temp stack,
		so add them to the amend list for later.
	*/
	ffly_alad((void**)&argl);
	ffly_alad((void**)&arg);
	ffly_alad(&frame);
	// reload
	ffly_alrr();

	__ffly_mem_free(tmp);
	while(argp != end)
		*(arg++) = *(argp++);

	prep();

	if (!hatch)
		ffly_hatch_start();

	ffmain(arg-argl, argl);
# else
	prep();
	ffmain(argc, argv);
# endif
# ifndef __ffly_crucial
	ffly_printf("\n\n");

	if (!hatch) {
		ffly_hatch_shutoff();
		ffly_hatch_wait();
	}

	if (!conf)
		ffly_free_sysconf();
_end:
	{
		ffoptp cur = optbed, bk;
		while(cur != NULL) {
			bk = cur;
			cur = cur->next;
			__ffly_mem_free((void*)bk->name);
			if (bk->val != NULL)
				__ffly_mem_free((void*)bk->val);
			__ffly_mem_free(bk);
		}
	}
# ifdef __ffly_debug
	__ffmod_debug
		ffly_arstat();
# endif
	ffly_collapse(frame);
# endif // __ffly_crucial
//	pr();
//	pf();
	envcleanup();
	fini();
	exit(0);
}
