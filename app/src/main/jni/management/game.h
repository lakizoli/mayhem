#pragma once

#include "IUtil.h"
#include "IContentManager.h"
#include "scene.h"
#include "../content/vector2D.h"

/// Base class of the game.
class Game {
//Data 
private:
	static Game* mGame;
	IUtil& mUtil;
	IContentManager& mContentManager;

	int mWidth;
	int mHeight;

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
	virtual void Init (int width, int height, int refWidth, int refHeight);
	virtual void Shutdown ();

	void Resize (int newWidth, int newHeight);

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
	virtual void TouchDown (int fingerID, float x, float y);
	virtual void TouchUp (int fingerID, float x, float y);
	virtual void TouchMove (int fingerID, float x, float y);

//Helpers
public:
	float ScreenWidth () const {
		return ToLocal (mWidth, 0).x;
	}

	float ScreenHeight () const {
		return ToLocal (0, mHeight).y;
	}

	float WidthRatio () const {
		return (float) mWidth / (float) mRefWidth;
	}

	float HeightRatio () const {
		return (float) mHeight / (float) mRefHeight;
	}

	Vector2D ToLocal (float x, float y) const;

//Inner methods
private:
	void InitProjection (int width, int height);
};
