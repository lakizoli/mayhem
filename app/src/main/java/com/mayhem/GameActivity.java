package com.mayhem;

import android.app.Activity;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Bundle;

import java.io.FileInputStream;
import java.io.FileOutputStream;

public class GameActivity extends Activity {
	GLView mView;

	static {
		System.loadLibrary ("c64emu");
		System.loadLibrary ("game");
	}

	@Override
	protected void onCreate (Bundle icicle) {
		super.onCreate (icicle);
		init (getAssets ());

		mView = new GLView (getApplication ());
		setContentView (mView);

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

			StringBuffer fileContent = new StringBuffer("");
			byte[] buffer = new byte[1024];

			int n = 0;
			while ((n = inputStream.read(buffer)) != -1) {
				fileContent.append(new String(buffer, 0, n));
			}

			content = fileContent.toString ();
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				if (inputStream != null)
					inputStream.close ();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}

		return content;
	}

	public void writeFile (String fileName, String content) {
		FileOutputStream outputStream = null;
		try {
			outputStream = openFileOutput(fileName, Context.MODE_PRIVATE);
			outputStream.write(content.getBytes());
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				if (outputStream != null)
					outputStream.close ();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
	}
	//endregion
}
