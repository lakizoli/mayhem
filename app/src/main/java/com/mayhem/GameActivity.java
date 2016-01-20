package com.mayhem;

import android.app.Activity;
import android.os.Bundle;

public class GameActivity extends Activity {
	GLView mView;

	@Override
	protected void onCreate (Bundle icicle) {
		super.onCreate (icicle);
		mView = new GLView (getApplication ());
		setContentView (mView);
	}

	@Override
	protected void onPause () {
		super.onPause ();
		mView.onPause ();
	}

	@Override
	protected void onResume () {
		super.onResume ();
		mView.onResume ();
	}
}
