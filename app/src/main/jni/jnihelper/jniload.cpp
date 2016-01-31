#include "../pch.h"
#include "jniload.h"

namespace JNI {

class JNI
{
	friend void Init (JavaVM* vm);
	friend JavaVM* GetJavaVM ();
	friend JNIEnv* GetEnv ();
	friend jclass FindClass (const char* signature);
	//friend jint JNICALL::JNI_OnLoad (JavaVM* vm, void* reserved);
	//friend void ::JNI_OnUnload (JavaVM* vm, void* reserved);

private:
	JavaVM* javaVM;
	jclass classLoaderClass;
	jobject classLoaderInstance;
	jmethodID findClassMethod;

public:
	JNI () {
		javaVM = nullptr;
		classLoaderClass = nullptr;
		classLoaderInstance = nullptr;
		findClassMethod = nullptr;
	}

	~JNI () {
		javaVM = nullptr;

		ReleaseGlobalReferencedObject (classLoaderClass);
		classLoaderClass = nullptr;

		ReleaseGlobalReferencedObject (classLoaderInstance);
		classLoaderInstance = nullptr;

		findClassMethod = nullptr;
	}

	static JNI& Get () {
		static JNI jni;
		return jni;
	}

	JNIEnv* GetEnv () {
		CHECKMSG (javaVM != nullptr, "Java virtual machine reference cannot be nullptr!");

		JNIEnv* env = nullptr;
		int status = javaVM->GetEnv ((void**) &env, JNI_VERSION_1_6);
		switch (status) {
			case JNI_EDETACHED: {
				//Attach current thread to environment
		    	int statusAttach = javaVM->AttachCurrentThread (&env, nullptr);
		        CHECKARG (statusAttach == 0, "Cannot attach current thread to environment! status: %d", statusAttach);
		        break;
	    	}
		    case JNI_OK:
			    break;
		    default:
		    case JNI_EVERSION: {
		        CHECKMSG (nullptr, "Version not supported!");
		        break;
		    }
		}

		CHECKMSG (env != nullptr, "env cannot be nullptr!");
		return env;
	}

	void Init (JavaVM* vm) {
		javaVM = vm;

		JNIEnv* env = GetEnv ();

		AutoLocalRef<jclass> gameClass (env->FindClass ("com/mayhem/GameActivity"), "JNI::Init () - gameClass");
		CHECKMSG (gameClass != nullptr, "cannot find java class: 'com/mayhem/GameActivity'");

		AutoLocalRef<jclass> classClass (env->FindClass ("java/lang/Class"), "JNI::Init () - classClass");
		CHECKMSG (classClass != nullptr, "cannot find java class: 'java/lang/Class'");

		AutoLocalRef<jclass> classLoaderClassLocalRef (env->FindClass ("java/lang/ClassLoader"), "JNI::Init () - classLoaderClassLocalRef");
		CHECKMSG (classLoaderClassLocalRef != nullptr, "cannot find java class: 'java/lang/ClassLoader'");

		classLoaderClass = GlobalReferenceObject (classLoaderClassLocalRef.get (), "cannot reference java class: 'java/lang/ClassLoader'");

		jmethodID getClassLoaderMethod = env->GetMethodID (classClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
		CHECKMSG (getClassLoaderMethod != nullptr, "cannot find java method: 'ClassLoader Class.getClassLoader ()'");

		AutoLocalRef<jobject> classLoaderInstanceLocalRef (env->CallObjectMethod (gameClass, getClassLoaderMethod), "JNI::Init () - classLoaderInstanceLocalRef");
		CHECKMSG (classLoaderInstanceLocalRef != nullptr, "cannot find java instance of class: 'java/lang/ClassLoader'");

		classLoaderInstance = GlobalReferenceObject (classLoaderInstanceLocalRef.get (), "cannot reference java instance of class: 'java/lang/ClassLoader'");

		findClassMethod = env->GetMethodID (classLoaderClass, "findClass", "(Ljava/lang/String;)Ljava/lang/Class;");
		CHECKMSG (findClassMethod != nullptr, "cannot find java method: 'Class ClassLoader.findClass (String name)'");
	}
};

void Init (JavaVM* vm)
{
	JNI& jni = JNI::Get ();
	jni.Init (vm);
}

bool IsInited ()
{
	return GetJavaVM () != nullptr;
}

JavaVM* GetJavaVM ()
{
	JNI& jni = JNI::Get ();
	return jni.javaVM;
}

JNIEnv* GetEnv ()
{
	JNI& jni = JNI::Get ();
	return jni.GetEnv ();
}

jclass FindClass (const char* signature)
{
	JNI& jni = JNI::Get ();
	JNIEnv* env = jni.GetEnv ();

	jclass result = env->FindClass (signature);
	if (env->ExceptionCheck () || result == nullptr) { //If environment not knows this class, then try to find in Java
		//Ignore Java exception, if occured...
		env->ExceptionClear ();

		//Find class with java on ClassLoader
		jstring str = env->NewStringUTF (signature);
		result = reinterpret_cast<jclass> (env->CallObjectMethod (jni.classLoaderInstance, jni.findClassMethod, str));
		env->DeleteLocalRef (str);
	}

	CHECKARG (!env->ExceptionCheck (), "Couldn't find class: %s, Java exception occured!", signature);
	CHECKARG (result != nullptr, "Couldn't find class: %s", signature);

	return result;
}

jmethodID GetMethod (jclass clazz, const char* method, const char* signature) {
	JNIEnv* env = GetEnv ();
	jmethodID result = env->GetMethodID (clazz, method, signature);
	CHECKARG (!env->ExceptionCheck (), "Couldn't find method: %s, signature: %s, Java exception occured!", method, signature);
	CHECKARG (result != nullptr, "Couldn't find method: %s, signature: %s", method, signature);
	return result;
}

jmethodID GetStaticMethod (jclass clazz, const char* method, const char* signature) {
	JNIEnv* env = GetEnv ();
	jmethodID result = env->GetStaticMethodID (clazz, method, signature);
	CHECKARG (!env->ExceptionCheck (), "Couldn't find static method: %s, signature: %s, Java exception occured!", method, signature);
	CHECKARG (result != nullptr, "Couldn't find static method: %s, signature: %s", method, signature);
	return result;
}

jfieldID GetField (jclass clazz, const char* field, const char* signature) {
	JNIEnv* env = GetEnv ();
	jfieldID result = env->GetFieldID (clazz, field, signature);
	CHECKARG (!env->ExceptionCheck (), "Couldn't find field: %s, signature: %s, Java exception occured!", field, signature);
	CHECKARG (result != nullptr, "Couldn't find field: %s, signature: %s", field, signature);
	return result;
}

jfieldID GetStaticField (jclass clazz, const char* field, const char* signature) {
	JNIEnv* env = GetEnv ();
	jfieldID result = env->GetStaticFieldID (clazz, field, signature);
	CHECKARG (!env->ExceptionCheck (), "Couldn't find static field: %s, signature: %s, Java exception occured!", field, signature);
	CHECKARG (result != nullptr, "Couldn't find static field: %s, signature: %s", field, signature);
	return result;
}

void PushLocalFrame (int neededRefCount)
{
	CHECKMSG (neededRefCount > 0, "The needed local frame ref count must be greater than 0!");

	JNIEnv* env = GetEnv ();
	jint res = env->PushLocalFrame (neededRefCount);

	CHECKARG (!env->ExceptionCheck (), "Couldn't create local frame for references! neededRefCount: %d", neededRefCount);
	CHECKARG (res == 0, "Couldn't create local frame for references! neededRefCount: %d", neededRefCount);
}

void PopLocalFrame ()
{
	JNIEnv* env = GetEnv ();
	env->PopLocalFrame (nullptr);
	CHECKARG (!env->ExceptionCheck (), "Couldn't release local frame for references!");
}

jobject GlobalReferenceObject (jobject obj, const char* errorMessage)
{
	CHECKMSG (obj != nullptr, "Couldn't global reference object with null pointer!");

	//Create global reference
	JNIEnv* env = GetEnv ();
	jobject result = reinterpret_cast<jobject> (env->NewGlobalRef (obj));
	if (errorMessage == nullptr) {
		CHECKMSG (!env->ExceptionCheck (), "Couldn't global reference object! Java exception occured!");
		CHECKMSG (result != nullptr, "Couldn't global reference object!");
	} else {
		CHECKARG (!env->ExceptionCheck (), "Couldn't global reference object! Java exception occured! msg: %s", errorMessage);
		CHECKARG (result != nullptr, "Couldn't global reference object! msg: %s", errorMessage);
	}

	// LOGI ("GlobalReferenceObject () - src obj: 0x%08x, globalRef result: 0x%08x, refType: %d, msg: %s", (int)obj, (int)result, obj == nullptr ? -1 : (int) GetEnv ()->GetObjectRefType (obj), errorMessage);

	return result;
}

bool IsGlobalReference (jobject obj)
{
	if (obj != nullptr)
		return GetEnv ()->GetObjectRefType (obj) == JNIGlobalRefType;
	return false;
}

void ReleaseGlobalReferencedObject (jobject obj, const char* errorMessage)
{
	// LOGI ("ReleaseGlobalReferencedObject () - obj: 0x%08x, refType: %d, msg: %s", (int)obj, obj == nullptr ? -1 : (int) GetEnv ()->GetObjectRefType (obj), errorMessage);

	if (obj != nullptr) {
		JNIEnv* env = GetEnv ();

		jobjectRefType refType = env->GetObjectRefType (obj);
		if (refType == JNIGlobalRefType)
			env->DeleteGlobalRef (obj);
		else {
			if (errorMessage == nullptr) {
				CHECKARG (!env->ExceptionCheck (), "Couldn't release referenced object! Reference error! Unknown or unhandled reference type... refType: %d", (int)refType);
			} else {
				CHECKARG (!env->ExceptionCheck (), "Couldn't release referenced object! Reference error! Unknown or unhandled reference type... refType: %d, msg: %s", (int)refType, errorMessage);
			}
		}

		if (errorMessage == nullptr) {
			CHECKMSG (!env->ExceptionCheck (), "Couldn't release referenced object! Java exception occured!");
		} else {
			CHECKARG (!env->ExceptionCheck (), "Couldn't release referenced object! Java exception occured! msg: %s", errorMessage);
		}
	}
}

jobject LocalReferenceObject (jobject obj, const char* errorMessage)
{
	CHECKMSG (obj != nullptr, "Couldn't local reference object with null pointer!");

	//Create local reference
	JNIEnv* env = GetEnv ();
	jobject result = reinterpret_cast<jobject> (env->NewLocalRef (obj));
	if (errorMessage == nullptr) {
		CHECKMSG (!env->ExceptionCheck (), "Couldn't local reference object! Java exception occured!");
		CHECKMSG (result != nullptr, "Couldn't local reference object!");
	} else {
		CHECKARG (!env->ExceptionCheck (), "Couldn't local reference object! Java exception occured! msg: %s", errorMessage);
		CHECKARG (result != nullptr, "Couldn't local reference object! msg: %s", errorMessage);
	}

	// LOGI ("LocalReferenceObject () - src obj: 0x%08x, localRef result: 0x%08x, refType: %d, msg: %s", (int)obj, (int)result, obj == nullptr ? -1 : (int) GetEnv ()->GetObjectRefType (obj), errorMessage);

	return result;
}

bool IsLocalReference (jobject obj)
{
	if (obj != nullptr)
		return GetEnv ()->GetObjectRefType (obj) == JNILocalRefType;
	return false;
}

void ReleaseLocalReferencedObject (jobject obj, const char* errorMessage)
{
	// LOGI ("ReleaseLocalReferencedObject () - obj: 0x%08x, refType: %d, msg: %s", (int)obj, obj == nullptr ? -1 : (int) GetEnv ()->GetObjectRefType (obj), errorMessage);

	if (obj != nullptr) {
		JNIEnv* env = GetEnv ();

		jobjectRefType refType = env->GetObjectRefType (obj);
		if (refType == JNILocalRefType)
			env->DeleteLocalRef (obj);
		else {
			if (errorMessage == nullptr) {
				CHECKARG (!env->ExceptionCheck (), "Couldn't release local referenced object! Reference error! Unknown or unhandled reference type... refType: %d", (int)refType);
			} else {
				CHECKARG (!env->ExceptionCheck (), "Couldn't release local referenced object! Reference error! Unknown or unhandled reference type... refType: %d, msg: %s", (int)refType, errorMessage);
			}
		}

		if (errorMessage == nullptr) {
			CHECKMSG (!env->ExceptionCheck (), "Couldn't release referenced object! Java exception occured!");
		} else {
			CHECKARG (!env->ExceptionCheck (), "Couldn't release referenced object! Java exception occured! msg: %s", errorMessage);
		}
	}
}

void DumpReferenceTables ()
{
	JNIEnv* env = GetEnv ();
	AutoLocalRef<jclass> vm_class (FindClass ("dalvik/system/VMDebug"), "Java class of dalvik/system/VMDebug");
	jmethodID method = env->GetStaticMethodID (vm_class, "dumpReferenceTables", "()V");
	env->CallStaticVoidMethod (vm_class, method);
}

void ClearExceptions () {
	JNIEnv* env = GetEnv ();
	env->ExceptionClear ();
}

void ThrowException (const char* clazz, const char* msg) {
	jclass exceptionClass = FindClass (clazz);
	CHECKARG (exceptionClass != nullptr, "Unable to find exception class %s", clazz);

	JNIEnv* env = GetEnv ();
	int status = env->ThrowNew (exceptionClass, msg);
	CHECKARG (status == JNI_OK, "Failed throwing '%s' '%s'", clazz, msg);
}

string StrError (int errnum)
{
	char buf[1024];

	// Note: glibc has a nonstandard strerror_r that returns char* rather than POSIX's int.
	// char *strerror_r(int errnum, char *buf, size_t n);
	char* ret = (char*) strerror_r (errnum, buf, sizeof (buf));
	if (((int)ret) == 0) {
		// POSIX strerror_r, success
		return string (buf);
	} else if (((int)ret) == -1) {
		// POSIX strerror_r, failure
		// (Strictly, POSIX only guarantees a value other than 0. The safest
		// way to implement this function is to use C++ and overload on the
		// type of strerror_r to accurately distinguish GNU from POSIX. But
		// realistic implementations will always return -1.)
		snprintf (buf, sizeof (buf), "errno %d", errnum);
		return string (buf);
	}

	// glibc strerror_r returning a string
	return string (ret);
}

} //namespace JNI

/**
* @brief The module load event handler (called once, when the dynamic library loaded)
*
* @param vm The reference for the Java virtual machine.
* @param reserved Reserved by the Java virtual machine.
*
* @return The version of the used JNI protocol.
*/
JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM* vm, void* reserved) {
	//Init JNI layer
	JNI::JNI& jni = JNI::JNI::Get ();
	jni.Init (vm);

	//Init BIMx specific stuffs
	//...

	return JNI_VERSION_1_6;
}

/**
* @brief The module unload event handler.
*
* @param vm The reference for the Java virtual machine.
* @param reserved Reserved by the Java virtual machine.
*/
JNIEXPORT void JNI_OnUnload (JavaVM* vm, void* reserved) {
	//Destroy BIMx specific stuffs
	//...
}
