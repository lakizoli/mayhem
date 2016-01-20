package com.mayhem;

public class GameLib {
	static {
		System.loadLibrary ("game");
	}

	public static native void init (int width, int height);
	public static native void step ();
}
