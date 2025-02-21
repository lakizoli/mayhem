#include "pch.h"
#include "jnihelper/JavaString.h"
#include "engine.h"
#include "game/mayhemgame.h"
#include "platform/androidcontentmanager.h"
#include "platform/audiomanager.h"
#include "management/game.h"

//c64emu declarations
extern "C" int main_program (int argc, char **argv);

extern "C" void ui_pause_emulation(void);
extern "C" void ui_continue_emulation(void);
extern "C" int ui_emulation_is_paused(void);

typedef void (*t_fn_ui_event_callback)(void);
extern "C" void ui_android_init_callbacks (t_fn_ui_event_callback eventCallback);

//Video callbacks
typedef int (*t_fn_init_canvas) (uint32_t width, uint32_t height, uint32_t bpp, uint32_t visible_width, uint32_t visible_height, uint8_t** buffer, uint32_t* pitch);
extern "C" void video_android_set_init_callback (t_fn_init_canvas init_canvas);

typedef void (*t_fn_lock_canvas) ();
extern "C" void video_android_set_locking_callbacks (t_fn_lock_canvas lock_canvas, t_fn_lock_canvas unlock_canvas);

typedef void (*t_fn_speed_callback) (double speed, double frame_rate, int warp_enabled);
extern "C" void vsyncarch_android_set_speed_callback (t_fn_speed_callback fn_speed_callback);

//Sound callbacks
typedef int (*t_fn_get_sample_rate) ();
typedef void (*t_fn_sound_init) (int numChannels, int sampleRate, int bytesPerSec);
typedef void (*t_fn_sound_close) ();
typedef void (*t_fn_sound_write) (const uint8_t* buffer, size_t size);

extern "C" void sound_android_set_pcm_callbacks (t_fn_get_sample_rate get_sample_rate, t_fn_sound_init sound_init, t_fn_sound_close sound_close, t_fn_sound_write sound_write);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Game data
////////////////////////////////////////////////////////////////////////////////////////////////////
engine_s g_engine; ///< The one and only global object of the game!

struct s_auto_vsync_lock {
	s_auto_vsync_lock () {
		while (!g_engine.is_warp) {
			{
				lock_guard <recursive_mutex> lock (g_engine.vsync_lock);
				if (g_engine.run_game)
					break;
			}

			sched_yield ();
		}
	}

	~s_auto_vsync_lock () {
		if (!g_engine.is_warp) {
			lock_guard <recursive_mutex> lock (g_engine.vsync_lock);
			g_engine.run_game = false;
		}
	}
};

static void UIEventCallback () {
	//Try to set run_game flag to true (only if the value was false before!)
	while (!g_engine.is_warp) {
		{
			lock_guard <recursive_mutex> lock (g_engine.vsync_lock);
			if (!g_engine.run_game) {
//				LOGD ("enter UI callback");
				g_engine.run_game = true;
				break;
			}
		}

		sched_yield ();
	}

	//... Here runs the overlay game code ...

	//Wait until run_game flag will be false again
	while (!g_engine.is_warp) {
		{
			lock_guard <recursive_mutex> lock (g_engine.vsync_lock);
			if (!g_engine.run_game) {
//				LOGD ("quit UI callback");
				break;
			}
		}

		sched_yield ();
	}
}

static int InitCanvas (uint32_t width, uint32_t height, uint32_t bpp, uint32_t visible_width, uint32_t visible_height, uint8_t** buffer, uint32_t* pitch) {
	CHECKMSG (buffer != nullptr, "InitCanvas () - buffer cannot be nullptr!");
	CHECKMSG (pitch != nullptr, "InitCanvas () - pitch cannot be nullptr!");

	uint32_t bytePerPixel = bpp / 8;

	g_engine.canvas_width = width;
	g_engine.canvas_height = height;
	g_engine.canvas_bit_per_pixel = bpp;
	g_engine.canvas_pitch = width * bytePerPixel;

	g_engine.visible_width = visible_width;
	g_engine.visible_height = visible_height;

	*pitch = g_engine.canvas_pitch;

	g_engine.canvas.resize (g_engine.canvas_pitch * height);
	*buffer = &g_engine.canvas[0];

	g_engine.canvas_dirty = false;
	g_engine.run_game = false;

	g_engine.canvas_inited = true;
	return 0;
}

static void LockCanvas () {
	g_engine.canvas_lock.lock ();
}

static void UnlockCanvas () {
	g_engine.canvas_dirty = true;
	g_engine.canvas_lock.unlock ();
}

static void DisplaySpeed (double speed, double frame_rate, int warp_enabled) {
	static int last_fps = -1;

	int fps = (int)frame_rate;
	if (fps != last_fps) {
		last_fps = fps;

#ifndef PRODUCTION_VERSION
		stringstream ss;
		ss << "FPS: " << fps;
		Game::ContentManager ().DisplayStatus (ss.str ());
#endif //PRODUCTION_VERSION
	}
}

static int SoundGetSampleRate () {
	return g_engine.deviceSamplingRate;
}

static void SoundInit (int numChannels, int sampleRate, int bytesPerSec) {
	lock_guard <recursive_mutex> lock (g_engine.pcm_lock);

	g_engine.pcm_sampleRate = (uint32_t)sampleRate;
	g_engine.pcm_bytesPerSec = (uint32_t)bytesPerSec;
	g_engine.pcm_numChannels = (uint32_t)numChannels;

	g_engine.pcm.clear ();
	g_engine.pcm_dirty = false;
}

static void SoundClose () {
	lock_guard <recursive_mutex> lock (g_engine.pcm_lock);

	g_engine.pcm_sampleRate = 0;
	g_engine.pcm_bytesPerSec = 0;
	g_engine.pcm_numChannels = 0;

	g_engine.pcm.clear ();
	g_engine.pcm_dirty = false;
}

static void SoundWrite (const uint8_t* buffer, size_t size) {
	lock_guard <recursive_mutex> lock (g_engine.pcm_lock);

	g_engine.pcm.push_back (vector<uint8_t> (buffer, buffer + size));
	g_engine.pcm_dirty = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// JNI functions of the GameLib java class
////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_init (JNIEnv *env, jclass clazz, jint screenWidth, jint screenHeight, jint refWidth, jint refHeight,
																			 jint deviceSamplingRate, jint deviceBufferFrames, jint deviceBufferCount) {
	CHECKMSG (g_engine.contentManager != nullptr, "g_engine.contentManager must be initialized before GameLib init!");
	CHECKMSG (g_engine.pointerIDs != nullptr, "g_engine.pointerIDs must be initialized before GameLib init!");

	if (!g_engine.game) {
		g_engine.game.reset (new MayhemGame (*(g_engine.contentManager)));
		g_engine.game->Init (screenWidth, screenHeight, refWidth, refHeight);
	}

	g_engine.lastUpdateTime = -1;
	g_engine.is_warp = true;
	g_engine.is_paused = false;

	g_engine.canvas_inited = false;
	g_engine.canvas_width = 0;
	g_engine.canvas_height = 0;
	g_engine.canvas_bit_per_pixel = 0;
	g_engine.canvas_pitch = 0;

	g_engine.visible_width = 0;
	g_engine.visible_height = 0;

	g_engine.canvas_dirty = false;

	g_engine.deviceSamplingRate = (uint32_t) deviceSamplingRate;
	g_engine.deviceBufferFrames = (uint32_t) deviceBufferFrames;
	g_engine.deviceBufferCount = (uint32_t) deviceBufferCount;
	g_engine.pcm_sampleRate = 0;
	g_engine.pcm_bytesPerSec = 0;
	g_engine.pcm_numChannels = 0;

	g_engine.pcm_dirty = false;

	glViewport (0, 0, screenWidth, screenHeight);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_mayheminmonsterland_GameLib_isInited (JNIEnv* env, jclass type) {
	return g_engine.game != nullptr ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_step (JNIEnv *env, jclass clazz) {
	//Sync game to vsync, when not in warp mode
	s_auto_vsync_lock autoVsyncLock;
//	LOGD ("UI callback started");

	//Get current time
	timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	double currentTime = (double) now.tv_sec + (double) now.tv_nsec / 1e9;

	//Update the game
	double elapsedTime = 0;
	if (g_engine.lastUpdateTime >= 0)
		elapsedTime = currentTime - g_engine.lastUpdateTime;
	g_engine.lastUpdateTime = currentTime;

	if (ui_emulation_is_paused ()) //Handle pause
		return;

	g_engine.game->Update ((float)elapsedTime);

	//Render the game
	g_engine.game->Render ();
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_pause (JNIEnv* env, jclass type) {
	g_engine.is_paused = true;

	g_engine.game->Pause ();
	ui_pause_emulation ();
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_resume (JNIEnv* env, jclass type) {
	g_engine.lastUpdateTime = -1;

	g_engine.game->Continue ();
	ui_continue_emulation ();

	g_engine.is_paused = false;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_mayheminmonsterland_GameLib_isPaused (JNIEnv* env, jclass type) {
	return ui_emulation_is_paused () ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_resize (JNIEnv* env, jclass clazz, jint newScreenWidth, jint newScreenHeight) {
	//Sync game to vsync, when not in warp mode
	s_auto_vsync_lock autoVsyncLock;
//	LOGD ("resize run");

	glViewport (0, 0, newScreenWidth, newScreenHeight);

	if (g_engine.game)
		g_engine.game->Resize (newScreenWidth, newScreenHeight);
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_mayheminmonsterland_GameLib_hasPointerID (JNIEnv* env, jclass clazz, jint id) {
	return g_engine.pointerIDs && g_engine.pointerIDs->find (id) != g_engine.pointerIDs->end () ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_insertPointerID (JNIEnv* env, jclass clazz, jint id) {
	if (g_engine.pointerIDs) {
		g_engine.pointerIDs->insert (id);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_erasePointerID (JNIEnv* env, jclass clazz, jint id) {
	if (g_engine.pointerIDs) {
		g_engine.pointerIDs->erase (id);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_touchDown (JNIEnv* env, jclass clazz, jint id, jfloat x, jfloat y) {
	if (g_engine.game) {
		g_engine.game->TouchDown (id, x, y);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_touchUp (JNIEnv* env, jclass clazz, jint id, jfloat x, jfloat y) {
	if (g_engine.game) {
		g_engine.game->TouchUp (id, x, y);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameLib_touchMove (JNIEnv* env, jclass clazz, jint id, jfloat x, jfloat y) {
	if (g_engine.game) {
		g_engine.game->TouchMove (id, x, y);
	}
}

extern "C" JNIEXPORT jint JNICALL Java_com_mayheminmonsterland_GameLib_runEmulator (JNIEnv *env, jclass clazz, jstring exePath, jstring diskPath) {
	CHECKMSG (g_engine.pointerIDs != nullptr, "g_engine must be initialized before start emulator (pointerIDs)!");
	CHECKMSG (g_engine.contentManager != nullptr, "g_engine must be initialized before start emulator (contentManager)!");
	CHECKMSG (g_engine.game != nullptr, "g_engine must be initialized before start emulator (game)!");

	CHECKMSG (exePath != nullptr, "exePath cannot be null!");
	CHECKMSG (diskPath != nullptr, "diskPath cannot be null!");

	string&& exe = JavaString (exePath).getString ();
	CHECKMSG (exe.length () > 0, "exePath cannot be empty!");

	string&& disk = JavaString (diskPath).getString ();
	CHECKMSG (disk.length () > 0, "diskPath cannot be empty!");

	//Compose parameters of the emulator
	char* exeBuffer = new char[exe.length () + 1];
	strcpy (&exeBuffer[0], exe.c_str ());

	char* diskBuffer = new char[disk.length () + 1];
	strcpy (&diskBuffer[0], disk.c_str ());

	char* argv[] = { exeBuffer, diskBuffer };
	int argc = sizeof (argv) / sizeof (argv[0]);

	g_engine.diskImage = disk;

	//Change working directory to exe dir
	size_t pos = exe.find_last_of ('/');
	if (pos != string::npos) {
		string dir = exe.substr (0, pos);
		int res = chdir (dir.c_str ());
		if (res != 0) {
			LOGE ("GameLib::runEmulator () - Cannot set working directory! error code: %d", res);
			return -1;
		}

		g_engine.dataPath = dir;
	}

	//Set callbacks of engine
	ui_android_init_callbacks (&UIEventCallback);
	video_android_set_init_callback (&InitCanvas);
	video_android_set_locking_callbacks (&LockCanvas, &UnlockCanvas);
	vsyncarch_android_set_speed_callback (&DisplaySpeed);
	sound_android_set_pcm_callbacks (&SoundGetSampleRate, &SoundInit, &SoundClose, &SoundWrite);

	//Call main function of emulator
	int res = main_program (argc, argv);

	//Clear allocated memory
	delete [] diskBuffer;
	diskBuffer = nullptr;

	delete [] exeBuffer;
	exeBuffer = nullptr;

	return res;
}
