#pragma once

#include "IUtil.h"
#include "IContentManager.h"
#include "scene.h"
#include "../content/vector2D.h"

///
/// Base class of the game.
///
/// Uses 3 coordinate system: (all 3 has the origin in top left corner!)
/// 1.) Screen system -> mScreenWidth, mScreenHeight (physical dimension of the screen)
/// 2.) Reference system -> mRefWidth, mRefHeight (physical dimension of the developer [reference] machine's screen)
/// 3.) Local system -> mWidth,  mHeight(floating point coordinates in OpenGL used for drawing and positioning [0..1, 0..aspect])
class Game {
//Data 
private:
	static Game* mGame;
	IUtil& mUtil;
	IContentManager& mContentManager;

	float mWidth;
	float mHeight;

	int mScreenWidth;
	int mScreenHeight;

	int mRefWidth;
	int mRefHeight;

	shared_ptr<Scene> mCurrentScene;

//Construction
protected:
	Game (IUtil& util, IContentManager& contentManager);

public:
	virtual ~Game () {
		mGame = nullptr;
	}

	static Game& Get () {
		assert (mGame != nullptr);
		return *mGame;
	}

	static IUtil& Util () {
		assert (mGame != nullptr);
		return mGame->mUtil;
	}

	static IContentManager& ContentManager () {
		assert (mGame != nullptr);
		return mGame->mContentManager;
	}

//Management interface
public:
	virtual void Init (int screenWidth, int screenHeight, int refWidth, int refHeight);
	virtual void Shutdown ();

	virtual void Pause ();
	virtual void Continue ();

	virtual void Resize (int newScreenWidth, int newScreenHeight);

	/// <summary>
	/// The update step of the game.
	/// </summary>
	/// <param name="elapsedTime">The elapsed time from the last update in seconds.</param>
	virtual void Update (float elapsedTime);
	virtual void Render ();

//Scene interface
public:
	shared_ptr<Scene> CurrentScene () const {
		return mCurrentScene;
	}

	void SetCurrentScene (shared_ptr<Scene> scene);

//Input handlers
public:
	virtual void TouchDown (int fingerID, float screenX, float screenY);
	virtual void TouchUp (int fingerID, float screenX, float screenY);
	virtual void TouchMove (int fingerID, float screenX, float screenY);

//Helpers
public:
	float Width () const { return mWidth; }
	float Height () const { return mHeight; }
	Vector2D Size () const { return Vector2D (mWidth, mHeight); }

	int ScreenWidth () const { return mScreenWidth; }
	int ScreenHeight () const { return mScreenHeight; }
	Vector2D ScreenSize () const { return Vector2D ((float)mScreenWidth, (float)mScreenHeight); }

	int RefWidth () const { return mRefWidth; }
	int RefHeight () const { return mRefHeight; }
	Vector2D RefSize () const { return Vector2D ((float)mRefWidth, (float)mRefHeight); }

	Vector2D LocalToScreen (float localX, float localY) const {
		return Vector2D (localX * mScreenWidth / mWidth, localY * mScreenHeight / mHeight);
	}

	Vector2D LocalToRef (float localX, float localY) const {
		return Vector2D (localX * mRefWidth / mWidth, localY * mRefHeight / mHeight);
	}

	Vector2D ScreenToLocal (float x, float y) const {
		return Vector2D (x * mWidth / mScreenWidth, y * mHeight / mScreenHeight);
	}

	Vector2D RefToLocal (float refX, float refY) const {
		return Vector2D (refX * mWidth / mRefWidth, refY * mHeight / mRefHeight);
	}

//Inner methods
private:
	void InitProjection (int screenWidth, int screenHeight);
};
