#include "../pch.h"
#include "gamescene.h"
#include "../engine.h"
#include "../management/game.h"
#include "../content/texanimmesh.h"

extern engine_s g_engine;

void GameScene::Init (int width, int height) {
	mC64Screen.reset (); //created in update phase
}

void GameScene::Shutdown () {
	mC64Screen.reset ();
}

void GameScene::Resize (int newWidth, int newHeight) {
}

void GameScene::Update (float elapsedTime) {
	if (mC64Screen == nullptr && g_engine.canvas_inited) {
		mC64Screen.reset (new TexAnimMesh (g_engine.canvas_width, g_engine.canvas_height));
	}
}

void GameScene::Render () {
	glClearColor (1.0f, 0.5f, 0.5f, 1.0f);
	//glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	if (mC64Screen != nullptr) {
		if (g_engine.canvas_dirty) {
			lock_guard <recursive_mutex> lock (g_engine.canvas_lock);

			mC64Screen->SetPixels (&g_engine.canvas[0]);
			mC64Screen->SetDirty (true);

			g_engine.canvas_dirty = false;
		}

		mC64Screen->Render ();
	}
}

void GameScene::TouchDown (int fingerID, float x, float y) {
	Scene::TouchDown (fingerID, x, y);

	//...
}

void GameScene::TouchUp (int fingerID, float x, float y) {
	Scene::TouchUp (fingerID, x, y);

	//...
}

void GameScene::TouchMove (int fingerID, float x, float y) {
	Scene::TouchMove (fingerID, x, y);

	//...
}
