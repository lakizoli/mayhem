package com.mayheminmonsterland;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.Log;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

class GLView extends GLSurfaceView {
	private static String TAG = "GLView";
	private static final boolean DEBUG = false;

	public GLView (Context context, Thread emulatorThread, int deviceSampleRate, int deviceBufferFrames, int deviceBufferCount) {
		super (context);
		init (emulatorThread, deviceSampleRate, deviceBufferFrames, deviceBufferCount, false, 0, 0);
	}

	public GLView (Context context, Thread emulatorThread, int deviceSampleRate, int deviceBufferFrames, int deviceBufferCount, boolean translucent, int depth, int stencil) {
		super (context);
		init (emulatorThread, deviceSampleRate, deviceBufferFrames, deviceBufferCount, translucent, depth, stencil);
	}

	//region init functions
	private void init (Thread emulatorThread, int deviceSampleRate, int deviceBufferFrames, int deviceBufferCount, boolean translucent, int depth, int stencil) {
		setKeepScreenOn (true);

        /* By default, GLSurfaceView() creates a RGB_565 opaque surface.
		 * If we want a translucent one, we should change the surface's
         * format here, using PixelFormat.TRANSLUCENT for GL Surfaces
         * is interpreted as any 32-bit surface with alpha by SurfaceFlinger.
         */
		if (translucent) {
			this.getHolder ().setFormat (PixelFormat.TRANSLUCENT);
		}

        /* Setup the context factory for 2.0 rendering.
         * See ContextFactory class definition below
         */
		setEGLContextFactory (new ContextFactory ());

        /* We need to choose an EGLConfig that matches the format of
         * our surface exactly. This is going to be done in our
         * custom config chooser. See ConfigChooser class definition
         * below.
         */
		setEGLConfigChooser (translucent ?
				new ConfigChooser (8, 8, 8, 8, depth, stencil) :
				new ConfigChooser (5, 6, 5, 0, depth, stencil));

        /* Set the renderer responsible for frame rendering */
		setRenderer (new Renderer (emulatorThread, deviceSampleRate, deviceBufferFrames, deviceBufferCount));
	}

	private static class ContextFactory implements GLSurfaceView.EGLContextFactory {
		private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;

		public EGLContext createContext (EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
			Log.w (TAG, "creating OpenGL ES 1 context");
			checkEglError ("Before eglCreateContext", egl);
			int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 1, EGL10.EGL_NONE};
			EGLContext context = egl.eglCreateContext (display, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);
			checkEglError ("After eglCreateContext", egl);
			return context;
		}

		public void destroyContext (EGL10 egl, EGLDisplay display, EGLContext context) {
			egl.eglDestroyContext (display, context);
		}
	}

	private static void checkEglError (String prompt, EGL10 egl) {
		int error;
		while ((error = egl.eglGetError ()) != EGL10.EGL_SUCCESS) {
			Log.e (TAG, String.format ("%s: EGL error: 0x%x", prompt, error));
		}
	}

	private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

		public ConfigChooser (int r, int g, int b, int a, int depth, int stencil) {
			mRedSize = r;
			mGreenSize = g;
			mBlueSize = b;
			mAlphaSize = a;
			mDepthSize = depth;
			mStencilSize = stencil;
		}

		/* This EGL config specification is used to specify 2.0 rendering.
		 * We use a minimum size of 4 bits for red/green/blue, but will
		 * perform actual matching in chooseConfig() below.
		 */
		private static int EGL_OPENGL_ES2_BIT = 4;
		private static int[] s_configAttribs2 =
				{
						EGL10.EGL_RED_SIZE, 4,
						EGL10.EGL_GREEN_SIZE, 4,
						EGL10.EGL_BLUE_SIZE, 4,
						EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
						EGL10.EGL_NONE
				};

		public EGLConfig chooseConfig (EGL10 egl, EGLDisplay display) {

            /* Get the number of minimally matching EGL configurations
             */
			int[] num_config = new int[1];
			egl.eglChooseConfig (display, s_configAttribs2, null, 0, num_config);

			int numConfigs = num_config[0];

			if (numConfigs <= 0) {
				throw new IllegalArgumentException ("No configs match configSpec");
			}

            /* Allocate then read the array of minimally matching EGL configs
             */
			EGLConfig[] configs = new EGLConfig[numConfigs];
			egl.eglChooseConfig (display, s_configAttribs2, configs, numConfigs, num_config);

			if (DEBUG) {
				printConfigs (egl, display, configs);
			}
            /* Now return the "best" one
             */
			return chooseConfig (egl, display, configs);
		}

		public EGLConfig chooseConfig (EGL10 egl, EGLDisplay display,
									   EGLConfig[] configs) {
			for (EGLConfig config : configs) {
				int d = findConfigAttrib (egl, display, config,
						EGL10.EGL_DEPTH_SIZE, 0);
				int s = findConfigAttrib (egl, display, config,
						EGL10.EGL_STENCIL_SIZE, 0);

				// We need at least mDepthSize and mStencilSize bits
				if (d < mDepthSize || s < mStencilSize)
					continue;

				// We want an *exact* match for red/green/blue/alpha
				int r = findConfigAttrib (egl, display, config,
						EGL10.EGL_RED_SIZE, 0);
				int g = findConfigAttrib (egl, display, config,
						EGL10.EGL_GREEN_SIZE, 0);
				int b = findConfigAttrib (egl, display, config,
						EGL10.EGL_BLUE_SIZE, 0);
				int a = findConfigAttrib (egl, display, config,
						EGL10.EGL_ALPHA_SIZE, 0);

				if (r == mRedSize && g == mGreenSize && b == mBlueSize && a == mAlphaSize)
					return config;
			}
			return null;
		}

		private int findConfigAttrib (EGL10 egl, EGLDisplay display,
									  EGLConfig config, int attribute, int defaultValue) {

			if (egl.eglGetConfigAttrib (display, config, attribute, mValue)) {
				return mValue[0];
			}
			return defaultValue;
		}

		private void printConfigs (EGL10 egl, EGLDisplay display,
								   EGLConfig[] configs) {
			int numConfigs = configs.length;
			Log.w (TAG, String.format ("%d configurations", numConfigs));
			for (int i = 0; i < numConfigs; i++) {
				Log.w (TAG, String.format ("Configuration %d:\n", i));
				printConfig (egl, display, configs[i]);
			}
		}

		private void printConfig (EGL10 egl, EGLDisplay display,
								  EGLConfig config) {
			int[] attributes = {
					EGL10.EGL_BUFFER_SIZE,
					EGL10.EGL_ALPHA_SIZE,
					EGL10.EGL_BLUE_SIZE,
					EGL10.EGL_GREEN_SIZE,
					EGL10.EGL_RED_SIZE,
					EGL10.EGL_DEPTH_SIZE,
					EGL10.EGL_STENCIL_SIZE,
					EGL10.EGL_CONFIG_CAVEAT,
					EGL10.EGL_CONFIG_ID,
					EGL10.EGL_LEVEL,
					EGL10.EGL_MAX_PBUFFER_HEIGHT,
					EGL10.EGL_MAX_PBUFFER_PIXELS,
					EGL10.EGL_MAX_PBUFFER_WIDTH,
					EGL10.EGL_NATIVE_RENDERABLE,
					EGL10.EGL_NATIVE_VISUAL_ID,
					EGL10.EGL_NATIVE_VISUAL_TYPE,
					0x3030, // EGL10.EGL_PRESERVED_RESOURCES,
					EGL10.EGL_SAMPLES,
					EGL10.EGL_SAMPLE_BUFFERS,
					EGL10.EGL_SURFACE_TYPE,
					EGL10.EGL_TRANSPARENT_TYPE,
					EGL10.EGL_TRANSPARENT_RED_VALUE,
					EGL10.EGL_TRANSPARENT_GREEN_VALUE,
					EGL10.EGL_TRANSPARENT_BLUE_VALUE,
					0x3039, // EGL10.EGL_BIND_TO_TEXTURE_RGB,
					0x303A, // EGL10.EGL_BIND_TO_TEXTURE_RGBA,
					0x303B, // EGL10.EGL_MIN_SWAP_INTERVAL,
					0x303C, // EGL10.EGL_MAX_SWAP_INTERVAL,
					EGL10.EGL_LUMINANCE_SIZE,
					EGL10.EGL_ALPHA_MASK_SIZE,
					EGL10.EGL_COLOR_BUFFER_TYPE,
					EGL10.EGL_RENDERABLE_TYPE,
					0x3042 // EGL10.EGL_CONFORMANT
			};
			String[] names = {
					"EGL_BUFFER_SIZE",
					"EGL_ALPHA_SIZE",
					"EGL_BLUE_SIZE",
					"EGL_GREEN_SIZE",
					"EGL_RED_SIZE",
					"EGL_DEPTH_SIZE",
					"EGL_STENCIL_SIZE",
					"EGL_CONFIG_CAVEAT",
					"EGL_CONFIG_ID",
					"EGL_LEVEL",
					"EGL_MAX_PBUFFER_HEIGHT",
					"EGL_MAX_PBUFFER_PIXELS",
					"EGL_MAX_PBUFFER_WIDTH",
					"EGL_NATIVE_RENDERABLE",
					"EGL_NATIVE_VISUAL_ID",
					"EGL_NATIVE_VISUAL_TYPE",
					"EGL_PRESERVED_RESOURCES",
					"EGL_SAMPLES",
					"EGL_SAMPLE_BUFFERS",
					"EGL_SURFACE_TYPE",
					"EGL_TRANSPARENT_TYPE",
					"EGL_TRANSPARENT_RED_VALUE",
					"EGL_TRANSPARENT_GREEN_VALUE",
					"EGL_TRANSPARENT_BLUE_VALUE",
					"EGL_BIND_TO_TEXTURE_RGB",
					"EGL_BIND_TO_TEXTURE_RGBA",
					"EGL_MIN_SWAP_INTERVAL",
					"EGL_MAX_SWAP_INTERVAL",
					"EGL_LUMINANCE_SIZE",
					"EGL_ALPHA_MASK_SIZE",
					"EGL_COLOR_BUFFER_TYPE",
					"EGL_RENDERABLE_TYPE",
					"EGL_CONFORMANT"
			};
			int[] value = new int[1];
			for (int i = 0; i < attributes.length; i++) {
				int attribute = attributes[i];
				String name = names[i];
				if (egl.eglGetConfigAttrib (display, config, attribute, value)) {
					Log.w (TAG, String.format ("  %s: %d\n", name, value[0]));
				} else {
					// Log.w(TAG, String.format("  %s: failed\n", name));
					while (egl.eglGetError () != EGL10.EGL_SUCCESS) ;
				}
			}
		}

		// Subclasses can adjust these values:
		protected int mRedSize;
		protected int mGreenSize;
		protected int mBlueSize;
		protected int mAlphaSize;
		protected int mDepthSize;
		protected int mStencilSize;
		private int[] mValue = new int[1];
	}
	//endregion

	private static class Renderer implements GLSurfaceView.Renderer {
		private Thread mEmulatorThread;
		private int mDeviceSampleRate;
		private int mDeviceBufferFrames;
		private int mDeviceBufferCount;

		public Renderer (Thread emulatorThread, int deviceSampleRate, int deviceBufferFrames, int deviceBufferCount) {
			super ();
			mEmulatorThread = emulatorThread;
			mDeviceSampleRate = deviceSampleRate;
			mDeviceBufferFrames = deviceBufferFrames;
			mDeviceBufferCount = deviceBufferCount;
		}

		public void onDrawFrame (GL10 gl) {
			GameLib.step ();
		}

		public void onSurfaceChanged (GL10 gl, int width, int height) {
			if (GameLib.isInited ()) {
				if (GameLib.isPaused ())
					GameLib.resume ();
				GameLib.resize (width, height);
			} else { //First call
				int refWidth = width > height ? 2392 : 1440;
				int refHeight = width > height ? 1440 : 2392;
				GameLib.init (width, height, refWidth, refHeight, mDeviceSampleRate, mDeviceBufferFrames, mDeviceBufferCount);

				//Starting the c64 emulator in a background thread
				mEmulatorThread.start ();
			}
		}

		public void onSurfaceCreated (GL10 gl, EGLConfig config) {
			// Do nothing.
		}
	}

	//region Pause / Continue handlers
	@Override
	public void onPause () {
		if (GameLib.isInited ()) {
			GameLib.pause ();
		}

		super.onPause ();
	}

	@Override
	public void onResume () {
		super.onResume ();
	}
	//endregion

	//region touch handlers
	@Override
	public boolean onTouchEvent (MotionEvent event) {
		int action = event.getAction ();
		int pointerIndex = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
//		Log.d ("Mayhem", String.format ("OnTouchEvent (Raw) -> action: %d, idx: %d, id: %d, x: %.2f, y: %.2f", action, pointerIndex, event.getPointerId (pointerIndex), event.getX (pointerIndex), event.getY (pointerIndex)));
		if (action == MotionEvent.ACTION_DOWN || (action & MotionEvent.ACTION_POINTER_DOWN) == MotionEvent.ACTION_POINTER_DOWN) {
			int id = event.getPointerId (pointerIndex);
			if (!GameLib.hasPointerID (id)) {
//				Log.d ("Mayhem", String.format ("OnTouchEvent (Down) -> idx: %d, id: %d, x: %.2f, y: %.2f", pointerIndex, id, event.getX (pointerIndex), event.getY (pointerIndex)));
				GameLib.insertPointerID (id);
				GameLib.touchDown (id, event.getX (pointerIndex), event.getY (pointerIndex));
			}
		} else if (action == MotionEvent.ACTION_UP || (action & MotionEvent.ACTION_POINTER_UP) == MotionEvent.ACTION_POINTER_UP) {
			int id = event.getPointerId (pointerIndex);
			if (GameLib.hasPointerID (id)) {
//				Log.d ("Mayhem", String.format ("OnTouchEvent (Up) -> idx: %d, id: %d, x: %.2f, y: %.2f", pointerIndex, id, event.getX (pointerIndex), event.getY (pointerIndex)));
				GameLib.erasePointerID (id);
				GameLib.touchUp (id, event.getX (pointerIndex), event.getY (pointerIndex));
			}
		} else if (action == MotionEvent.ACTION_MOVE) {
			for (int i = 0, iEnd = event.getPointerCount (); i < iEnd; ++i) {
				int id = event.getPointerId (i);
				if (GameLib.hasPointerID (id)) {
//					Log.d ("Mayhem", String.format ("OnTouchEvent (Move) -> idx: %d, id: %d, x: %.2f, y: %.2f", pointerIndex, id, event.getX (pointerIndex), event.getY (pointerIndex)));
					GameLib.touchMove (id, event.getX (i), event.getY (i));
				}
			}
		}

		return true; //super.onTouchEvent (event);
	}
	//endregion
}
