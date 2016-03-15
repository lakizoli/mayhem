package com.mayheminmonsterland;

import android.content.Context;
import android.content.res.AssetManager;
import android.support.annotation.NonNull;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class GameLib {
	private static String mDataPath;

	public static native void init (int screenWidth, int screenHeight, int refWidth, int refHeight, int deviceSamplingRate);
	public static native boolean isInited ();

	public static native void pause ();
	public static native void resume ();
	public static native boolean isPaused ();

	public static native void step ();
	public static native void resize (int newScreenWidth, int newScreenHeight);

	public static native boolean hasPointerID (int id);
	public static native void insertPointerID (int id);
	public static native void erasePointerID (int id);

	public static native void touchDown (int id, float x, float y);
	public static native void touchUp (int id, float x, float y);
	public static native void touchMove (int id, float x, float y);

	public static void runEmulator () {
		String exePath = combinePath (mDataPath, "x86.exe");
		String diskPath = combinePath (mDataPath, "game.d64");
		int res = runEmulator (exePath, diskPath);
		if (res < 0) {
			//...
		}
	}

	private static native int runEmulator (String exePath, String diskPath);

	//region Init Game Environment
	public static boolean initEnvironment (@NonNull Context context) {
		boolean succeded = true;

		//Create or get the base data directory
		File internalDir = context.getDir ("data", Context.MODE_PRIVATE);
		if (internalDir == null)
			succeded = false;

		//Copy data dir from .apk to internal storage
		if (succeded && !copyDir (context, "data", internalDir))
			succeded = false;

		//Store data path
		if (succeded) {
			mDataPath = internalDir.getAbsolutePath ();
		}

		return succeded;
	}

	private static boolean copyDir (@NonNull Context context, @NonNull String sourceDir, @NonNull File destDir) {
		try {
			boolean succeeded = true;

			//Get content of dir in assets folder
			AssetManager assetManager = context.getAssets ();
			String[] dirItems = assetManager.list (sourceDir);

			if (dirItems != null && dirItems.length > 0) {
				for (String item : dirItems) {
					String itemPath = combinePath (sourceDir, item);
					if (hasChildren (assetManager, itemPath)) { //Copy directory
						//Create dir if not exists
						String path = combinePath (destDir.getAbsolutePath (), item);
						File childDir = new File (path);
						if (!childDir.exists () && !childDir.mkdir ()) {
							succeeded = false;
							break;
						}

						//Copy dirs content
						if (!copyDir (context, itemPath, childDir)) {
							succeeded = false;
							break;
						}
					} else { //Copy file
						if (!copyFile (context, itemPath, destDir)) {
							succeeded = false;
							break;
						}
					}
				}
			}

			return succeeded;
		} catch (IOException ex) {
			ex.printStackTrace ();
		}

		return false;
	}

	private static boolean hasChildren (@NonNull AssetManager assetManager, @NonNull String path) {
		try {
			String[] children = assetManager.list (path);
			return children != null && children.length > 0;
		} catch (IOException ex) {
			//...
		}

		return false;
	}

	private static boolean copyFile (@NonNull Context context, @NonNull String sourceFile, @NonNull File destDir) throws IOException {
		//Combine path
		boolean succeeded = true;
		String fileName = extractFileName (sourceFile);
		String destFilePath = combinePath (destDir.getAbsolutePath (), fileName);

		//Delete the old file if exists
		File file = new File (destFilePath);
		if (file.exists () && !file.delete ())
			succeeded = false;

		//Copy the new file to destination
		if(succeeded) {
			AssetManager assetManager = context.getAssets ();
			InputStream inputStream = assetManager.open (sourceFile);
			FileOutputStream outputStream = new FileOutputStream (file);

			int read = 0;
			byte[] bytes = new byte[1024];
			while ((read = inputStream.read (bytes)) != -1) {
				outputStream.write (bytes, 0, read);
			}
		}

		return succeeded;
	}

	@NonNull
	private static String combinePath (@NonNull String path, @NonNull String item) {
		StringBuilder builder = new StringBuilder ();
		builder.append (path);

		if (path.length () > 0 && path.charAt (path.length () - 1) != '/')
			builder.append ('/');

		builder.append (item);

		return builder.toString ();
	}

	@NonNull
	private static String extractFileName (@NonNull String path) {
		int idx = path.lastIndexOf ('/');
		if (idx >= 0 && path.length () > idx + 1)
			return path.substring (idx + 1);
		return path;
	}
	//endregion
}
