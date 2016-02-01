#include "pch.h"
#include "jnihelper/JavaString.h"
#include "engine.h"
#include "game/mayhemgame.h"
#include "platform/androidcontentmanager.h"
#include "platform/androidutil.h"

//c64emu declarations
extern "C" int main_program (int argc, char **argv);

////////////////////////////////////////////////////////////////////////////////////////////////////
// Game data
////////////////////////////////////////////////////////////////////////////////////////////////////
engine_s g_engine; ///< The one and only global object of the game!

////////////////////////////////////////////////////////////////////////////////////////////////////
// JNI functions of the GameLib java class
////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_init (JNIEnv *env, jclass clazz, jint width, jint height, jint refWidth, jint refHeight) {
	CHECKMSG (g_engine.util != nullptr, "g_engine.util must be initialized before GameLib init!");
	CHECKMSG (g_engine.contentManager != nullptr, "g_engine.contentManager must be initialized before GameLib init!");
	CHECKMSG (g_engine.pointerIDs != nullptr, "g_engine.pointerIDs must be initialized before GameLib init!");

	if (!g_engine.game) {
		g_engine.game.reset (new MayhemGame (*(g_engine.util), *(g_engine.contentManager)));
		g_engine.game->Init (width, height, refWidth, refHeight);
	}

	g_engine.lastUpdateTime = -1;

	glViewport (0, 0, width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_step (JNIEnv *env, jclass clazz) {
	//Get current time
	timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	double currentTime = (double) now.tv_sec + (double) now.tv_nsec / 1e9;

	//Update the game
	double elapsedTime = 0;
	if (g_engine.lastUpdateTime >= 0)
		elapsedTime = currentTime - g_engine.lastUpdateTime;
	g_engine.lastUpdateTime = currentTime;

	g_engine.game->Update ((float)elapsedTime);

	//Render the game
	g_engine.game->Render ();
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_mayhem_GameLib_hasPointerID (JNIEnv* env, jclass clazz, jint id) {
	return g_engine.pointerIDs && g_engine.pointerIDs->find (id) != g_engine.pointerIDs->end () ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_insertPointerID (JNIEnv* env, jclass clazz, jint id) {
	if (g_engine.pointerIDs) {
		g_engine.pointerIDs->insert (id);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_erasePointerID (JNIEnv* env, jclass clazz, jint id) {
	if (g_engine.pointerIDs) {
		g_engine.pointerIDs->erase (id);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_touchDown (JNIEnv* env, jclass clazz, jint id, jfloat x, jfloat y) {
	if (g_engine.game) {
		g_engine.game->TouchDown (id, x, y);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_touchUp (JNIEnv* env, jclass clazz, jint id, jfloat x, jfloat y) {
	if (g_engine.game) {
		g_engine.game->TouchUp (id, x, y);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_touchMove (JNIEnv* env, jclass clazz, jint id, jfloat x, jfloat y) {
	if (g_engine.game) {
		g_engine.game->TouchMove (id, x, y);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_mayhem_GameLib_runEmulator (JNIEnv *env, jclass clazz, jstring exePath, jstring diskPath) {
	CHECKMSG (g_engine.pointerIDs != nullptr, "g_engine must be initialized before start emulator (pointerIDs)!");
	CHECKMSG (g_engine.util != nullptr, "g_engine must be initialized before start emulator (util)!");
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

	char* argv[2] = { exeBuffer, diskBuffer };
	int argc = sizeof (argv) / sizeof (argv[0]);

	//Call main function of emulator
	int res = main_program (argc, argv);

	//Clear allocated memory
	delete [] diskBuffer;
	diskBuffer = nullptr;

	delete [] exeBuffer;
	exeBuffer = nullptr;
}
