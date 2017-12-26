# include "config.h"
# include "data/str_len.h"
# include "data/str_dupe.h"
# include "memory/mem_alloc.h"
# include "memory/mem_free.h"
# include "system/io.h"
# include "system/file.h"
# include "system/string.h"
enum {
    _tok_ident,
    _tok_keyword,
    _tok_str,
    _tok_no,
    _tok_chr
};

enum {
    _colon,
    _comma,
    _l_bracket,
    _r_bracket
};

ffly_bool_t static is_space(char __c) {
    return (__c == ' ' || __c == '\n');
}

void const static* ffly_conf_get(struct ffly_conf *__conf, char *__name) {
    return ffly_map_get(&__conf->env, (mdl_u8_t*)__name, ffly_str_len(__name));
}

void static* ffly_conf_get_arr_elem(struct ffly_conf *__conf, void *__p, mdl_uint_t __no) {
    return *(void**)ffly_vec_at(&((struct ffly_conf_arr*)__p)->data, __no);
}

struct token {
    mdl_u8_t kind, id;
    void *p;
};

struct token static* read_token(struct ffly_conf*, ffly_err_t*);
struct token static* peek_token(struct ffly_conf*, ffly_err_t*);
ffly_bool_t static expect_token(struct ffly_conf *__conf, mdl_u8_t __kind, mdl_u8_t __id, ffly_err_t *__err) {
    struct token *tok = read_token(__conf, __err);
    if (_err(*__err)) return 0;
    mdl_u8_t ret_val;
    if (!(ret_val = (tok->kind == __kind && tok->id == __id))) {
        if (tok->kind == __kind) {
            switch(__id) {
                case _colon:
                    ffly_fprintf(ffly_out, "expected colon.\n");
                break;
                case _comma:
                    ffly_fprintf(ffly_out, "expected comma.\n");
                break;
                case _l_bracket:
                    ffly_fprintf(ffly_out, "expected left side bracket.\n");
                break;
                case _r_bracket:
                    ffly_fprintf(ffly_out, "expected right side bracket.\n");
                break;
            }
        }
    }
    *__err = FFLY_SUCCESS;
    return ret_val;
}

ffly_err_t static push_free(struct ffly_conf *__conf, void *__p) {
    void **p;
    ffly_vec_push_back(&__conf->free, (void**)&p);
    *p = __p;
}

ffly_err_t static push_arr(struct ffly_conf *__conf, struct ffly_conf_arr *__arr) {
    struct ffly_conf_arr **p;
    ffly_vec_push_back(&__conf->arrs, (void**)&p);
    *p = __arr;
}

ffly_bool_t static is_eof(struct ffly_conf *__conf) {
    return (__conf->p+__conf->off) >= __conf->end;
}

void static mk_ident(struct token *__tok, char *__s) {
    *__tok = (struct token){.kind=_tok_ident, .p=(void*)__s};
}
void static mk_keyword(struct token *__tok, mdl_u8_t __id) {
    *__tok = (struct token){.kind=_tok_keyword, .id=__id, .p=NULL};
}
void static mk_str(struct token *__tok, char *__s) {
    *__tok = (struct token){.kind=_tok_str, .p=(void*)__s};
}
void static mk_no(struct token *__tok, char *__s) {
    *__tok = (struct token){.kind=_tok_no, .p=(void*)__s};
}
void static mk_chr(struct token *__tok, char *__s) {
    *__tok = (struct token){.kind=_tok_chr, .p=(void*)__s};
}

char static* read_ident(struct ffly_conf *__conf) {
    char *itr = (char*)(__conf->p+__conf->off);
    while((*itr >= 'a' && *itr <= 'z') || *itr == '_') {
        ffly_buff_put(&__conf->sbuf, itr++);
        ffly_buff_incr(&__conf->sbuf);
    }

    char end = '\0';
    ffly_buff_put(&__conf->sbuf, &end);
    char *s = ffly_str_dupe((char*)ffly_buff_begin(&__conf->sbuf));
    __conf->off+= ffly_buff_off(&__conf->sbuf);
    ffly_buff_off_reset(&__conf->sbuf);
    return s;
}

char static* read_no(struct ffly_conf *__conf) {
    char *itr = (char*)(__conf->p+__conf->off);
    while((*itr >= '0' && *itr <= '9') || *itr == '.') {
        ffly_buff_put(&__conf->sbuf, itr++);
        ffly_buff_incr(&__conf->sbuf);
    }

    char end = '\0';
    ffly_buff_put(&__conf->sbuf, &end);
    char *s = ffly_str_dupe((char*)ffly_buff_begin(&__conf->sbuf));
    __conf->off+= ffly_buff_off(&__conf->sbuf);
    ffly_buff_off_reset(&__conf->sbuf);
    return s;
}

char static* read_str(struct ffly_conf *__conf) {
    char *itr = (char*)(__conf->p+__conf->off);
    while(*itr != '"') {
        ffly_buff_put(&__conf->sbuf, itr++);
        ffly_buff_incr(&__conf->sbuf);
    }

    char end = '\0';
    ffly_buff_put(&__conf->sbuf, &end);
    char *s = ffly_str_dupe((char*)ffly_buff_begin(&__conf->sbuf));
    __conf->off+= ffly_buff_off(&__conf->sbuf);
    ffly_buff_off_reset(&__conf->sbuf);
    return s;
}

char static* read_chr(struct ffly_conf *__conf) {
    __conf->off++;
    return (char*)(__conf->p+(__conf->off-1));
}

ffly_err_t static uread_token(struct ffly_conf *__conf, struct token *__tok) {
    ffly_err_t err;
    ffly_buff_put(&__conf->iject_buff, (void*)&__tok);
    ffly_buff_incr(&__conf->iject_buff);
    return FFLY_SUCCESS;
}   

char static fetchc(struct ffly_conf *__conf) {
    return *(__conf->p+__conf->off);
}

struct token static* read_token(struct ffly_conf *__conf, ffly_err_t *__err) {
    ffly_err_t err;
    if (ffly_buff_off(&__conf->iject_buff)>0){
        struct token *tok;
        ffly_buff_decr(&__conf->iject_buff);
        ffly_buff_get(&__conf->iject_buff, (void*)&tok);
        return tok;
    }

    while(is_space(fetchc(__conf)) && !is_eof(__conf)) __conf->off++;
    struct token *tok = (struct token*)__ffly_mem_alloc(sizeof(struct token));
    switch(fetchc(__conf)) {
        case '"':
            __conf->off++;
            mk_str(tok, read_str(__conf));
            __conf->off++;
        break;
        case '\x27':
            __conf->off++;
            mk_chr(tok, read_chr(__conf));
            __conf->off++;
        break;
        case ':':
            mk_keyword(tok, _colon);
            __conf->off++;
        break;
        case ',':
            mk_keyword(tok, _comma);
            __conf->off++;
        break;
        case '[':
            mk_keyword(tok, _l_bracket);
            __conf->off++;
        break;
        case ']':
            mk_keyword(tok, _r_bracket);
            __conf->off++;
        break;
        default:
            if ((fetchc(__conf) >= 'a' && fetchc(__conf) <= 'z') || fetchc(__conf) == '_')
                mk_ident(tok, read_ident(__conf));
            else if (fetchc(__conf) >= '0' && fetchc(__conf) <= '9')
                mk_no(tok, read_no(__conf));
            else {
                *__err = FFLY_FAILURE;
                tok->p = NULL;
            }
    }

    struct token **p;
    ffly_vec_push_back(&__conf->toks, (void**)&p);
    *p = tok;
    *__err = FFLY_SUCCESS;
    return tok;
}

ffly_bool_t static next_token_is(struct ffly_conf *__conf, mdl_u8_t __kind, mdl_u8_t __id, ffly_err_t *__err) {
    struct token *tok = read_token(__conf, __err);
    if (tok->kind == __kind && tok->id == __id) return 1;
    *__err = uread_token(__conf, tok);
    *__err = FFLY_SUCCESS;
    return 0;
}

ffly_byte_t static* read_literal(struct ffly_conf *__conf, mdl_u8_t *__kind, ffly_err_t *__err) {
    struct token *tok = read_token(__conf, __err);
    if (tok->kind == _tok_no) {
        void *p;
        mdl_u64_t no = ffly_stno((char*)tok->p);
        if (no >= 0 && no <= (mdl_u8_t)~0) {
            *(mdl_u8_t*)(p = __ffly_mem_alloc(sizeof(mdl_u8_t))) = no;
            *__kind = _ffly_conf_vt_8l_int;
            return (ffly_byte_t*)p;
        } else if (no > (mdl_u8_t)~0 && no <= (mdl_u16_t)~0) {
            *(mdl_u16_t*)(p = __ffly_mem_alloc(sizeof(mdl_u16_t))) = no;
            *__kind = _ffly_conf_vt_16l_int;
            return (ffly_byte_t*)p;
        } else if (no > (mdl_u16_t)~0 && no <= (mdl_u32_t)~0) {
            *(mdl_u32_t*)(p = __ffly_mem_alloc(sizeof(mdl_u32_t))) = no;
            *__kind = _ffly_conf_vt_32l_int;
            return (ffly_byte_t*)p;
        } else if (no > (mdl_u32_t)~0 && no <= ~(mdl_u64_t)0) {
            *(mdl_u64_t*)(p = __ffly_mem_alloc(sizeof(mdl_u64_t))) = no;
            *__kind = _ffly_conf_vt_64l_int;
            return (ffly_byte_t*)p;
        }
    }

    if (tok->kind == _tok_str)
        *__kind = _ffly_conf_vt_str;
    else if (tok->kind == _tok_chr)
        *__kind = _ffly_conf_vt_chr;
    *__err = FFLY_SUCCESS;
    return (ffly_byte_t*)ffly_str_dupe((char*)tok->p); 
}

ffly_err_t static read_val(struct ffly_conf *__conf, struct ffly_conf_val *__val) {
    ffly_err_t err;
    __val->p = read_literal(__conf, &__val->kind, &err);
    push_free(__conf, __val->p);
    if (!__val->p) {
        return FFLY_FAILURE;
    }

    if (_err(err)) return err;
    return FFLY_SUCCESS;
}

ffly_err_t static read_arr(struct ffly_conf *__conf, struct ffly_vec *__data) {
    ffly_err_t err;
    ffly_vec_clear_flags(__data);
    ffly_vec_set_flags(__data, VEC_AUTO_RESIZE);
    ffly_vec_init(__data, sizeof(void*));
    _again:
    if (next_token_is(__conf, _tok_keyword, _l_bracket, &err)) {
        struct ffly_conf_arr **arr;
        ffly_vec_push_back(__data, (void**)&arr);

        *arr = (struct ffly_conf_arr*)__ffly_mem_alloc(sizeof(struct ffly_conf_arr));
        read_arr(__conf, &(*arr)->data);
        push_arr(__conf, *arr);
        goto _again;
    }

    struct token *tok = peek_token(__conf, &err);
    struct ffly_conf_val **val;
    ffly_vec_push_back(__data, (void**)&val);
    *val = (struct ffly_conf_val*)__ffly_mem_alloc(sizeof(struct ffly_conf_val));
    push_free(__conf, *val);
    if (_err(err = read_val(__conf, *val))) {
        ffly_fprintf(ffly_out, "failed to read value.\n");
    }

    if (next_token_is(__conf, _tok_keyword, _comma, &err)) {
        goto _again;
    }

    if (!next_token_is(__conf, _tok_keyword, _r_bracket, &err)) {
        return FFLY_FAILURE;
    }

    if (!expect_token(__conf, _tok_keyword, _comma, &err)) {
        return FFLY_FAILURE;
    }
    return FFLY_SUCCESS;
}

ffly_err_t static read_decl(struct ffly_conf *__conf) {
    ffly_err_t err;
    struct token *name = read_token(__conf, &err);
    if (_err(err)) return FFLY_FAILURE;

    if (!expect_token(__conf, _tok_keyword, _colon, &err)) {
        if (_err(err)) return err;
        return FFLY_FAILURE;
    }

    void *p;    
    if (next_token_is(__conf, _tok_keyword, _l_bracket, &err)) {
        if (_err(err)) return FFLY_FAILURE;
        struct ffly_conf_arr *arr = (struct ffly_conf_arr*)__ffly_mem_alloc(sizeof(struct ffly_conf_arr));
        err = read_arr(__conf, &arr->data);
        if (_err(err)) {
            __ffly_mem_free(arr);
            return FFLY_FAILURE;
        }

        arr->name = (char*)name->p;
        p = (void*)arr;
        push_arr(__conf, arr);
    } else {
        struct ffly_conf_val val;
        err = read_val(__conf, &val);
        if (_err(err)) return FFLY_FAILURE;

        struct ffly_conf_var *var = (struct ffly_conf_var*)__ffly_mem_alloc(sizeof(struct ffly_conf_var));
        *var = (struct ffly_conf_var) {
            .val = val,
            .name = (char*)name->p
        };

        p = (void*)var;
        push_free(__conf, p);
    }

    ffly_map_put(&__conf->env, (mdl_u8_t*)name->p, ffly_str_len((char*)name->p), p);
    __ffly_mem_free(name->p);

    if (!expect_token(__conf, _tok_keyword, _comma, &err)) {
        return FFLY_FAILURE;
    }
    return FFLY_SUCCESS;
}

ffly_err_t ffly_conf_init(struct ffly_conf *__conf) {
    ffly_err_t err;
    if (_err(err = ffly_buff_init(&__conf->sbuf, 100, 1))) {
        ffly_fprintf(ffly_err, "failed to init buff.\n");
        return err;
    }

    ffly_vec_clear_flags(&__conf->toks);
    ffly_vec_set_flags(&__conf->toks, VEC_AUTO_RESIZE);
    if (_err(err = ffly_vec_init(&__conf->toks, sizeof(struct token*)))) {
        ffly_fprintf(ffly_err, "failed to init vec.\n");
        return err;
    }

    if (_err(err = ffly_buff_init(&__conf->iject_buff, 8, sizeof(struct token*)))) {
        ffly_fprintf(ffly_err, "failed to init buff.\n");
        return err;
    }

    ffly_vec_clear_flags(&__conf->arrs);
    ffly_vec_set_flags(&__conf->arrs, VEC_AUTO_RESIZE);
    if (_err(err = ffly_vec_init(&__conf->arrs, sizeof(struct ffly_conf_arr*)))) {
        ffly_fprintf(ffly_err, "failed to init vec.\n");
        return err;
    }

    ffly_vec_clear_flags(&__conf->free);
    ffly_vec_set_flags(&__conf->free, VEC_AUTO_RESIZE);
    if (_err(err = ffly_vec_init(&__conf->free, sizeof(void*)))) {
        ffly_fprintf(ffly_err, "failed to init vec.\n");
        return err;
    }

    if (_err(err = ffly_map_init(&__conf->env))) {
        ffly_fprintf(ffly_err, "failed to init map.\n");
        return err;
    }
    return FFLY_SUCCESS;
}

struct token static* peek_token(struct ffly_conf *__conf, ffly_err_t *__err) {
    struct token *tok = read_token(__conf, __err);
    if (_err(*__err)) return NULL;
    uread_token(__conf, tok);
    return tok;
}

ffly_err_t ffly_conf_read(struct ffly_conf *__conf) {
    ffly_err_t err;
    struct token *tok;
    while(!is_eof(__conf)) {
        tok = peek_token(__conf, &err);
        if (_err(err)) return FFLY_FAILURE;
        if (tok->kind == _tok_ident)
            err = read_decl(__conf);
        else
            break;
        if (_err(err))
            return FFLY_FAILURE;
    }
    return FFLY_SUCCESS;
}

ffly_err_t ffly_conf_ld(struct ffly_conf *__conf, char *__file) {
    ffly_err_t err;
    FF_FILE *f = ffly_fopen(__file, FF_O_RDONLY, 0, &err);
    struct ffly_stat st;
    ffly_fstat(__file, &st);

    __conf->p = (ffly_byte_t*)__ffly_mem_alloc(st.size);
    ffly_fread(f, __conf->p, st.size);
    __conf->end = __conf->p+st.size;
    ffly_fclose(f);
    __conf->off = 0;
    return FFLY_SUCCESS; 
}

ffly_err_t ffly_conf_free(struct ffly_conf *__conf) {
    ffly_err_t err;
    if (_err(err = ffly_buff_de_init(&__conf->sbuf))) {
        ffly_fprintf(ffly_err, "failed to de-init buff.\n");
        return err;
    }

    if (ffly_vec_size(&__conf->toks)>0) {
        struct token *tok;
        struct token **itr = (struct token**)ffly_vec_begin(&__conf->toks);
        while(itr <= (struct token**)ffly_vec_end(&__conf->toks)) {
            tok = *(itr++);
            if (tok->p != NULL) {
                if ((mdl_u8_t*)tok->p >= __conf->end || (mdl_u8_t*)tok->p < __conf->p)
                    __ffly_mem_free(tok->p);
            }
            __ffly_mem_free(tok);
        }
    }

    if (_err(err = ffly_vec_de_init(&__conf->toks))) {
        ffly_fprintf(ffly_err, "failed to de-init vec.\n");
        return err;
    }

    if (_err(err = ffly_buff_de_init(&__conf->iject_buff))) {
        ffly_fprintf(ffly_err, "failed to de-init buff.\n");
        return err;
    }

    if (ffly_vec_size(&__conf->arrs)>0) {
        struct ffly_conf_arr **itr = (struct ffly_conf_arr**)ffly_vec_begin(&__conf->arrs);
        while(itr <= (struct ffly_conf_arr**)ffly_vec_end(&__conf->arrs)) {
            ffly_vec_de_init(&(*itr)->data);
            __ffly_mem_free(*itr);
            itr++;
        }
    }

    if (_err(err = ffly_vec_de_init(&__conf->arrs))) {
        ffly_fprintf(ffly_err, "failed to de-init vec.\n");
        return err;
    }

    if (ffly_vec_size(&__conf->free)>0) {
        void **itr = (void**)ffly_vec_begin(&__conf->free);
        while(itr <= (void**)ffly_vec_end(&__conf->free))
            __ffly_mem_free(*(itr++));
    }

    if (_err(err = ffly_vec_de_init(&__conf->free))) {
        ffly_fprintf(ffly_err, "failed to de-init vec.\n");
        return err;
    }

    if (_err(err = ffly_map_de_init(&__conf->env))) {
        ffly_fprintf(ffly_err, "failed to de-init map.\n");
        return err;
    }

    __ffly_mem_free(__conf->p);
    return FFLY_SUCCESS;
}

void static print_val(struct ffly_conf_val *__val) {
    if (__val->kind == _ffly_conf_vt_str)
        printf("%s\n", (char*)__val->p);
    else if (__val->kind == _ffly_conf_vt_chr)
        printf("%c\n", *(char*)__val->p);
    else if (__val->kind == _ffly_conf_vt_64l_int)
        printf("%lu\n", *(mdl_u64_t*)__val->p);
    else if (__val->kind == _ffly_conf_vt_32l_int)
        printf("%u\n", *(mdl_u32_t*)__val->p);
    else if (__val->kind == _ffly_conf_vt_16l_int)
        printf("%u\n", *(mdl_u16_t*)__val->p);
    else if (__val->kind == _ffly_conf_vt_8l_int)
        printf("%u\n", *(mdl_u8_t*)__val->p);
}

int main() {
    ffly_io_init();
    struct ffly_conf conf;
    ffly_conf_init(&conf);
    ffly_conf_ld(&conf, "test.conf");
    ffly_conf_read(&conf);

    void const *arr = ffly_conf_get(&conf, "info");
    if (!arr) {
        ffly_fprintf(ffly_out, "(null)\n");
        return -1;
    }

    void *in = ffly_conf_get_arr_elem(&conf, arr, 0);
    print_val((struct ffly_conf_val*)ffly_conf_get_arr_elem(&conf, in, 0));
 //   print_val((struct ffly_conf_val*)ffly_conf_get_arr_elem(&conf, arr, 1));
  //  print_val((struct ffly_conf_val*)ffly_conf_get_arr_elem(&conf, arr, 2));
   // print_val((struct ffly_conf_val*)ffly_conf_get_arr_elem(&conf, arr, 3));
    //print_val((struct ffly_conf_val*)ffly_conf_get(&conf, "git_url"));

    ffly_conf_free(&conf);
    ffly_io_closeup();
}
