# include "tex.h"
# include "context.h"
# include "../system/io.h"

ff_u16_t static
_bron_texbuf_new(ff_u32_t __size) {
	ff_u16_t buf;
	struct bron_context *ctx;
	ctx = BRON_CONTEXT;
	buf = ctx->stack;
	ctx->stack+=8;

	ctx->driver.texbuf_new(buf, __size);
	return buf;
}

void static
_bron_texbuf_destroy(ff_u16_t __buf) {
	BRON_CONTEXT->driver.texbuf_destroy(__buf);
}

void static
_bron_texbuf_map(ff_u16_t __buf) {
	BRON_CONTEXT->driver.texbuf_map(__buf);
}

void static
_bron_texbuf_unmap(ff_u16_t __buf) {
	BRON_CONTEXT->driver.texbuf_unmap(__buf);
}

void static
_bron_texbuf_write(ff_u16_t __buf, ff_u32_t __offset, ff_u32_t __size, void *__src) {
	BRON_CONTEXT->driver.texbuf_write(__buf, __offset, __size, __src);
}

void static
_bron_texbuf_read(ff_u16_t __buf, ff_u32_t __offset, ff_u32_t __size, void *__dst) {
	BRON_CONTEXT->driver.texbuf_read(__buf, __offset, __size, __dst);
}

ff_u16_t ffly_bron_texbuf_new(ff_u32_t __size) {
	return _bron_texbuf_new(__size);
}

void ffly_bron_texbuf_destroy(ff_u16_t __buf) {
	_bron_texbuf_destroy(__buf);
}

void ffly_bron_texbuf_map(ff_u16_t __buf) {
	_bron_texbuf_map(__buf);
}

void ffly_bron_texbuf_unmap(ff_u16_t __buf) {
	_bron_texbuf_unmap(__buf);
}

void ffly_bron_texbuf_write(ff_u16_t __buf, ff_u32_t __offset, ff_u32_t __size, void *__src) {
	_bron_texbuf_write(__buf, __offset, __size, __src);
}

void ffly_bron_texbuf_read(ff_u16_t __buf, ff_u32_t __offset, ff_u32_t __size, void *__dst) {
	_bron_texbuf_read(__buf, __offset, __size, __dst);
}

# include "../memory/mem_alloc.h"
# include "../memory/mem_free.h"
/*
	for debug
*/
ff_u16_t static
_bron_tex_new(ff_u16_t __tb) {
	ff_u16_t tx;
	struct bron_context *ctx;
	ctx = BRON_CONTEXT;
	
	tx = ctx->stack;
	ctx->driver.tex_new(tx, __tb);
	ctx->stack+=8;
	return tx;
}

ff_u16_t ffly_bron_tex_new(ff_u16_t __tb) {
	return _bron_tex_new(__tb);
}
