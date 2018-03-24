# include "thread.h"
# include "io.h"
# include "../types/bool_t.h"
# include "../memory/mem_alloc.h"
# include "../memory/mem_realloc.h"
# include "../memory/mem_free.h"
# include "../types/off_t.h"
# include "mutex.h"
# include "atomic.h"
# include "../types/byte_t.h"
# include "../types/flag_t.h"
# include "cond_lock.h"
# define MAX_THREADS 20
# define PAGE_SIZE 4
# define UU_PAGE_SIZE 26
# define DSS 0xffff
# define TLS 0xff
# include "../ffly_system.h"
# ifndef __ffly_no_sysconf
#	include "config.h"
# endif
# define MAX_THREADS 20
typedef struct ffly_thread {
	ffly_cond_lock_t lock;
	__linux_pid_t pid;
	ffly_tid_t tid;
	ffly_bool_t alive;
	ffly_byte_t *sp, *tls;
	void*(*routine)(void*);
	void *arg_p;
} *ffly_threadp;

static struct ffly_thread **threads = NULL;
mdl_uint_t static page_c = 0;
ffly_off_t static off = 0;
ffly_atomic_uint_t active_threads = 0;

# include "../linux/signal.h"
# include "../linux/types.h"
# include "../linux/sched.h"
# include "../linux/unistd.h"
# include "../linux/wait.h"

struct {
	ffly_tid_t *p;
	mdl_uint_t page_c;
	ffly_off_t off;
} uu_ids = {.p = NULL, .page_c = 0, .off = 0};
ffly_mutex_t static mutex = FFLY_MUTEX_INIT;

// change this to macro
static struct ffly_thread *get_thr(ffly_tid_t __tid) {return *(threads+__tid);}

mdl_uint_t no_threads() {return off-uu_ids.off;}

static __thread ffly_tid_t id = FFLY_TID_NULL;
ffly_tid_t ffly_gettid() {
	return id;
}

__linux_pid_t ffly_thread_getpid(ffly_tid_t __tid) {
	return get_thr(__tid)->pid;
}

ffly_err_t static ffly_thread_del(ffly_tid_t __tid) {
	ffly_mutex_lock(&mutex);
	if (uu_ids.off >= uu_ids.page_c*UU_PAGE_SIZE) {
		if (!uu_ids.p) {
			if ((uu_ids.p = (ffly_tid_t*)__ffly_mem_alloc(((++uu_ids.page_c)*UU_PAGE_SIZE)*sizeof(ffly_tid_t))) == NULL) {
				ffly_fprintf(ffly_err, "thread: failed to allocate memory for unused thread ids to be stored.\n");
				return FFLY_FAILURE;
			}
		} else {
			if ((uu_ids.p = (ffly_tid_t*)__ffly_mem_realloc(uu_ids.p, ((++uu_ids.page_c)*UU_PAGE_SIZE)*sizeof(ffly_id_t))) == NULL) {
				ffly_fprintf(ffly_err, "thread: failed to realloc memory for unused thread ids.\n");
				return FFLY_FAILURE;
			}
		}
	}

	*(uu_ids.p+(uu_ids.off++)) = __tid;
	ffly_mutex_unlock(&mutex);
	return FFLY_SUCCESS;
}

void static thread_free(ffly_tid_t __tid) {
	struct ffly_thread *thr = get_thr(__tid);
	__ffly_mem_free(thr->sp);
	thr->sp = NULL;
}

ffly_bool_t ffly_thread_alive(ffly_tid_t __tid) {
	return get_thr(__tid)->alive;
}

ffly_bool_t ffly_thread_dead(ffly_tid_t __tid) {
	return !get_thr(__tid)->alive;
}

# include "../tools/printbin.c"
void static
ffly_thr_proxy() {
	void *arg_p;
	__asm__("movq -8(%%rbp), %%rdi\n\t"
			"movq %%rdi, %0": "=m"(arg_p) : : "rdi");
	ffly_threadp thr = (ffly_threadp)arg_p;

	ff_setpid();
	thr->pid = ff_getpid();
	id = thr->tid;
	ffly_fprintf(ffly_out, "pid: %ld, tid: %lu\n", thr->pid, thr->tid);
	ffly_atomic_incr(&active_threads);
	thr->alive = 1;

	ffly_cond_lock_signal(&thr->lock);

	thr->routine(thr->arg_p);
	//ffly_thread_del(thr->tid);

	ffly_atomic_decr(&active_threads);
	thr->alive = 0;
	ffly_araxe();
	exit(SIGKILL);
}

ffly_err_t ffly_thread_kill(ffly_tid_t __tid) {
	struct ffly_thread *thr = get_thr(__tid);
	if (kill(thr->pid, SIGKILL) == -1) {
		ffly_fprintf(ffly_err, "thread, failed to kill..\n");
		return FFLY_FAILURE;
	}

	return FFLY_SUCCESS;
}

void ffly_thread_wait(ffly_tid_t __tid) {
//	  while(get_thr(__tid)->alive);
	wait4(get_thr(__tid)->pid, NULL, __WALL|__WCLONE, NULL);
}

ffly_err_t ffly_thread_create(ffly_tid_t *__tid, void*(*__p)(void*), void *__arg_p) {
# ifndef __ffly_no_sysconf
	if (no_threads() == __ffly_sysconf__.max_threads) {
		ffly_fprintf(ffly_log, "thread: only %u threads are allowed.\n", __ffly_sysconf__.max_threads);
# else
	if (no_threads() == MAX_THREADS) {
		ffly_fprintf(ffly_log, "thread: only %u threads are allowed.\n", MAX_THREADS);
# endif
		return FFLY_FAILURE;
	}

	ffly_mutex_lock(&mutex);
	mdl_u8_t reused_id = 0;
	if ((reused_id = (uu_ids.off>0))) {
		*__tid = *(uu_ids.p+(--uu_ids.off));
		if (uu_ids.off < ((uu_ids.page_c-1)*UU_PAGE_SIZE) && uu_ids.page_c > 1) {
			if ((uu_ids.p = (ffly_tid_t*)__ffly_mem_realloc(uu_ids.p,
				((--uu_ids.page_c)*UU_PAGE_SIZE)*sizeof(ffly_id_t))) == NULL)
			{
				ffly_fprintf(ffly_err, "thread: failed to realloc memory space for unused thread ids.\n");
				return FFLY_FAILURE;
			}
		}
		ffly_mutex_unlock(&mutex);
		goto _exec;
	}

	ffly_printf("thread id: %u\n", *__tid = off++);

	ffly_mutex_unlock(&mutex);
	goto _alloc;

	_exec:
	{
	ffly_printf("stage, exec.\n");

	ffly_threadp thr = get_thr(*__tid);
	thr->alive = 0;
	thr->lock = FFLY_COND_LOCK_INIT;
	if (reused_id) {
		//if (thr->sp == NULL)
		//	thr->sp = (ffly_byte_t*)__ffly_mem_alloc(DSS);
		thr->arg_p = __arg_p;
		thr->routine = __p;
	} else {
		*thr = (struct ffly_thread) {
			.tid = *__tid,
			.sp = (ffly_byte_t*)__ffly_mem_alloc(DSS),
			.tls = (ffly_byte_t*)__ffly_mem_alloc(TLS),
			.arg_p = __arg_p,
			.routine = __p
		};
	}

	*(void**)(thr->sp+(DSS-8)) = (void*)ffly_thr_proxy;
	*(void**)(thr->sp+(DSS-16)) = (void*)thr;

	__linux_pid_t pid;
	if ((pid = clone(CLONE_VM|CLONE_SIGHAND|CLONE_FILES|CLONE_FS|SIGCHLD|CLONE_SETTLS,
		(mdl_u64_t)(thr->sp+(DSS-8)), NULL, NULL, (mdl_u64_t)(thr->tls+TLS))) == (__linux_pid_t)-1)
	{
		ffly_fprintf(ffly_err, "thread: failed.\n");
		return FFLY_FAILURE;
	}

	ffly_cond_lock_wait(&thr->lock);
	}
	return FFLY_SUCCESS;

	_alloc:
	ffly_printf("stage, alloc.\n");
	ffly_mutex_lock(&mutex);
	if (off > page_c*PAGE_SIZE) {
		if (!threads) {
			page_c++;
			if ((threads = (ffly_threadp*)__ffly_mem_alloc(PAGE_SIZE*sizeof(ffly_threadp))) == NULL) {
				ffly_fprintf(ffly_err, "thread: failed to alloc memory, errno{%d}, ffly_errno{%u}\n", 0, ffly_errno);
				goto _err;
			}
		} else {
			if ((threads = (ffly_threadp*)__ffly_mem_realloc(threads,
				((++page_c)*PAGE_SIZE)*sizeof(ffly_threadp))) == NULL)
			{
				ffly_fprintf(ffly_err, "thread: failed to realloc memory, errno{%d}, ffly_errno{%u}\n", 0, ffly_errno);
				goto _err;
			}
		}

		ffly_threadp *beg = threads+((page_c-1)*PAGE_SIZE);
		ffly_threadp *itr = beg, *end = beg+PAGE_SIZE;
		while(itr != end)
			*(itr++) = NULL;
	}

	if (!*(threads+*__tid))
		*(threads+*__tid) = (ffly_threadp)__ffly_mem_alloc(sizeof(struct ffly_thread));
	
	ffly_mutex_unlock(&mutex);
	goto _exec;
	_err:
	ffly_mutex_unlock(&mutex);
	return FFLY_FAILURE;
}

ffly_err_t ffly_thread_cleanup() {
	while(active_threads != 0);
	ffly_threadp *itr = threads;
	ffly_threadp *end = threads+off;
	ffly_printf("thread shutdown, stage 0.\n");
	while(itr != end) {
		if ((*itr)->alive)
			ffly_thread_kill((*itr)->tid);
		ffly_thread_wait((*itr)->tid);
# ifdef __ffly_use_allocr
		if ((*itr)->sp != NULL)
			__ffly_mem_free((*itr)->sp);
# endif
		__ffly_mem_free(*(itr++));
	}

	ffly_printf("thread shutdown, stage 1.\n");
	if (threads != NULL) {
		__ffly_mem_free(threads);
		threads = NULL;
	}

	ffly_printf("thread shutdown, stage 2.\n");
	if (uu_ids.p != NULL) {
		__ffly_mem_free(uu_ids.p);
		uu_ids.p = NULL;
	}
	uu_ids.off = uu_ids.page_c = 0;
	off = page_c = 0;
}
