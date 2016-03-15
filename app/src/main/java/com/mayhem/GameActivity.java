package com.mayheminmonsterland;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;

public class GameActivity extends Activity {
	GLView mView;
	Thread mEmulatorThread;

	FrameLayout mAdFrameLayout;
	AdView mAdView;

	TextView mStatusLine;

	static {
		System.loadLibrary ("c64emu");
		System.loadLibrary ("game");
	}

	@Override
	protected void onCreate (Bundle bundle) {
		super.onCreate (bundle);
		init (getAssets ());

		int deviceSampleRate = 0;
		AudioManager audioManager = (AudioManager) getSystemService (Context.AUDIO_SERVICE);
		if (Build.VERSION.SDK_INT >= 17) {
			String sampleRateValue = audioManager.getProperty (AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
			deviceSampleRate = Integer.parseInt (sampleRateValue == null ? "0" : sampleRateValue);
		}

		mEmulatorThread = new Thread ("c64_emulator") {
			@Override
			public void run () {
				GameLib.runEmulator ();
			}
		};

		mView = new GLView (getApplication (), mEmulatorThread, deviceSampleRate);
		setContentView (mView);

		View overlayView = getLayoutInflater ().inflate (R.layout.sample_overlay_view, null);
		ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams (ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
		addContentView (overlayView, layoutParams);

		mAdFrameLayout = (FrameLayout) findViewById (R.id.game_adframe);
		mStatusLine = (TextView) findViewById (R.id.status_text);

		hideSystemUI ();

		//Setup adMob overlay
		if (isLite ()) {
			int screenSize = getResources ().getConfiguration ().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK;
			boolean isTablet = screenSize >= Configuration.SCREENLAYOUT_SIZE_LARGE;

			mAdView = new AdView (this);
			mAdView.setAdUnitId ("ca-app-pub-9778250992563662/3524377634"); //zlaki.games@gmail.com - Mayhem In Monsterland Top Banner
			mAdView.setAdSize (isTablet ? AdSize.LEADERBOARD : AdSize.BANNER);
			mAdFrameLayout.addView (mAdView);

			AdRequest adRequest = new AdRequest.Builder ()
//					.addTestDevice ("75313E1E3C419C226D4DD43C42C2C483") //Samsung Galaxy S4
//					.addTestDevice ("54F567A97CA221C8AF6DC24725DD98A9") //Nexus 6 phone
//					.addTestDevice ("657B606D88C7789A95533151364832AC") //Nexus 9 tablet
					.build ();
			mAdView.loadAd (adRequest);
		}

		//Init Game Environment
		if (!GameLib.initEnvironment (getApplication ())) {
			// ... valami alert kellhet, mert nem tudunk elindulni ...
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
	public String readTextFile (String fileName) {
		byte[] data = readFile (fileName);
		if (data != null)
			return new String (data, 0, data.length);
		return "";
	}

	public void writeTextFile (String fileName, String content, boolean append) {
		writeFile (fileName, content.getBytes (), append);
	}

	public byte[] readFile (String fileName) {
		byte[] content = null;
		FileInputStream inputStream = null;
		try {
			inputStream = openFileInput (fileName);
			long size = inputStream.getChannel ().size ();

			ByteBuffer fileContent = ByteBuffer.allocate ((int)size);
			byte[] buffer = new byte[1024];

			int n;
			while ((n = inputStream.read (buffer)) != -1) {
				fileContent.put (buffer, 0, n);
			}

			content = fileContent.array ();
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

	public void writeFile (String fileName, byte[] content, boolean append) {
		FileOutputStream outputStream = null;
		try {
			outputStream = openFileOutput (fileName, append ? Context.MODE_APPEND : Context.MODE_PRIVATE);
			outputStream.write (content);
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
		});
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
	/**
	 * This snippet hides the system bars.
	 */
	private void hideSystemUI () {
		// Set the IMMERSIVE flag.
		// Set the content to appear under the system bars so that the content
		// doesn't resize when the system bars hide and show.
		if (Build.VERSION.SDK_INT >= 16) {
			int visibility = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
				| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION // hide nav bar
				| View.SYSTEM_UI_FLAG_FULLSCREEN; // hide status bar

			if (Build.VERSION.SDK_INT >= 19)
				visibility |= View.SYSTEM_UI_FLAG_IMMERSIVE;

			mView.setSystemUiVisibility (visibility);
		}
	}

	/**
	 * This snippet shows the system bars. It does this by removing all the flags
	 * except for the ones that make the content appear under the system bars.
	 */
	private void showSystemUI () {
		if (Build.VERSION.SDK_INT >= 16) {
			mView.setSystemUiVisibility (
					View.SYSTEM_UI_FLAG_LAYOUT_STABLE
							| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
							| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
		}
	}

	@Override
	public void onWindowFocusChanged (boolean hasFocus) {
		super.onWindowFocusChanged (hasFocus);
		if (Build.VERSION.SDK_INT >= 16 && hasFocus) {
			int visibility =  View.SYSTEM_UI_FLAG_LAYOUT_STABLE
				| View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
				| View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
				| View.SYSTEM_UI_FLAG_FULLSCREEN;

			if (Build.VERSION.SDK_INT >= 19)
				visibility |= View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

			mView.setSystemUiVisibility (visibility);
		}
	}
	//endregion
}
