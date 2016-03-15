#include "../pch.h"
#include "androidcontentmanager.h"
#include "../jnihelper/JavaString.h"
#include "audiomanager.h"

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/bitmap.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// JNI helper for AndroidContentManager class
////////////////////////////////////////////////////////////////////////////////////////////////////
class JNI_ContentManager {
	friend class AndroidContentManager;

	JNI_ContentManager () :
		mClassActivity (nullptr),
		mClassAsset (nullptr),
		mClassInputStream (nullptr),
		mClassBitmapFactory (nullptr),
		mGetAssetsMethod (nullptr),
		mOpenMethod (nullptr),
		mCloseMethod (nullptr),
		mDecodeStreamMethod (nullptr),
		mReadFileMethod (nullptr),
		mWriteFileMethod (nullptr),
		mDisplayStatusMethod (nullptr),
		mActivity (nullptr),
		mAssetManager (nullptr) {
	}

	~JNI_ContentManager () {
		Release ();
	}

	static JNI_ContentManager &Get (bool checkInit = true) {
		static JNI_ContentManager inst;
		if (checkInit && !inst.IsInited ())
			inst.Init ();
		return inst;
	}

	static void ReinitJNI () {
		JNI_ContentManager &jni = JNI_ContentManager::Get (); //Get reference for jni interface
		jni.Release (); //Release all of the stored interface data
		jni.Init (); //Reinitialize jni interface
	}

	bool IsInited () const {
		return mClassActivity != nullptr;
	}

	void Init () {
		if (IsInited ())
			return;

		//Find and reference Java class and collect needed methods
		JNI::AutoLocalRef <jclass> clazzActivity (JNI::FindClass ("com/mayheminmonsterland/GameActivity"));
		mClassActivity = JNI::GlobalReferenceObject (clazzActivity.get (), "com/mayheminmonsterland/GameActivity");

		JNI::AutoLocalRef <jclass> clazzAsset (JNI::FindClass ("android/content/res/AssetManager"));
		mClassAsset = JNI::GlobalReferenceObject (clazzAsset.get (), "android/content/res/AssetManager");

		JNI::AutoLocalRef <jclass> clazzInputStream (JNI::FindClass ("java/io/InputStream"));
		mClassInputStream = JNI::GlobalReferenceObject (clazzInputStream.get (), "java/io/InputStream");

		JNI::AutoLocalRef <jclass> clazzBitmapFactory (JNI::FindClass ("android/graphics/BitmapFactory"));
		mClassBitmapFactory = JNI::GlobalReferenceObject (clazzBitmapFactory.get (), "android/graphics/BitmapFactory");

		//Collect needed methods
		mGetAssetsMethod = JNI::GetMethod (clazzActivity, "getAssets", "()Landroid/content/res/AssetManager;");
		mOpenMethod = JNI::GetMethod (clazzAsset, "open", "(Ljava/lang/String;)Ljava/io/InputStream;");
		mCloseMethod = JNI::GetMethod (clazzInputStream, "close", "()V");
		mDecodeStreamMethod = JNI::GetStaticMethod (clazzBitmapFactory, "decodeStream", "(Ljava/io/InputStream;)Landroid/graphics/Bitmap;");

		mReadFileMethod = JNI::GetMethod (clazzActivity, "readFile", "(Ljava/lang/String;)Ljava/lang/String;");
		mWriteFileMethod = JNI::GetMethod (clazzActivity, "writeFile", "(Ljava/lang/String;Ljava/lang/String;)V");
		mDisplayStatusMethod = JNI::GetMethod (clazzActivity, "displayStatus", "(Ljava/lang/String;)V");
	}

	void Release () {
		if (!IsInited ())
			return;

		JNI::ReleaseGlobalReferencedObject (mClassBitmapFactory, "JNI_ContentManager::Release () - BitmapFactory");
		mClassBitmapFactory = nullptr;

		JNI::ReleaseGlobalReferencedObject (mClassInputStream, "JNI_ContentManager::Release () - InputStream");
		mClassInputStream = nullptr;

		JNI::ReleaseGlobalReferencedObject (mClassAsset, "JNI_ContentManager::Release () - Asset");
		mClassAsset = nullptr;

		JNI::ReleaseGlobalReferencedObject (mClassActivity, "JNI_ContentManager::Release () - Activity");
		mClassActivity = nullptr;

		mGetAssetsMethod = nullptr;
		mOpenMethod = nullptr;
		mCloseMethod = nullptr;
		mDecodeStreamMethod = nullptr;

		mReadFileMethod = nullptr;
		mWriteFileMethod = nullptr;
		mDisplayStatusMethod = nullptr;
	}

	//private:
	jclass mClassActivity; ///< The java class: com/mayheminmonsterland/GameActivity
	jclass mClassAsset; ///< The java class: android/content/res/AssetManager
	jclass mClassInputStream; ///< The java class: java/io/InputStream
	jclass mClassBitmapFactory; ///< The java class: android/graphics/BitmapFactory

	jmethodID mGetAssetsMethod; ///< The getAssets () method.
	jmethodID mOpenMethod; ///< The open () method.
	jmethodID mCloseMethod; ///< The close () method.
	jmethodID mDecodeStreamMethod; ///< The static decodeStream () method.

	jmethodID mReadFileMethod;
	jmethodID mWriteFileMethod;
	jmethodID mDisplayStatusMethod;

	jobject mActivity; ///< The java instance of the Activity.
	jobject mAssetManager; ///< The java instance of the AssetManager;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// AndroidContentManager implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
AndroidContentManager::AndroidContentManager (jobject activity, jobject assetManager) {
	CHECKMSG (activity != nullptr, "Activity reference cannot be nullptr!");
	CHECKMSG (assetManager != nullptr, "AssetManager reference cannot be nullptr!");

	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	jni.mActivity = JNI::GlobalReferenceObject (activity, "AndroidContentManager::Init () - activity");
	jni.mAssetManager = JNI::GlobalReferenceObject (assetManager, "AndroidContentManager::Init () - assetManager");

	AudioManager& audioManager = AudioManager::Get ();
	AAssetManager* man = AAssetManager_fromJava (JNI::GetEnv (), jni.mAssetManager);
	CHECKMSG (man != nullptr, "Native AAssetManager reference cannot be nullptr!");
	audioManager.Init (man);
}

AndroidContentManager::~AndroidContentManager () {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.Shutdown ();

	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNI::ReleaseGlobalReferencedObject (jni.mAssetManager, "AndroidContentManager::Init () - assetManager release");
	jni.mAssetManager = nullptr;

	JNI::ReleaseGlobalReferencedObject (jni.mActivity, "AndroidContentManager::Init () - activity release");
	jni.mActivity = nullptr;
}

Image AndroidContentManager::LoadImage (const string & asset) {
	return (Image) loadBitmap (asset);
}

void AndroidContentManager::UnloadImage (Image& image) {
	jobject jimg = (jobject) image;
	JNI::ReleaseLocalReferencedObject (jimg);
	image = nullptr;
}

const uint8_t * AndroidContentManager::LockPixels (Image image) {
	void* ptr = nullptr;
	AndroidBitmap_lockPixels (JNI::GetEnv (), (jobject) image, &ptr);
	return (const uint8_t *) ptr;
}

void AndroidContentManager::UnlockPixels (Image image) {
	AndroidBitmap_unlockPixels (JNI::GetEnv (), (jobject) image);
}

int AndroidContentManager::GetWidth (const Image image) const {
	AndroidBitmapInfo info;
	AndroidBitmap_getInfo (JNI::GetEnv (), (jobject) image, &info);
	return (int) info.width;
}

int AndroidContentManager::GetHeight (const Image image) const {
	AndroidBitmapInfo info;
	AndroidBitmap_getInfo (JNI::GetEnv (), (jobject) image, &info);
	return (int) info.height;
}

int AndroidContentManager::LoadSound (const string & asset) {
	AudioManager& audioManager = AudioManager::Get ();
	return audioManager.Load (asset);
}

void AndroidContentManager::UnloadSound (int soundID) {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.Unload (soundID);
}

void AndroidContentManager::PlaySound (int soundID, float volume, bool looped) {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.Play (soundID, volume, looped);
}

void AndroidContentManager::StopSound (int soundID) {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.Stop (soundID);
}

bool AndroidContentManager::IsSoundEnded (int soundID) const {
	AudioManager& audioManager = AudioManager::Get ();
	return audioManager.IsEnded (soundID);
}

void AndroidContentManager::OpenPCM (float volume, int numChannels, int sampleRate, int bytesPerSample, int deviceBufferFrames, int deviceBufferCount) {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.OpenPCM (volume, numChannels, sampleRate, bytesPerSample, deviceBufferFrames, deviceBufferCount);
}

void AndroidContentManager::ClosePCM () {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.ClosePCM ();
}

bool AndroidContentManager::IsOpenedPCM () const {
	AudioManager& audioManager = AudioManager::Get ();
	return audioManager.IsOpenedPCM ();
}

void AndroidContentManager::WritePCM (const uint8_t* buffer, size_t size) {
	AudioManager& audioManager = AudioManager::Get ();
	audioManager.WritePCM (buffer, size);
}

string AndroidContentManager::ReadFile (const string& fileName) const {
	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNIEnv* env = JNI::GetEnv ();
	JavaString&& jstr = JNI::CallObjectMethod<JavaString> (jni.mActivity, jni.mReadFileMethod, JavaString (fileName).get ());
	CHECKARG (!env->ExceptionCheck (), "Cannot read file, Java exception occured!");
	return jstr.getString ();
}

void AndroidContentManager::WriteFile (const string& fileName, const string& content) {
	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNIEnv* env = JNI::GetEnv ();
	env->CallVoidMethod (jni.mActivity, jni.mWriteFileMethod, JavaString (fileName).get (), JavaString (content).get ());
	CHECKARG (!env->ExceptionCheck (), "Cannot write file, Java exception occured!");
}

void AndroidContentManager::DisplayStatus (const string& status) const {
	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNIEnv* env = JNI::GetEnv ();
	env->CallVoidMethod (jni.mActivity, jni.mDisplayStatusMethod, JavaString (status).get ());
}

void AndroidContentManager::Log (const string& log) {
	LOGD ("%s", log.c_str ());
}

double AndroidContentManager::GetTime () const {
	timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (double) now.tv_sec + (double) now.tv_nsec / 1e9;
}

jobject AndroidContentManager::openAsset (const string & asset) const {
	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNIEnv* env = JNI::GetEnv ();

	JNI::AutoLocalRef <jobject> assets (env->CallObjectMethod (jni.mActivity, jni.mGetAssetsMethod));
	CHECKMSG (assets.get () != nullptr, "AndroidContentManager::openAsset () - assets cannot be nullptr!");

	jobject istream = env->CallObjectMethod (assets, jni.mOpenMethod, JavaString (asset).get ());
	if (istream == nullptr) {
		stringstream ss;
		ss << "Unable to open asset: " << asset;
		JNI::ThrowException ("java/lang/Exception", ss.str ());
	}

	return istream;
}

void AndroidContentManager::closeStream (jobject istream) const {
	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNIEnv* env = JNI::GetEnv ();
	env->CallVoidMethod (istream, jni.mCloseMethod);
}

jobject AndroidContentManager::loadBitmap (const string & asset) const {
	JNI_ContentManager& jni = JNI_ContentManager::Get ();
	JNIEnv* env = JNI::GetEnv ();

	JNI::AutoLocalRef <jobject> istream (openAsset (asset));
	CHECKMSG (istream.get () != nullptr, "AndroidContentManager::LoadBitmap () - istream cannot be nullptr!");

	jobject bitmap = env->CallStaticObjectMethod (jni.mClassBitmapFactory, jni.mDecodeStreamMethod, istream.get ());
	CHECKMSG (bitmap != nullptr, "AndroidContentManager::LoadBitmap () - bitmap cannot be nullptr!");

	closeStream (istream);

	return bitmap;
}
