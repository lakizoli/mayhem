#include "pch.h"
#include "engine.h"
#include "game/mayhemgame.h"
#include "platform/androidcontentmanager.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Game data
////////////////////////////////////////////////////////////////////////////////////////////////////
extern engine_s g_engine;

////////////////////////////////////////////////////////////////////////////////////////////////////
// JNI functions of the GameActivity java class
////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" JNIEXPORT void JNICALL Java_com_mayheminmonsterland_GameActivity_init (JNIEnv *env, jobject obj, jobject jAssetManager) {
	if (!g_engine.pointerIDs) {
		g_engine.pointerIDs.reset (new set<int32_t> ());
	}

	if (!g_engine.contentManager) {
		g_engine.contentManager.reset (new AndroidContentManager (obj, jAssetManager));
	}
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_mayheminmonsterland_GameActivity_isLite (JNIEnv* env, jobject obj) {
#ifdef FULL_VERSION
	return JNI_FALSE;
#else //FULL_VERSION
	return JNI_TRUE;
#endif //FULL_VERSION
}
