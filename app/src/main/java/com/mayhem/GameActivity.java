package com.mayhem;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.io.FileInputStream;
import java.io.FileOutputStream;

public class GameActivity extends Activity {
	GLView mView;
	Thread mEmulatorThread;

	TextView mStatusLine;

	static {
		System.loadLibrary ("c64emu");
		System.loadLibrary ("game");
	}

	@Override
	protected void onCreate (Bundle icicle) {
		super.onCreate (icicle);
		init (getAssets ());

		mEmulatorThread = new Thread ("c64_emulator") {
			@Override
			public void run () {
				GameLib.runEmulator ();
			}
		};

		mView = new GLView (getApplication (), mEmulatorThread);
		setContentView (mView);

		View overlayView = getLayoutInflater ().inflate (R.layout.sample_overlay_view, null);
		ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams (ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
		addContentView (overlayView, layoutParams);

		mStatusLine = (TextView) findViewById (R.id.status_text);

		//Setup game overlays

		//Setup adMob overlay
		if (isLite ()) {
//			_adView = new AdView(this);
//			_adView.setAdUnitId("ca-app-pub-9778250992563662/1847669238"); //zlaki.games@gmail.com - Ball Fall Top Banner
//			if (isTablet()) {
//				_adView.setAdSize(AdSize.LEADERBOARD);
//			} else {
//				_adView.setAdSize(AdSize.BANNER);
//			}
//
//			AdRequest adRequest = new AdRequest.Builder()
//					//.addTestDevice("54F567A97CA221C8AF6DC24725DD98A9") //Nexus 6 phone
//					//.addTestDevice("657B606D88C7789A95533151364832AC") //Nexus 9 tablet
//					.build();
//			_adView.loadAd(adRequest);
		}

		//Init Game Environment
		if (!GameLib.initEnvironment (getApplication ())) {
			//TODO: ... valami alert, mert nem tudunk elindulni ...
			return;
		}
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

	@Override
	public void onConfigurationChanged (Configuration newConfig) {
		super.onConfigurationChanged (newConfig);
		//... Nothing to do ...
	}

	private native void init (AssetManager assetManager);
	private native boolean isLite ();

	//region AndroidContentManager functions
	public void initAdMob () {
		if (isLite ()) {
			//...
		}
	}

	public String readFile (String fileName) {
		String content = "";
		FileInputStream inputStream = null;
		try {
			inputStream = openFileInput (fileName);

			StringBuffer fileContent = new StringBuffer ("");
			byte[] buffer = new byte[1024];

			int n = 0;
			while ((n = inputStream.read (buffer)) != -1) {
				fileContent.append (new String (buffer, 0, n));
			}

			content = fileContent.toString ();
		} catch (Exception e) {
			e.printStackTrace ();
		} finally {
			try {
				if (inputStream != null)
					inputStream.close ();
			} catch (Exception e) {
				e.printStackTrace ();
			}
		}

		return content;
	}

	public void writeFile (String fileName, String content) {
		FileOutputStream outputStream = null;
		try {
			outputStream = openFileOutput (fileName, Context.MODE_PRIVATE);
			outputStream.write (content.getBytes ());
		} catch (Exception e) {
			e.printStackTrace ();
		} finally {
			try {
				if (outputStream != null)
					outputStream.close ();
			} catch (Exception e) {
				e.printStackTrace ();
			}
		}
	}

	public void displayStatus (final String status) {
		runOnUiThread (new Runnable () {
						   @Override
						   public void run () {
							   mStatusLine.setText (status);
						   }
					   }
		);
	}
	//endregion

	//region Common Helper functions
	public static int dpToPx (int dp) {
		return (int) (dp * Resources.getSystem ().getDisplayMetrics ().density + 0.5f);
	}

	public static int pxToDp (int px) {
		return (int) (px / Resources.getSystem ().getDisplayMetrics ().density + 0.5f);
	}
	//endregion

	//region Hiding and showing navigation buttons
	// This snippet hides the system bars.
	private void hideSystemUI () {
		// Set the IMMERSIVE flag.
		// Set the content to appear under the system bars so that the content
		// doesn't resize when the system bars hide and show.
		mView.setSystemUiVisibility (
				View.SYSTEM_UI_FLAG_LAYOUT_STABLE
						| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
						| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
						| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
						| View.SYSTEM_UI_FLAG_FULLSCREEN // hide status bar
						| View.SYSTEM_UI_FLAG_IMMERSIVE);
	}

	// This snippet shows the system bars. It does this by removing all the flags
	// except for the ones that make the content appear under the system bars.
	private void showSystemUI () {
		mView.setSystemUiVisibility (
				View.SYSTEM_UI_FLAG_LAYOUT_STABLE
						| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
						| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
	}

	@Override
	public void onWindowFocusChanged (boolean hasFocus) {
		super.onWindowFocusChanged (hasFocus);
//		if (hasFocus) {
//			mView.setSystemUiVisibility (
//					View.SYSTEM_UI_FLAG_LAYOUT_STABLE
//							| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
//							| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
//							| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
//							| View.SYSTEM_UI_FLAG_FULLSCREEN
//							| View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
//		}
	}
	//endregion
}
