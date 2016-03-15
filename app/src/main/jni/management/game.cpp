#include "../pch.h"
#include "game.h"

Game* Game::mGame = nullptr;

Game::Game (IContentManager& contentManager) :
	mContentManager (contentManager),
	mWidth (0),
	mHeight (0),
	mScreenWidth (0),
	mScreenHeight (0),
	mRefWidth (0),
	mRefHeight (0) {
	mGame = this;
}

void Game::Init (int screenWidth, int screenHeight, int refWidth, int refHeight) {
	mRefWidth = refWidth;
	mRefHeight = refHeight;

	InitProjection (screenWidth, screenHeight);
}

void Game::Shutdown () {
	SetCurrentScene (nullptr);
}

void Game::Pause () {
	if (mCurrentScene)
		mCurrentScene->Pause ();
}

void Game::Continue () {
	if (mCurrentScene)
		mCurrentScene->Continue ();
}

void Game::Resize (int newScreenWidth, int newScreenHeight) {
	float oldWidth = mWidth;
	float oldHeight = mHeight;

	int refWidth = mRefWidth;
	int refHeight = mRefHeight;
	if (newScreenWidth > newScreenHeight) { //Horizontal layout
		mRefWidth = max (refWidth, refHeight);
		mRefHeight = min (refWidth, refHeight);
	} else { //Vertical layout
		mRefWidth = min (refWidth, refHeight);
		mRefHeight = max (refWidth, refHeight);
	}

	InitProjection (newScreenWidth, newScreenHeight);

	if (mCurrentScene != nullptr)
		mCurrentScene->Resize (oldWidth, oldHeight, mWidth, mHeight);
}

void Game::Update (float elapsedTime) {
	if (mCurrentScene != nullptr)
		mCurrentScene->Update (elapsedTime);
}

void Game::Render () {
	if (mCurrentScene != nullptr)
		mCurrentScene->Render ();
}

void Game::SetCurrentScene (shared_ptr < Scene > scene) {
	if (mCurrentScene != nullptr) {
		mCurrentScene->Shutdown ();
		mCurrentScene = nullptr;
	}

	if (scene != nullptr) {
		mCurrentScene = scene;
		mCurrentScene->Init (mWidth, mHeight);
	}
}

void Game::TouchDown (int fingerID, float screenX, float screenY) {
	if (mCurrentScene != nullptr)
		mCurrentScene->TouchDown (fingerID, ScreenToLocal (screenX, screenY));
}

void Game::TouchUp (int fingerID, float screenX, float screenY) {
	if (mCurrentScene != nullptr)
		mCurrentScene->TouchUp (fingerID, ScreenToLocal (screenX, screenY));
}

void Game::TouchMove (int fingerID, float screenX, float screenY) {
	if (mCurrentScene != nullptr)
		mCurrentScene->TouchMove (fingerID, ScreenToLocal (screenX, screenY));
}

void Game::InitProjection (int screenWidth, int screenHeight) {
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	float min = (float) std::min (screenWidth, screenHeight);
	float max = (float) std::max (screenWidth, screenHeight);
	float aspect = max / min;
	if (screenWidth <= screenHeight) { //Vertical state
		mWidth = 1.0f;
		mHeight = aspect;

		glOrthof (0, 1.0f, aspect, 0, -1.0f, 1.0f);
	} else { //Horizontal state
		mWidth = aspect;
		mHeight = 1.0f;

		glOrthof (0, aspect, 1.0f, 0, -1.0f, 1.0f);
	}
}
