package com.mayhem;

public class GameLib {
	public static native void init (int width, int height, int refWidth, int refHeight);
	public static native void step ();
}
