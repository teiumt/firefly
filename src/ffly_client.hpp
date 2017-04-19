# ifndef __ffly__client__hpp
# define __ffly__client__hpp
# include "networking/tcp_client.hpp"
# include "networking/udp_client.hpp"
# include <chrono>
# include <signal.h>
# include "draw.hpp"
# include <boost/cstdint.hpp>
# include "types/client_info_t.hpp"
# include "types/player_info_t.hpp"
# include "asset_manager.hpp"
# include "layer_manager.hpp"
# include <serializer.hpp>
# include <cuda_runtime.h>
# include "defaults.hpp"
# include <errno.h>
# include "system/errno.h"
# include "types/init_opt_t.hpp"
# include "graphics/window.hpp"
# include "types/layer_info_t.hpp"
# include "system/event.hpp"
# include "ffly_system.hpp"
# include "ffly_graphics.hpp"
# include "entity_manager.hpp"
# ifdef __WITH_OBJ_MANAGER
#	include "obj_manager.hpp"
# endif

# ifdef ROOM_MANAGER
#	include "room_manager.hpp"
# endif

# ifdef __WITH_UNI_MANAGER
#	include "uni_manager.hpp"
#   include "types/uni_prop_t.hpp"
# endif
# include "types/id_t.hpp"
namespace mdl { class ffly_client
{
	public:
# ifndef __WITH_UNI_MANAGER
	ffly_client(boost::uint16_t __win_xlen, boost::uint16_t __win_ylen) : win_xlen(__win_xlen), win_ylen(__win_ylen) {}
# else
	ffly_client(boost::uint16_t __win_xlen, boost::uint16_t __win_ylen, firefly::types::uni_prop_t uni_props)
	: win_xlen(__win_xlen), win_ylen(__win_ylen), uni_manager(uni_props.xaxis_len, uni_props.yaxis_len, uni_props.zaxis_len) {}
# endif
	boost::int8_t connect_to_server(int& __sock);
	boost::int8_t send_client_info();
	boost::int8_t recv_cam_frame();

	~ffly_client() {
		printf("ffly_client has safly shutdown.\n");
//		this-> cu_clean();
	}

	void cu_clean() {
		cudaDeviceReset();
	}

	boost::int8_t de_init();

	void shutdown();
	typedef struct {
		uint_t fps_count() {
			return _this-> curr_fps;
		}

		bool poll_event(firefly::system::event& __event) {
			return this-> _this-> poll_event(__event);
		}

		boost::int8_t connect_to_server(char const *__addr, boost::uint16_t __portno, uint_t __layer_id) {
			if (_this-> layer.does_layer_exist(__layer_id)) {
				fprintf(stderr, "error the layer for the camera does not exist.\n");
				return FFLY_FAILURE;
			}

			firefly::types::layer_info_t layer_info = _this-> layer.get_layer_info(__layer_id);

			if (layer_info.xaxis_len != _this-> cam_xlen || layer_info.yaxis_len != _this-> cam_ylen) {
				fprintf(stderr, "error layer size does not match the camera size.\n");
				return FFLY_FAILURE;
			}

			_this-> server_ipaddr = __addr;
			_this-> server_portno = __portno;
			_this-> cam_layer_id = __layer_id;

			return FFLY_SUCCESS;
		}

		bool server_connected() {
			return _this-> server_connected;
		}

		firefly::types::pixmap_t pixbuff = nullptr;

		mdl::ffly_client *_this;
	} portal_t;

	firefly::types::__id_t bse_layer_id = 0;
	firefly::layer_manager layer;

	char const *server_ipaddr = nullptr;
	boost::uint16_t server_portno = 0;
	bool server_connected = false;

	boost::int8_t init(firefly::types::init_opt_t __init_options);

	boost::uint8_t begin(char const *__frame_title,
		void (* __extern_loop)(boost::int8_t, portal_t*, void *), void *__this
	);

	firefly::graphics::window window;

	bool poll_event(firefly::system::event& __event);

	bool static to_shutdown;

	firefly::system::event *event = nullptr;
	portal_t portal;

	uint_t cam_layer_id = 0;
	uint_t curr_fps = 0;
	uint_t fps_counter = 0;

	uint_t cam_xlen = 0, cam_ylen = 0;
	uint_t cam_pm_size = 0;
	uint_t connect_trys = 0;

	firefly::asset_manager asset_manager;
# ifdef __WITH_OBJ_MANAGER
	firefly::obj_manager *obj_manager = nullptr;
	void manage_objs() {
		if (this-> obj_manager == nullptr) return;
		this-> obj_manager-> manage();
	}
# endif

# ifdef ROOM_MANAGER
	firefly::types::id_t bse_room_id;
	firefly::room_manager room_manager;
# endif

# ifdef __WITH_UNI_MANAGER
	firefly::uni_manager uni_manager;
# endif

	firefly::entity_manager entity_manager;
	private:
	firefly::types::init_opt_t init_options;
	firefly::types::client_info_t client_info;

	firefly::networking::tcp_client tcp_stream;
	firefly::networking::udp_client udp_stream;
	boost::uint16_t const win_xlen, win_ylen;
};
}

# endif /*__ffly__client__hpp*/

