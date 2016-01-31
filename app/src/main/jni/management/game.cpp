#include "../pch.h"
#include "game.h"

Game* Game::mGame = nullptr;

Game::Game (IUtil& util, IContentManager& contentManager) : mUtil (util), mContentManager (contentManager), mWidth (0), mHeight (0), mRefWidth (0), mRefHeight (0) {
	mGame = this;
}

void Game::Init (int width, int height, int refWidth, int refHeight) {
	if (width > height) {
		mRefWidth = max (refWidth, refHeight);
		mRefHeight = min (refWidth, refHeight);
	} else {
		mRefWidth = min (refWidth, refHeight);
		mRefHeight = max (refWidth, refHeight);
	}

	InitProjection (width, height);
}

void Game::Shutdown () {
	SetCurrentScene (nullptr);
}

void Game::Resize (int newWidth, int newHeight) {
	int refWidth = mRefWidth;
	int refHeight = mRefHeight;
	if (newWidth > newHeight) {
		mRefWidth = max (refWidth, refHeight);
		mRefHeight = min (refWidth, refHeight);
	} else {
		mRefWidth = min (refWidth, refHeight);
		mRefHeight = max (refWidth, refHeight);
	}

	InitProjection (newWidth, newHeight);

	if (mCurrentScene != nullptr)
		mCurrentScene->Resize (newWidth, newHeight);
}

void Game::Update (float elapsedTime) {
	if (mCurrentScene != nullptr)
		mCurrentScene->Update (elapsedTime);
}

void Game::Render () {
	if (mCurrentScene != nullptr)
		mCurrentScene->Render ();
}

void Game::SetCurrentScene (shared_ptr<Scene> scene) {
	if (mCurrentScene != nullptr) {
		mCurrentScene->Shutdown ();
		mCurrentScene = nullptr;
	}

	if (scene != nullptr) {
		mCurrentScene = scene;
		mCurrentScene->Init (mWidth, mHeight);
	}
}

void Game::TouchDown (int fingerID, float x, float y) {
	if (mCurrentScene != nullptr)
		mCurrentScene->TouchDown (fingerID, x, y);
}

void Game::TouchUp (int fingerID, float x, float y) {
	if (mCurrentScene != nullptr)
		mCurrentScene->TouchUp (fingerID, x, y);
}

void Game::TouchMove (int fingerID, float x, float y) {
	if (mCurrentScene != nullptr)
		mCurrentScene->TouchMove (fingerID, x, y);
}

Vector2D Game::ToLocal (float x, float y) const {
	float widthRatio = 1.0f; //TODO: ... (float) mWidth / (float) mRefWidth;
	float heightRatio = 1.0f; //TODO: ... (float) mHeight / (float) mRefHeight;

	float min = (float) std::min (mWidth, mHeight);
	float max = (float) std::max (mWidth, mHeight);
	float aspect = max / min;
	if (mWidth <= mHeight) {
		return Vector2D (x / (float) mWidth * widthRatio, aspect * y / (float) mHeight * heightRatio);
	}

	return Vector2D (aspect * x / (float) mWidth * widthRatio, y / (float) mHeight * heightRatio);
}

void Game::InitProjection (int width, int height) {
	mWidth = width;
	mHeight = height;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();

	float min = (float) std::min (width, height);
	float max = (float) std::max (width, height);
	float aspect = max / min;
	if (width <= height) {
		glOrthof (0, 1.0f, aspect, 0, -1.0f, 1.0f);
	} else {
		glOrthof (0, aspect, 1.0f, 0, -1.0f, 1.0f);
	}
}