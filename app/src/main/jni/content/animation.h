#pragma once

class Animation {
private:
	bool mIsStarted;
	bool mIsPaused;

protected:
	float mTimeOffset;

protected:
	Animation () : mIsStarted (false), mIsPaused (false), mTimeOffset (0.0f) {}

public:
	void Start ();
	void Stop ();
	bool IsStarted () const {
		return mIsStarted;
	}

	void Pause ();
	void Continue ();
	bool IsPaused () const {
		return mIsPaused;
	}

	void Update (float elapsedTime);

protected:
	virtual void OnStart () {}
	virtual void OnStop () {}
	virtual void OnPause () {}
	virtual void OnContinue () {}
	virtual void OnUpdate (float elapsedTime) {}
};

class PulseAnimation : public Animation {
private:
	float mTopScale;
	float mTimeFrame;

	float _scale;

public:
	PulseAnimation (float topScale, float timeFrame) : mTopScale (topScale), mTimeFrame (timeFrame), _scale (1.0f) {}

	float Scale () const {
		return _scale;
	}

protected:
	virtual void OnUpdate (float elapsedTime) override;
};

class LinearAnimation : public Animation {
private:
	float mStartValue;
	float mEndValue;
	float mTimeFrame;

	float mValue;

public:
	LinearAnimation (float startValue, float endValue, float timeFrame) :
		mStartValue (startValue),
		mEndValue (endValue),
		mTimeFrame (timeFrame),
		mValue (startValue) {
	}

	float Value () const {
		return mValue;
	}

protected:
	virtual void OnUpdate (float elapsedTime) override;
};

class FrameAnimation : public Animation {
private:
	uint32_t mFrame;
	float mFrameTime;

public:
	FrameAnimation (float frameTime) : mFrameTime (frameTime) {}

	uint32_t Frame () const {
		return mFrame;
	}

protected:
	virtual void OnUpdate (float elapsedTime) override;
};
