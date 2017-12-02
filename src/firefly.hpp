# ifndef __firefly__hpp
# define __firefly__hpp
# include "types/err_t.h"
# include "types/bool_t.h"
# ifdef FFLY_CLIENT
#	include "system/event.hpp"
#	include "system/event_field.h"
#	include "ffly_client.hpp"
# endif
/*
# include "ffly_graphics.hpp"
# include "ffly_system.hpp"
# include "ffly_memory.hpp"
# include "system/event.hpp"
# include "system/smem_buff.h"
# include "types/btn_event_t.hpp"
# include "types/event_id_t.hpp"
# include "types/event_t.hpp"
# include "types/coords_t.h"
# include "types/bool_t.h"
# include "core_portal.h"
# include "graphics/window.hpp"
*/
# include "act.h"
ffly_act_gid_t extern act_gid_de_init;
ffly_act_gid_t extern act_gid_cleanup;
namespace mdl {
namespace firefly {

types::err_t init();
types::err_t de_init();
/*
struct ffly_smem_buff_t extern *gui_btn_ev_dbuff;
struct ffly_smem_buff_t extern *wd_ev_dbuff;


types::err_t init_core_portal(graphics::window *__window);
*/
# ifdef FFLY_CLIENT
types::bool_t poll_event(types::event_t& __event);
void event_free(ffly_client *__ffc, types::event_t& __event);
# endif
}
}

# endif /*__firefly__hpp*/
