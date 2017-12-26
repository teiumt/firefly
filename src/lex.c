# define __ffly_script_internal
# include "script.h"
# include "types/bool_t.h"
# include "memory/mem_alloc.h"
# include "memory/mem_free.h"
# include "data/str_dupe.h"
# include "data/str_len.h"
ffly_bool_t is_space(char __c) {
	if (__c == ' ' || __c == '\n') return 1;
	return 0;
}

char static fetchc(struct ffly_script *__script) {
	return *(__script->p+__script->off);
}

ffly_bool_t is_next(struct ffly_script *__script, char __c) {
    return (*(__script->p+__script->off+1) == __c);
}

char static* read_ident(struct ffly_script *__script) {
	char *itr = (char*)(__script->p+__script->off);
	while((*itr >= 'a' && *itr <= 'z') || *itr == '_' || (*itr >= '0' && *itr <= '9')) {
		ffly_buff_put(&__script->sbuf, itr++);
		ffly_buff_incr(&__script->sbuf);
	}

	char end = '\0';
	ffly_buff_put(&__script->sbuf, &end);
	char *s = ffly_str_dupe((char*)ffly_buff_begin(&__script->sbuf));
	__script->off+= ffly_buff_off(&__script->sbuf);
	ffly_buff_off_reset(&__script->sbuf);
	return s;
}

char static* read_no(struct ffly_script *__script) {
	char *itr = (char*)(__script->p+__script->off);
	while(*itr >= '0' && *itr <= '9') {
		ffly_buff_put(&__script->sbuf, itr++);
		ffly_buff_incr(&__script->sbuf);
	}

	char end = '\0';
	ffly_buff_put(&__script->sbuf, &end);
	char *s = ffly_str_dupe((char*)ffly_buff_begin(&__script->sbuf));
	__script->off+= ffly_buff_off(&__script->sbuf);
	ffly_buff_off_reset(&__script->sbuf);
	return s;
}

void make_keyword(struct token *__tok, mdl_u8_t __id) {
	__tok->kind = TOK_KEYWORD;
	__tok->id = __id;
}

static struct token* read_token(struct ffly_script *__script) {
	struct token *tok = (struct token*)__ffly_mem_alloc(sizeof(struct token));
	tok->p = NULL;
    ffly_off_t off = __script->off;
	switch(fetchc(__script)) {
        case '<':
            make_keyword(tok, _lt);
            __script->off++;
        break;
        case '>':
            make_keyword(tok, _gt);
            __script->off++;
        break;
        case '!':
            if (is_next(__script, '=')) {
                make_keyword(tok, _neq);
                __script->off++;
            }
            __script->off++;
        break;
		case ';':
			make_keyword(tok, _semicolon);
			__script->off++;
		break;
		case '=':
            if (is_next(__script, '=')) {
                make_keyword(tok, _eqeq);
                __script->off++;
            } else
			    make_keyword(tok, _eq);
			__script->off++;
		break;
		case '(':
			make_keyword(tok, _l_paren);
			__script->off++;
		break;
		case ')':
			make_keyword(tok, _r_paren);
			__script->off++;
		break;
        case '{':
            make_keyword(tok, _l_brace);
            __script->off++;
        break;
        case '}':
            make_keyword(tok, _r_brace);
            __script->off++;
        break;
		default:
		if ((fetchc(__script) >= 'a' && fetchc(__script) <= 'z') || fetchc(__script) == '_') {
			*tok = (struct token) {
				.kind = TOK_IDENT,
				.p = (void*)read_ident(__script)
			};
		} else if (fetchc(__script) >= '0' && fetchc(__script) <= '9') {
			*tok = (struct token) {
				.kind = TOK_NO,
				.p = (void*)read_no(__script)
			};
		} else {
			__script->off++;
			tok->kind = TOK_NULL;
		}
	}
    
    tok->line = curl(__script);
    tok->off = off;
    tok->lo = curlo(__script);
	struct token **p;
	ffly_vec_push_back(&__script->toks, (void**)&p);
	*p = tok;
	return tok;
}

void ffly_script_ulex(struct ffly_script *__script, struct token *__tok) {
	if (!__tok) return;
	ffly_buff_put(&__script->iject_buff, (void*)&__tok);
	ffly_buff_incr(&__script->iject_buff);
}

struct token* ffly_script_lex(struct ffly_script *__script, ffly_err_t *__err) {
	if (ffly_buff_off(&__script->iject_buff)>0) {
		ffly_buff_decr(&__script->iject_buff);
		struct token *tok;
		ffly_buff_get(&__script->iject_buff, (void*)&tok);
		return tok;
	}

	while(is_space(fetchc(__script)) && !is_eof(__script)) {
        if (fetchc(__script) == '\n') {
            if (__script->end-1 != __script->p+__script->off) {
                __script->line++;
                __script->lo = __script->off+1;
            }
        }
        __script->off++;
    }

	struct token *tok;
	while(!(tok = read_token(__script)) && !is_eof(__script));
	return tok;
}
