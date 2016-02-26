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

	volatile bool is_warp;
	volatile bool is_paused;

	//Emulator parameter data
	string dataPath;
	string diskImage;

	//Emulator display data
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

	//Emulator sound data
	recursive_mutex pcm_lock;

	uint32_t pcm_sampleRate;
	uint32_t pcm_bytesPerSample;
	uint32_t pcm_numChannels;

	deque<vector<uint8_t>> pcm;
	volatile bool pcm_dirty;

	//Emulator syncronization data
	recursive_mutex vsync_lock;
	volatile bool run_game;
};
