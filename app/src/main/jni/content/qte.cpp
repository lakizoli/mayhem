#include "../pch.h"
#include "qte.h"

void QTE::Start () {
	if (mIsStarted)
		return;

	mIsStarted = true;
	mIsPaused = false;

	OnStart ();
}

void QTE::Stop () {
	if (!mIsStarted)
		return;

	mIsStarted = false;
	mIsPaused = false;

	OnStop ();
}

void QTE::Pause () {
	if (!mIsStarted || mIsPaused)
		return;

	mIsPaused = true;

	OnPause ();
}

void QTE::Continue () {
	if (!mIsStarted || !mIsPaused)
		return;

	mIsPaused = false;

	OnContinue ();
}

void QTE::Update (float elapsedTime) {
	if (!mIsStarted || mIsPaused)
		return;

	OnUpdate (elapsedTime);
}

void QTE::Render () {
	if (!mIsStarted)
		return;

	OnRender ();
}
