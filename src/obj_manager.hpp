# ifndef __obj__manager__hpp
# define __obj__manager__hpp
# include <eint_t.hpp>
# include "types/pixmap_t.h"
# include "types/obj_info_t.hpp"
# include <boost/numeric/ublas/vector.hpp>
# include <utility>
# include <string.h>
# include "system/stop_watch.hpp"
# include <boost/thread.hpp>
# include "graphics/draw_pixmap.hpp"
// how many obj's will be handled in 1 thread
# define BLOCK_SIZE 12
namespace ublas = boost::numeric::ublas;
namespace mdl {
namespace firefly {
class obj_manager {
	public:
	obj_manager(types::pixmap_t __pixbuff, uint_t __pb_xlen, uint_t __pb_ylen, uint_t __pb_zlen)
	: pixbuff(__pixbuff), pb_xlen(__pb_xlen), pb_ylen(__pb_ylen), pb_zlen(__pb_zlen) {}

	uint_t add(uint_t __xlen, uint_t __ylen, uint_t __zlen, uint_t __xaxis, uint_t __yaxis, uint_t __zaxis); // return obj_id
/*
	void move_forwards(uint_t __obj_id, uint_t __amount) {
		this-> set_zaxis(this-> get_zaxis() + __amount);
	}

	void move_backwards(uint_t __obj_id, uint_t __amount) {
		this-> set_zaxis(this-> get_zaxis() - __amount);
	}
*/

	void push_xaxis(uint_t __obj_id, uint_t __amount) {
		this-> set_xaxis(__obj_id, this-> get_xaxis(__obj_id) + __amount);
	}

	void pull_xaxis(uint_t __obj_id, uint_t __amount) {
		this-> set_xaxis(__obj_id, this-> get_xaxis(__obj_id) - __amount);
	}

	void push_yaxis(uint_t __obj_id, uint_t __amount) {
		this-> set_yaxis(__obj_id, this-> get_yaxis(__obj_id) + __amount);
	}

	void pull_yaxis(uint_t __obj_id, uint_t __amount) {
		this-> set_yaxis(__obj_id, this-> get_yaxis(__obj_id) - __amount);
	}

	uint_t get_xaxis(uint_t __obj_id) {
		return this-> obj_index[__obj_id].first.xaxis;
	}

	uint_t get_yaxis(uint_t __obj_id) {
		return this-> obj_index[__obj_id].first.yaxis;
	}

	void enable_gravity(uint_t __obj_id) {
		this-> obj_index[__obj_id].first.gravity_enabled = true;
	}

	void disable_gravity(uint_t __obj_id) {
		this-> obj_index[__obj_id].first.gravity_enabled = false;
	}

	void set_xaxis(uint_t __obj_id, uint_t __xaxis) {
		//uint_t old_xaxis = this-> obj_index[__obj_id].first.xaxis;

		if (this-> obj_index[__obj_id].first.bound_enabled) {
			if (__xaxis < this-> obj_index[__obj_id].first.xaxis_bound[0] || (__xaxis + this-> obj_index[__obj_id].first.xaxis_len) >= obj_index[__obj_id].first.xaxis_bound[1]) return;
		}

		this-> obj_index[__obj_id].first.xaxis = __xaxis;

	}

	void enable_bound(uint_t __obj_id) {
		this-> obj_index[__obj_id].first.bound_enabled = true;
	}

	void disable_bound(uint_t __obj_id) {
		this-> obj_index[__obj_id].first.bound_enabled = false;
	}

	void set_yaxis(uint_t __obj_id, uint_t __yaxis) {
		//uint_t old_yaxis = this-> obj_index[__obj_id].first.yaxis;

		if (this-> obj_index[__obj_id].first.bound_enabled) {
			if (__yaxis < this-> obj_index[__obj_id].first.yaxis_bound[0] || (__yaxis + this-> obj_index[__obj_id].first.yaxis_len) >= obj_index[__obj_id].first.yaxis_bound[1]) return;
		}

		this-> obj_index[__obj_id].first.yaxis = __yaxis;
	}

	void change_zaxis(uint_t __obj_id, uint_t __zaxis) {
		this-> obj_index[__obj_id].first.zaxis = __zaxis;
	}

	void set_xaxis_bound(uint_t __obj_id, uint_t __xaxis_min, uint_t __xaxis_max) {
		this-> obj_index[__obj_id].first.xaxis_bound[0] = __xaxis_min;
		this-> obj_index[__obj_id].first.xaxis_bound[1] = __xaxis_max;
	}

	void set_yaxis_bound(uint_t __obj_id, uint_t __yaxis_min, uint_t __yaxis_max) {
		this-> obj_index[__obj_id].first.yaxis_bound[0] = __yaxis_min;
		this-> obj_index[__obj_id].first.yaxis_bound[1] = __yaxis_max;
	}

	void set_zaxis_bound(uint_t __obj_id, uint_t __zaxis_min, uint_t __zaxis_max) {
		this-> obj_index[__obj_id].first.zaxis_bound[0] = __zaxis_min;
		this-> obj_index[__obj_id].first.zaxis_bound[1] = __zaxis_min;
	}

	boost::int8_t manage();

	typedef struct {
		uint_t amount, offset, id;
	} thr_config_t;

	void handle_objs(thr_config_t *thread_config);
	void draw_objs();

	private:
	bool thread_waiting = false;
	bool waiting_for_thr = false;
	uint_t waiting_thr_id = 0;
	ublas::vector<thr_config_t *> thread_index;
	types::pixmap_t pixbuff = nullptr;
	uint_t const pb_xlen, pb_ylen, pb_zlen;
	uint_t obj_count = 0, thr_offset = 0;
	ublas::vector<std::pair<types::obj_info_t, types::pixmap_t> > obj_index;
};
}
}

# endif /*__obj__manager__hpp*/
