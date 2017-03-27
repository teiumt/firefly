# include "room_manager.hpp"
boost::int8_t mdl::firefly::room_manager::add_room(uint_t*& __room_id, bool __overwrite) {
	if (__room_id != nullptr && !__overwrite) return FFLY_NOP;

	if (this-> room_count == 0) {
		if ((this-> _room_info = (room_info_t *)memory::mem_alloc(sizeof(room_info_t))) == NULL) {
			fprintf(stderr, "room_manager: failed to alloc memory for 'room_info', errno: %d\n", errno);
			return FFLY_FAILURE;
		}

		if ((this-> _room_data = (room_data_t *)memory::mem_alloc(sizeof(room_data_t))) == NULL) {
			fprintf(stderr, "room_manager: failed to alloc memory for 'room_data', errno: %d\n", errno);
			return FFLY_FAILURE;
		}
	} else {
			room_info_t *room_info = NULL;
			if ((room_info = (room_info_t *)memory::mem_realloc(this-> _room_info, (this-> room_count + 1) * sizeof(room_info_t))) == NULL) {
				fprintf(stderr, "room_manager: failed to realloc memory for 'room_info', errno: %d\n", errno);
				return FFLY_FAILURE;
			} else
				this-> _room_info = room_info;

			room_data_t *room_data = NULL;
			if ((room_data = (room_data_t *)memory::mem_realloc(this-> _room_data, (this-> room_count + 1) * sizeof(room_data_t))) == NULL) {
				fprintf(stderr, "room_manager: failed to realloc memory for 'room_info', errno: %d\n", errno);
				room_info = NULL;
				if ((room_info = (room_info_t *)memory::mem_realloc(this-> _room_info, (this-> room_count - 1) * sizeof(room_info_t))) == NULL)
					return FFLY_FAILURE;
				else
					this-> _room_info = room_info;
			} else
				this-> _room_info = room_info;
	}

	room_info_t *room_info = nullptr;
	room_data_t *room_data = nullptr;

	if (!this-> unused_ids.empty()) {
		std::set<uint_t *>::iterator itor = this-> unused_ids.end();
		--itor;

		__room_id = *itor;
		this-> unused_ids.erase(itor);
	} else {
		this-> _room_info[this-> room_count].id, __room_id = (uint_t *)memory::mem_alloc(sizeof(uint_t));
		if (__room_id == NULL) {
			fprintf(stderr, "room_manager: failed to alloc memory for 'room_id', errno: %d\n", errno);
			goto mem_clean;
		} else
			*this-> _room_info[this-> room_count].id, *__room_id = this-> room_count;
	}

	room_info = &this-> _room_info[*__room_id];
	room_data = &this-> _room_data[*__room_id];

	gui::btn_manager *btn_manager;
	if ((btn_manager = (gui::btn_manager *)memory::mem_alloc(sizeof(gui::btn_manager))) == NULL) {
		fprintf(stderr, "room_manager: failed to alloc memory for 'btn_manager', errno: %d\n", errno);
		goto mem_clean;
	} else
		room_data-> btn_manager = new (btn_manager) gui::btn_manager(nullptr, &this-> wd_coords, &this-> mouse_coords, 0, 0);

	this-> room_count ++;

	return FFLY_SUCCESS;
	mem_clean:

	if (__room_id != NULL)
		memory::mem_free(__room_id);

	if (this-> room_count == 0) {
		memory::mem_free(this-> _room_info);
		memory::mem_free(this-> _room_data);
	} else {
		this-> _room_info = (room_info_t *)memory::mem_realloc(this-> _room_info, (this-> room_count - 1) * sizeof(room_info_t));
		this-> _room_data = (room_data_t *)memory::mem_realloc(this-> _room_data, (this-> room_count - 1) * sizeof(room_data_t));
	}

	return FFLY_FAILURE;
}

boost::int8_t mdl::firefly::room_manager::rm_room(uint_t *__room_id, bool __hard) {

}

boost::int8_t mdl::firefly::room_manager::manage(uint_t *__room_id) {
	uint_t *room_id = __room_id == nullptr? this-> curr_room_id : __room_id;

	
}

boost::int8_t mdl::firefly::room_manager::de_init() {
}
