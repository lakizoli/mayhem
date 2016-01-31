#pragma once

/// Base class of all quick time event.
class QTE {
private:
	bool mIsStarted;
	bool mIsPaused;

protected:
	QTE () : mIsStarted (false), mIsPaused (false) {}

public:
	virtual void Init () {}
	virtual void Shutdown () {}

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
	void Render ();

protected:
	virtual void OnStart () {}
	virtual void OnStop () {}
	virtual void OnPause () {}
	virtual void OnContinue () {}
	virtual void OnUpdate (float elapsedTime) {}
	virtual void OnRender () {}
};
