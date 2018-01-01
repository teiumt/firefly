# include "chunk_manager.h"
# include "memory/mem_alloc.h"
# include "memory/mem_free.h"
# include "memory/mem_realloc.h"
# include "system/errno.h"
# include "system/io.h"
ffly_err_t ffly_cnk_man_init(struct ffly_cnk_man *__cnk_man, mdl_uint_t __xal, mdl_uint_t __yal, mdl_uint_t __zal) {
    __cnk_man->xal = __xal;
    __cnk_man->yal = __yal;
    __cnk_man->zal = __zal;
    __cnk_man->chunks = NULL;
    __cnk_man->off = 0;
    __cnk_man->no_chunks = 0;
}

ffly_err_t ffly_cnk_man_de_init(struct ffly_cnk_man *__cnk_man) {
    struct ffly_chunk **itr = __cnk_man->chunks;
    while(itr != __cnk_man->chunks+__cnk_man->off) {
        if (*itr != NULL)
            ffly_free_chunk(*itr);
        itr++;
    }

    if (__cnk_man->chunks != NULL)
        __ffly_mem_free(__cnk_man->chunks);
}

ffly_err_t ffly_cnk_man_create(struct ffly_cnk_man *__cnk_man, ffly_id_t *__id) {
    ffly_fprintf(ffly_log, "creating chunk, %u\n", __cnk_man->no_chunks);
    if (!__cnk_man->chunks) {
        __cnk_man->chunks = (struct ffly_chunk**)__ffly_mem_alloc(sizeof(struct ffly_chunk*));
        __cnk_man->no_chunks++;
    } else
        __cnk_man->chunks = (struct ffly_chunk**)__ffly_mem_realloc(__cnk_man->chunks, (++__cnk_man->no_chunks)*sizeof(struct ffly_chunk*));

    struct ffly_chunk *chunk = (*(__cnk_man->chunks+__cnk_man->off) = ffly_alloc_chunk(__cnk_man->xal, __cnk_man->yal, __cnk_man->zal));
    *__id = (ffly_id_t)__ffly_mem_alloc(sizeof(__ffly_id_t)); 
    **__id = __cnk_man->off++;

    return FFLY_SUCCESS;
}

struct ffly_chunk* ffly_cnk_man_fetch(struct ffly_cnk_man *__cnk_man, ffly_id_t __id) {
    return *(__cnk_man->chunks+(*__id));
}

ffly_err_t ffly_cnk_man_del(struct ffly_cnk_man *__cnk_man, ffly_id_t __id) {

}