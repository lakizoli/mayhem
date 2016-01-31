#pragma once

class AndroidUtil;
class AndroidContentManager;
class MayhemGame;

struct engine_s {
	unique_ptr<AndroidUtil> util;
	unique_ptr<AndroidContentManager> contentManager;
	unique_ptr<MayhemGame> game;
	unique_ptr<set<int32_t>> pointerIDs;
	double lastUpdateTime;
};
