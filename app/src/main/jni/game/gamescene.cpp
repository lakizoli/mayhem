#include "../pch.h"
#include "gamescene.h"

void GameScene::Init (int width, int height) {
}

void GameScene::Shutdown () {
}

void GameScene::Resize (int newWidth, int newHeight) {
}

void GameScene::Update (float elapsedTime) {
}

void GameScene::Render () {
	glClearColor (1.0f, 0.5f, 0.5f, 1.0f);
	//glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();

	//...
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
