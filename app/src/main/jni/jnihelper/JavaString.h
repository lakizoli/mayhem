#ifndef _JAVA_STRING_H_INCLUDED
#define _JAVA_STRING_H_INCLUDED

#include "JavaObject.h"
#include "JavaByteArray.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// JNI helper for JavaString class
////////////////////////////////////////////////////////////////////////////////////////////////////
class JNI_JavaString
{
	friend class JavaString;

private:
	JNI_JavaString () :
		mClass (nullptr),
		mInitMethod (nullptr),
		mInitWithBytesMethod (nullptr),
		mInitWithEncodingMethod (nullptr),
		mGetBytesMethod (nullptr),
		mGetBytesWithEncodingMethod (nullptr)
	{
	}

	~JNI_JavaString () {
		Release ();
	}

	static JNI_JavaString& Get (bool checkInit = true) {
		static JNI_JavaString inst;
		if (checkInit && !inst.IsInited ())
			inst.Init ();
		return inst;
	}

	static void ReinitJNI () {
		JNI_JavaString& jni = JNI_JavaString::Get (); //Get reference for jni interface
		jni.Release (); //Release all of the stored interface data
		jni.Init (); //Reinitialize jni interface 
	}

	bool IsInited () const {
		return mClass != nullptr;
	}

	void Init () {
		if (IsInited ())
			return;

        //Find and reference Java class java.lang.String
		JNI::AutoLocalRef<jclass> clazz (JNI::FindClass ("java/lang/String"));
		mClass = JNI::GlobalReferenceObject (clazz.get (), "Java class of java/lang/String");

        //Collect needed methods (java.lang.String)
        mInitMethod = JNI::GetMethod (mClass, "<init>", "()V");
		mInitWithBytesMethod = JNI::GetMethod (mClass, "<init>", "([B)V");
		mInitWithEncodingMethod = JNI::GetMethod (mClass, "<init>", "([BLjava/lang/String;)V");
		mGetBytesMethod = JNI::GetMethod (mClass, "getBytes", "()[B");
		mGetBytesWithEncodingMethod = JNI::GetMethod (mClass, "getBytes", "(Ljava/lang/String;)[B");
	}

	void Release () {
		if (!IsInited ())
			return;

		JNI::ReleaseGlobalReferencedObject (mClass, "JNI_JavaString::Release ()");
		mClass = nullptr;

		mInitMethod = nullptr;
		mInitWithBytesMethod = nullptr;
		mInitWithEncodingMethod = nullptr;
		mGetBytesMethod = nullptr;
		mGetBytesWithEncodingMethod = nullptr;
	}

private:
	jclass mClass; ///< The java class of java.lang.String

	jmethodID mInitMethod;
	jmethodID mInitWithBytesMethod;
	jmethodID mInitWithEncodingMethod;
	jmethodID mGetBytesMethod;
	jmethodID mGetBytesWithEncodingMethod;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// C++ implementation of the JavaString class
////////////////////////////////////////////////////////////////////////////////////////////////////
class JavaString : public JavaObject
{
public:
	JavaString () :
		JavaObject ()
	{
		JNI_JavaString& jni = JNI_JavaString::Get ();
		JNI::AutoLocalRef<jobject> jobj (JNI::GetEnv ()->NewObject (jni.mClass, jni.mInitMethod));
		mObject = JNI::GlobalReferenceObject (jobj.get ());
	}

	JavaString (const string& str, const char* encoding = "UTF-8") :
		JavaObject ()
	{
		InitWithEncoding (str.c_str (), str.length (), encoding);
	}

	JavaString (const char* bytes, int length, const char* encoding = "UTF-8") :
		JavaObject ()
	{
		InitWithEncoding (bytes, length, encoding);
	}

	JavaString (const JavaString& src) : JavaObject (src) {}
	JavaString (const JavaObject& src) : JavaObject (src) {}
	JavaString (JavaString&& src) : JavaObject (move (src)) {}
	JavaString (JavaObject&& src) : JavaObject (move (src)) {}

	JavaString (jstring obj) :
		JavaObject (obj)
	{
		if (mObject == nullptr) //The given obj is nullptr
			InitWithEncoding (nullptr, 0, "UTF-8");
	}

	JavaString (jobject obj) :
		JavaObject (obj)
	{
		if (mObject == nullptr) //The given obj is nullptr
			InitWithEncoding (nullptr, 0, "UTF-8");
	}

	const JavaString& operator = (const JavaString& src) {
		JavaObject::operator = (src);
		return *this;
	}

	const JavaString& operator = (JavaString&& src) {
		JavaObject::operator = (move (src));
		return *this;
	}

	string getString () const { return getStringWithEncoding ("UTF-8"); }

	string getStringWithEncoding (const char* encoding = "UTF-8") const {
		if (mObject == nullptr)
			return string ();
		
		JNI_JavaString& jni = JNI_JavaString::Get ();
		JNIEnv* env = JNI::GetEnv ();

		if (encoding == nullptr) {
			jobject jobj = env->CallObjectMethod (mObject, jni.mGetBytesMethod);
			if (env->ExceptionCheck ()) { //UnsupportedEncodingException (wrong UTF-8 content)
				if (jobj != nullptr)
					JNI::ReleaseLocalReferencedObject (jobj, "getStringWithEncoding () - getBytes release"); 
				JNI::ClearExceptions ();
				return string ();
			}
			JNI::AutoLocalRef<jobject> jarr (jobj);
			JavaByteArray arr (jarr.get ());
			return arr.getString ();
		}

		//We have encoding
		jobject jobj = env->CallObjectMethod (mObject, jni.mGetBytesWithEncodingMethod, JavaString (encoding, strlen (encoding)).get ());
		if (env->ExceptionCheck ()) { //UnsupportedEncodingException (wrong UTF-8 content)
			if (jobj != nullptr)
				JNI::ReleaseLocalReferencedObject (jobj, "getStringWithEncoding () - getBytesWithEncoding release"); 
			JNI::ClearExceptions ();
			return string ();
		}
		JNI::AutoLocalRef<jobject> jarr (jobj);
		JavaByteArray arr (jarr.get ());
		return arr.getString ();
	}

	uint64_t uint64Value () const {
		string&& str = getString ();
		if (str.empty ())
			return 0;

		return strtoull (str.c_str (), nullptr, 10);
	}


private:
	void InitWithEncoding (const char* bytes, int length, const char* encoding) {
		JNIEnv* env = JNI::GetEnv ();
		JNI_JavaString& jni = JNI_JavaString::Get ();

		if (bytes == nullptr || (bytes != nullptr && length <= 0)) { //If the source string is and empty C string!
			JNI::AutoLocalRef<jobject> jobj (env->NewObject (jni.mClass, jni.mInitMethod));
			mObject = JNI::GlobalReferenceObject (jobj.get ());
		} else { //The source is a valid C string!
			if (encoding == nullptr) {
				JNI::AutoLocalRef<jobject> jobj (env->NewObject (jni.mClass, jni.mInitWithBytesMethod, JavaByteArray (bytes, length).get ()));
				mObject = JNI::GlobalReferenceObject (jobj.get ());
			} else {
				JNI::AutoLocalRef<jstring> jencoding (env->NewStringUTF (encoding));
				JNI::AutoLocalRef<jobject> jobj (env->NewObject (jni.mClass, jni.mInitWithEncodingMethod, JavaByteArray (bytes, length).get (), jencoding.get ()));
				mObject = JNI::GlobalReferenceObject (jobj.get ());
			}
		}
	}
};

#endif
