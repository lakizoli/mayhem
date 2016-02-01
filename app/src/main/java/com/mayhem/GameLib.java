package com.mayhem;

public class GameLib {
	public static native void init (int width, int height, int refWidth, int refHeight);
	public static native void step ();

	public static native boolean hasPointerID (int id);
	public static native void insertPointerID (int id);
	public static native void erasePointerID (int id);

	public static native void touchDown (int id, float x, float y);
	public static native void touchUp (int id, float x, float y);
	public static native void touchMove (int id, float x, float y);
}
