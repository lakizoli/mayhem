
#ifndef _JNIONLOAD_H_
#define _JNIONLOAD_H_

using namespace std;

#include <jni.h>
#include <android/log.h>

//////////////////////////////////////////////////////////////////////////////////////////
//Macro definitions
//////////////////////////////////////////////////////////////////////////////////////////
#define LOG_TAG "BallFall"
#define LOGI(...)  __android_log_print (ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...)  __android_log_print (ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print (ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define CHECKMSG(check, msg)								\
	if (!(check)) {											\
		LOGE ("Assertion occured! message: %s", msg);		\
		assert (check);										\
	}

#define CHECK(check)	CHECKMSG (check, "nothing")

#define CHECKARG(check, ...)								\
	if (!(check)) {											\
		LOGE ("Assertion occured!");						\
		LOGE (__VA_ARGS__);									\
		assert (check);										\
	}

//////////////////////////////////////////////////////////////////////////////////////////
//Forward declarations
//////////////////////////////////////////////////////////////////////////////////////////
// class JavaObject;

//////////////////////////////////////////////////////////////////////////////////////////
//The JNI interface functions
//////////////////////////////////////////////////////////////////////////////////////////
namespace JNI {

//////////////////////////////////////////////////////////////////////////////////////////
//Basic functions
//////////////////////////////////////////////////////////////////////////////////////////
void Init (JavaVM*);
bool IsInited ();

JavaVM* GetJavaVM ();
JNIEnv* GetEnv ();

//////////////////////////////////////////////////////////////////////////////////////////
//Class functions
//////////////////////////////////////////////////////////////////////////////////////////
jclass FindClass (const char* signature);
inline jclass FindClass (const string& signature) {
	return JNI::FindClass (signature.c_str ());
}

jmethodID GetMethod (jclass clazz, const char* method, const char* signature);
inline jmethodID GetMethod (jclass clazz, const string& method, const string& signature) {
	return GetMethod (clazz, method.c_str (), signature.c_str ());
}

jmethodID GetStaticMethod (jclass clazz, const char* method, const char* signature);
inline jmethodID GetStaticMethod (jclass clazz, const string& method, const string& signature) {
	return GetStaticMethod (clazz, method.c_str (), signature.c_str ());
}

jfieldID GetField (jclass clazz, const char* field, const char* signature);
inline jfieldID GetField (jclass clazz, const string& field, const string& signature) {
	return GetField (clazz, field.c_str (), signature.c_str ());
}

jfieldID GetStaticField (jclass clazz, const char* field, const char* signature);
inline jfieldID GetStaticField (jclass clazz, const string& field, const string& signature) {
	return GetStaticField (clazz, field.c_str (), signature.c_str ());
}

//////////////////////////////////////////////////////////////////////////////////////////
//Reference functions
//
//Internal code!
//Use JavaObject [jnihelper/JavaObject.h] or something similar instead of this, where you can!
//Never use direct call to NewLocalRef () or NewGlobalRef!
//
//////////////////////////////////////////////////////////////////////////////////////////

//Frame functions
void PushLocalFrame (int neededRefCount);
void PopLocalFrame ();

struct AutoLocalFrame {
	AutoLocalFrame (int neededRefCount) { PushLocalFrame (neededRefCount); }
	~AutoLocalFrame () { PopLocalFrame (); }
};

//Global reference functions
jobject GlobalReferenceObject (jobject obj, const char* errorMessage = nullptr);

template<class T>
T GlobalReferenceObject (T obj, const char* errorMessage = nullptr) {
	return reinterpret_cast<T> (GlobalReferenceObject (reinterpret_cast<jobject> (obj), errorMessage));
}

bool IsGlobalReference (jobject obj);

template<class T>
bool IsGlobalReference (T obj) {
	return IsGlobalReference (reinterpret_cast<jobject> (obj));
}

void ReleaseGlobalReferencedObject (jobject obj, const char* errorMessage = nullptr);

template<class T>
void ReleaseGlobalReferencedObject (T obj, const char* errorMessage = nullptr) {
	ReleaseGlobalReferencedObject (reinterpret_cast<jobject> (obj), errorMessage);
}

template<class T>
class AutoGlobalRef {
	T obj;
	const char* msg;
public:
	AutoGlobalRef (T obj, const char* msg = "nothing") : obj (obj), msg (msg) {}
	AutoGlobalRef (const AutoGlobalRef& src); //deleted function
	AutoGlobalRef (AutoGlobalRef&& src) : obj (nullptr), msg (nullptr) { *this = move (src); }

	~AutoGlobalRef () { ReleaseGlobalReferencedObject (obj, msg); }

	const AutoGlobalRef& operator = (const AutoGlobalRef& src); //deleted function
	const AutoGlobalRef& operator = (AutoGlobalRef&& src) {
		ReleaseGlobalReferencedObject (obj, msg);

		obj = src.obj;
		src.obj = nullptr;

		msg = src.msg;
		src.msg = nullptr;

		return *this;
	}

	T get () const { return obj; }
	operator T () const { return obj; }
};

//Local reference functions
jobject LocalReferenceObject (jobject obj, const char* errorMessage = nullptr);

template<class T>
T LocalReferenceObject (T obj, const char* errorMessage = nullptr) {
	return reinterpret_cast<T> (LocalReferenceObject (reinterpret_cast<jobject> (obj), errorMessage));
}

bool IsLocalReference (jobject obj);

template<class T>
bool IsLocalReference (T obj) {
	return IsLocalReference (reinterpret_cast<jobject> (obj));
}

void ReleaseLocalReferencedObject (jobject obj, const char* errorMessage = nullptr);

template<class T>
void ReleaseLocalReferencedObject (T obj, const char* errorMessage = nullptr) {
	ReleaseLocalReferencedObject (reinterpret_cast<jobject> (obj), errorMessage);
}

template<class T>
class AutoLocalRef {
	T obj;
	const char* msg;
public:
	AutoLocalRef (const char* msg = "nothing") : obj (nullptr), msg (msg) {}
	AutoLocalRef (T obj, const char* msg = "nothing") : obj (obj), msg (msg) {}
	AutoLocalRef (const AutoLocalRef& src); //deleted function
	AutoLocalRef (AutoLocalRef&& src) : obj (nullptr), msg (nullptr) { *this = move (src); }

	~AutoLocalRef () { ReleaseLocalReferencedObject (obj, msg); }

	const AutoLocalRef& operator = (const AutoLocalRef& src); //deleted function
	const AutoLocalRef& operator = (AutoLocalRef&& src) {
		ReleaseLocalReferencedObject (obj, msg);

		obj = src.obj;
		src.obj = nullptr;

		msg = src.msg;
		src.msg = nullptr;

		return *this;
	}

	T get () const { return obj; }
	operator T () const { return obj; }

	void reset (T resetObj = nullptr) { ReleaseLocalReferencedObject (obj, msg); obj = resetObj; }
};

//Debug functions
void DumpReferenceTables ();

//////////////////////////////////////////////////////////////////////////////////////////
//Object call functions
//////////////////////////////////////////////////////////////////////////////////////////
template<class T>
T NewObject (jclass clazz, jmethodID initMethod, ...) {
	va_list params;
	va_start (params, initMethod);

	jobject jres = GetEnv ()->NewObjectV (clazz, initMethod, params);
	T res (jres);
	ReleaseLocalReferencedObject (jres);

	va_end (params);
	
	return move (res);
}

template<class T>
T CallObjectMethod (jobject obj, jmethodID method, ...) {
	va_list params;
	va_start (params, method);

	jobject jres = GetEnv ()->CallObjectMethodV (obj, method, params);
	T res (jres);
	ReleaseLocalReferencedObject (jres);

	va_end (params);
	
	return move (res);
}

template<class T>
T CallStaticObjectMethod (jclass clazz, jmethodID method, ...) {
	va_list params;
	va_start (params, method);

	jobject jres = GetEnv ()->CallStaticObjectMethodV (clazz, method, params);
	T res (jres);
	ReleaseLocalReferencedObject (jres);

	va_end (params);
	
	return move (res);
}

//////////////////////////////////////////////////////////////////////////////////////////
//Object field functions
//////////////////////////////////////////////////////////////////////////////////////////
template<class T>
T GetObjectField (jobject obj, jfieldID field) {
	jobject jres = GetEnv ()->GetObjectField (obj, field);
	T res (jres);
	ReleaseLocalReferencedObject (jres);
	
	return move (res);
}

template<class T>
T GetStaticObjectField (jclass clazz, jfieldID field) {
	jobject jres = GetEnv ()->GetStaticObjectField (clazz, field);
	T res (jres);
	ReleaseLocalReferencedObject (jres);
	
	return move (res);
}

//////////////////////////////////////////////////////////////////////////////////////////
//Exception functions
//////////////////////////////////////////////////////////////////////////////////////////
void ClearExceptions ();
void ThrowException (const char* clazz, const char* msg = "no message");
inline void ThrowException (const string& clazz, const string& msg = string ("no message")) {
	ThrowException (clazz.c_str (), msg.c_str ());
}

inline void ThrowNullPointerException (const char* msg = "no message") {
	ThrowException ("java/lang/NullPointerException", msg);
}

inline void ThrowNullPointerException (const string& msg = string ("no message")) {
	ThrowException ("java/lang/NullPointerException", msg.c_str ());
}

inline void ThrowRuntimeException (const char* msg = "no message") {
	ThrowException ("java/lang/RuntimeException", msg);
}

inline void ThrowRuntimeException (const string& msg = string ("no message")) {
	ThrowException ("java/lang/RuntimeException", msg.c_str ());
}

string StrError (int errnum);

inline void ThrowIOException (int errnum) {
	ThrowException ("java/io/IOException", StrError (errnum).c_str ());
}

} //namespace JNI

#endif //_JNIONLOAD_H_
