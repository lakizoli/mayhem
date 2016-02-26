#include "../pch.h"
#include "animation.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Animation implementation
//////////////////////////////////////////////////////////////////////////////////////////
void Animation::Start () {
	if (mIsStarted)
		return;

	mIsStarted = true;
	mIsPaused = false;
	mTimeOffset = 0;

	OnStart ();
}

void Animation::Stop () {
	if (!mIsStarted)
		return;

	mIsStarted = false;
	mIsPaused = false;
	mTimeOffset = 0;

	OnStop ();
}

void Animation::Pause () {
	if (!mIsStarted || mIsPaused)
		return;

	mIsPaused = true;

	OnPause ();
}

void Animation::Continue () {
	if (!mIsStarted || !mIsPaused)
		return;

	mIsPaused = false;

	OnContinue ();
}

void Animation::Update (float elapsedTime) {
	if (!mIsStarted || mIsPaused)
		return;

	mTimeOffset += elapsedTime;

	OnUpdate (elapsedTime);
}

//////////////////////////////////////////////////////////////////////////////////////////
// PulseAnimation implementation
//////////////////////////////////////////////////////////////////////////////////////////
void PulseAnimation::OnUpdate (float elapsedTime) {
	Animation::OnUpdate (elapsedTime);

	float offsetInFrame = fmodf (mTimeOffset, mTimeFrame);
	float halfTime = mTimeFrame / 2.0f;

	if (offsetInFrame >= mTimeFrame / 2.0f) //go down with scale
		_scale = (mTopScale - 1.0f) * (2.0f * halfTime - offsetInFrame) / halfTime + 1.0f;
	else //go up with scale
		_scale = (mTopScale - 1.0f) * offsetInFrame / halfTime + 1.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////
// LinearAnimation implementation
//////////////////////////////////////////////////////////////////////////////////////////
void LinearAnimation::OnUpdate (float elapsedTime) {
	Animation::OnUpdate (elapsedTime);

	if (mTimeOffset >= mTimeFrame) //Ended
		mValue = mEndValue;
	else //interpolate
		mValue = mStartValue + mTimeOffset / mTimeFrame * (mEndValue - mStartValue);
}

//////////////////////////////////////////////////////////////////////////////////////////
// FrameAnimation implementation
//////////////////////////////////////////////////////////////////////////////////////////
void FrameAnimation::OnUpdate (float elapsedTime) {
	Animation::OnUpdate (elapsedTime);
	mFrame = (uint32_t) (mTimeOffset / mFrameTime);
}
