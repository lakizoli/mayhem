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
	if (mC64Screen)
		mC64Screen->Shutdown ();
	mC64Screen.reset ();
}

void GameScene::Resize (int newWidth, int newHeight) {
}

void GameScene::Update (float elapsedTime) {
	//Create C64 screen texture
	if (!mC64Screen && g_engine.canvas_inited) {
		mC64Screen.reset (new TexAnimMesh (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel));
		mC64Screen->Init ();
		mC64Screen->Pos = Vector2D (0.5f, 0.5f);
		mC64Screen->Scale = Vector2D (0.45f, 0.45f);

		uint32_t bytePerPixel = g_engine.canvas_bit_per_pixel / 8;
		uint32_t pitch_src = g_engine.canvas_width * bytePerPixel;
		uint32_t pitch_dest = g_engine.visible_width * bytePerPixel;
		mC64Pixels.resize (pitch_dest * g_engine.visible_height);
	}

	//Update C64 screen texture
	if (mC64Screen) {
		if (g_engine.canvas_dirty) {
			lock_guard <recursive_mutex> lock (g_engine.canvas_lock);

			//Trim screen pixel buffer to visible size (convert BGR to RGB)
			assert (g_engine.visible_height <= g_engine.canvas_height);

			//TODO: ... hqnx scale (if needed...)

			uint32_t bytePerPixel = g_engine.canvas_bit_per_pixel / 8;
			uint32_t pitch_src = g_engine.canvas_width * bytePerPixel;
			uint32_t pitch_dest = g_engine.visible_width * bytePerPixel;
			assert (mC64Pixels.size () == pitch_dest * g_engine.visible_height);

			for (uint32_t y = 0, yEnd = g_engine.visible_height;y < yEnd;++y) {
				uint32_t* src_pixel = (uint32_t*) (&g_engine.canvas[y * pitch_src]);
				uint32_t* dst_pixel = (uint32_t*) (&mC64Pixels[y * pitch_dest]);

				for (uint32_t x = 0, xEnd = g_engine.visible_width;x < xEnd;x += 4) {
					uint32_t src = *src_pixel++;
					*dst_pixel++ = (src & 0x00FF0000) >> 16 | (src & 0x0000FF00) | (src & 0x000000FF) << 16 | 0xFF000000;

					src = *src_pixel++;
					*dst_pixel++ = (src & 0x00FF0000) >> 16 | (src & 0x0000FF00) | (src & 0x000000FF) << 16 | 0xFF000000;

					src = *src_pixel++;
					*dst_pixel++ = (src & 0x00FF0000) >> 16 | (src & 0x0000FF00) | (src & 0x000000FF) << 16 | 0xFF000000;

					src = *src_pixel++;
					*dst_pixel++ = (src & 0x00FF0000) >> 16 | (src & 0x0000FF00) | (src & 0x000000FF) << 16 | 0xFF000000;
				}
			}

			mC64Screen->SetPixels (g_engine.visible_width, g_engine.visible_height, g_engine.canvas_bit_per_pixel, &mC64Pixels[0]);
			g_engine.canvas_dirty = false;
		}
	}
}

void GameScene::Render () {
	//glClearColor (1.0f, 0.5f, 0.5f, 1.0f);
	glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	if (mC64Screen)
		mC64Screen->Render ();
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
