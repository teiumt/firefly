# include "workshop.h"
# include "graphics/pipe.h"
# include "memory/plate.h"
# include "system/nanosleep.h"
# include "graphics/frame_buff.h"
# include "duct.h"
# include "system/pipe.h"
# include "dep/mem_set.h"
# include "graphics.h"
# include "layer.h"
# include "graphics/colour.h"
# include "memory/mem_alloc.h"
# include "memory/mem_free.h"
# include "graphics/fill.h"
# include "system/io.h"
# include "system/sched.h"
# include "event.h"
struct ff_workshop workshop;

ff_i16_t pt_x = 0, pt_y = 0;
ff_i8_t pt_state;
enum {
	_bt_opt,
	_bt_front
};

void opt() {
	ffly_gui_btn_draw(workshop.front, ffly_frame(__frame_buff__), WIDTH, HEIGHT);
}

void front() {
	ffly_gui_window_draw(&workshop.window, &workshop.frame);
	ffly_pallet_draw(&workshop.frame, ffly_frame(__frame_buff__), WIDTH, HEIGHT);
	ffly_gui_btn_draw(workshop.opt, ffly_frame(__frame_buff__), WIDTH, HEIGHT);
}

static void(*draw)(void) = front;

# include "types/wd_event_t.h"
void ffly_workshop_start() {
	ff_u64_t cc = 0; //cycle count
	while(1) {
		ffly_pixfill(ffly_frame(__frame_buff__), WIDTH*HEIGHT, ffly_colour(63, 60, 54, 255));
		draw();
		ff_eventp event;
		while(!ff_event_poll(&event)) {
			if (event->kind == _ffly_wd_ek_btn_press || event->kind == _ffly_wd_ek_btn_release) {
				pt_x = ((ffly_wd_event_t*)event->data)->x;
				pt_y = ((ffly_wd_event_t*)event->data)->y;
				if (event->kind == _ffly_wd_ek_btn_press) {
					pt_state = 0;
				} else if (event->kind == _ffly_wd_ek_btn_release) {
					pt_state = -1;
				}
			}
			
			ffly_printf("event.\n");
			ff_event_del(event);
		}
		ffly_sched_clock_tick(1);
		ffly_scheduler_tick();
		ffly_nanosleep(0, 100000000);
		ffly_grp_unload(&__ffly_grp__);

		if (!ff_duct_serve())
			break;
		ffly_printf("%u\n", cc++);
	}
}

void static bt_press(ffly_gui_btnp __btn, void *__arg) {
	ffly_printf("button press.\n");
	if (__btn->id == _bt_opt) {
		//ffly_pixfill(btn->texture, 20*20, ffly_colour(66, 194, 224, 255));
		ffly_printf("switching to opt menu.\n");
		draw = opt;
	} else if (__btn->id == _bt_front) {
		ffly_printf("switching to front menu.\n");
		draw = front;
	}
}

void static bt_release(ffly_gui_btnp __btn, void *__arg) {
	ffly_printf("button, release.\n");
	if (__btn->id == _bt_opt) {
		//ffly_pixfill(btn->texture, 20*20, ffly_colour(2, 52, 132, 255));
	}
}

void static bt_hover(ffly_gui_btnp __btn, void *__arg) {
	ffly_printf("button hover.\n");
}

ff_u8_t *tex0, *tex1;
void ffly_workshop_init() {
	ffly_pallet_init(&workshop.frame, WIDTH>>_ffly_tile_64, HEIGHT>>_ffly_tile_64, _ffly_tile_64);
	ffly_grp_prepare(&__ffly_grp__, 100);
	ff_set_frame_size(WIDTH, HEIGHT);
	ff_graphics_init();

	ff_duct_open(FF_PIPE_CREAT);
	ffly_mem_set(ffly_frame(__frame_buff__), 255, (WIDTH*HEIGHT)*4);
	ffly_pallet_update(&workshop.frame, ffly_frame(__frame_buff__));
	ff_duct_listen();

	ffly_queue_init(&ffly_event_queue, sizeof(ff_eventp));

	ffly_scheduler_init();
	tex0 = (ff_u8_t*)__ffly_mem_alloc(20*20*4);
	tex1 = (ff_u8_t*)__ffly_mem_alloc(20*20*4);
	ffly_pixfill(tex0, 20*20, ffly_colour(2, 52, 132, 255));
	ffly_pixfill(tex1, 20*20, ffly_colour(244, 206, 66, 255));
	

	ffly_gui_btnp btn;
	btn = ffly_gui_btn_creat(tex0, 20, 20, 40, 20);
	btn->pt_x = &pt_x;
	btn->pt_y = &pt_y;
	btn->press = bt_press;
	btn->hover = bt_hover;
	btn->release = bt_release;
	btn->pt_state = &pt_state;
	btn->id = _bt_opt;
	workshop.opt = btn;

	btn = ffly_gui_btn_creat(tex1, 20, 20, 20, 20);
	btn->pt_x = &pt_x;
	btn->pt_y = &pt_y;
	btn->press = bt_press;
	btn->hover = bt_hover;
	btn->release = bt_release;
	btn->pt_state = &pt_state;
	btn->id = _bt_front;
	workshop.front = btn;

	ffly_gui_window_init(&workshop.window, 128, 128, 40, 40);
}

void ffly_workshop_de_init() {
	ffly_queue_de_init(&ffly_event_queue);
	ff_duct_close();
	ff_graphics_de_init();
	ffly_plate_cleanup();
	__ffly_mem_free(tex0);
	__ffly_mem_free(tex1);
	ffly_gui_window_de_init(&workshop.window);
	ffly_gui_btn_destroy(workshop.opt);
	ffly_gui_btn_destroy(workshop.front);
	ffly_scheduler_de_init();
	ff_event_cleanup();
	ffly_grp_cleanup(&__ffly_grp__);
	ffly_grj_cleanup();
	ffly_pallet_de_init(&workshop.frame);
}

ff_err_t ffmain(int __argc, char const *__argv[]) {
	ffly_workshop_init();
	ffly_workshop_start();
	ffly_workshop_de_init();
}
