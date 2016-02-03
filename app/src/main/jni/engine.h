#pragma once

class AndroidUtil;
class AndroidContentManager;
class MayhemGame;

struct engine_s {
	//Game data
	unique_ptr<AndroidUtil> util;
	unique_ptr<AndroidContentManager> contentManager;
	unique_ptr<MayhemGame> game;
	unique_ptr<set<int32_t>> pointerIDs;
	double lastUpdateTime;

	//Emulator data
	recursive_mutex canvas_lock;

	volatile bool canvas_inited;
	uint32_t canvas_width;
	uint32_t canvas_height;
	uint32_t canvas_bit_per_pixel;
	uint32_t canvas_pitch;

	uint32_t visible_width;
	uint32_t visible_height;

	vector<uint8_t> canvas; //screen pixels in BGR format
	volatile bool canvas_dirty;
};
