#ifndef JAVABYTEARRAY_H_INCLUDED
#define JAVABYTEARRAY_H_INCLUDED

#include <vector>
using namespace std;

#include "JavaObject.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// JNI helper for JavaByteArray class
////////////////////////////////////////////////////////////////////////////////////////////////////
class JNI_JavaByteArray
{
	friend class JavaByteArray;

private:
	JNI_JavaByteArray () :
		mClass (nullptr)
	{
	}

	~JNI_JavaByteArray () {
		Release ();
	}

	static JNI_JavaByteArray& Get (bool checkInit = true) {
		static JNI_JavaByteArray inst;
		if (checkInit && !inst.IsInited ())
			inst.Init ();
		return inst;
	}

	static void ReinitJNI () {
		JNI_JavaByteArray& jni = JNI_JavaByteArray::Get (); //Get reference for jni interface
		jni.Release (); //Release all of the stored interface data
		jni.Init (); //Reinitialize jni interface 
	}

	bool IsInited () const {
		return mClass != nullptr;
	}

	void Init () {
		if (IsInited ())
			return;

        //Find and reference Java class byte[]
		JNI::AutoLocalRef<jclass> clazz (JNI::FindClass ("[B"));
		mClass = JNI::GlobalReferenceObject (clazz.get (), "Java class of byte[]");

        //Collect needed methods (byte[])
        //...
	}

	void Release () {
		if (!IsInited ())
			return;

		JNI::ReleaseGlobalReferencedObject (mClass, "JNI_JavaByteArray::Release ()");
		mClass = nullptr;
	}

private:
	jclass mClass; ///< The java class of byte[]
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// C++ implementation of the JavaByteArray class
////////////////////////////////////////////////////////////////////////////////////////////////////
class JavaByteArray : public JavaObject
{
public:
	JavaByteArray () :
		JavaObject ()
	{
		InitWithBytes (nullptr, 0);
	}

	JavaByteArray (const vector<uint8_t>& bytes) :
		JavaObject ()
	{
		InitWithBytes ((const char*) &bytes[0], (int) bytes.size ());
	}

	JavaByteArray (const string& str) :
		JavaObject ()
	{
		InitWithBytes (&str[0], (int) str.length ());
	}

	JavaByteArray (const char* bytes, int length) :
		JavaObject ()
	{
		InitWithBytes (bytes, length);
	}

	JavaByteArray (const JavaByteArray& src) : JavaObject (src) {}
	JavaByteArray (const JavaObject& src) : JavaObject (src) {}
	JavaByteArray (JavaByteArray&& src) : JavaObject (move (src)) {}
	JavaByteArray (JavaObject&& src) : JavaObject (move (src)) {}

	JavaByteArray (jobject object) :
		JavaObject (object)
	{
		if (mObject == nullptr)
			InitWithBytes (nullptr, 0);
		
		CHECKMSG (mObject != nullptr, "JavaByteArray::JavaByteArray (jobject object) - object is nullptr!");
	}

	const JavaByteArray& operator = (const JavaByteArray& src) {
		JavaObject::operator = (src);
		return *this;
	}

	const JavaByteArray& operator = (JavaByteArray&& src) {
		JavaObject::operator = (move (src));
		return *this;
	}

	vector<uint8_t> getBytes () const {
		int len = length ();
		if (len <= 0)
			return vector<uint8_t> ();

		JNI_JavaByteArray& jni = JNI_JavaByteArray::Get ();
		JNIEnv* env = JNI::GetEnv ();

		jbyte* bytes = nullptr;
    	if (env->IsInstanceOf (mObject, jni.mClass)) //byte[]
    		bytes = env->GetByteArrayElements ((jbyteArray) mObject, nullptr);
    	else //NIO buffer
    		bytes = reinterpret_cast<jbyte*> (env->GetDirectBufferAddress (mObject));

    	CHECKMSG (bytes != nullptr, "JavaByteArray ()::getBytes () - bytes cannot be nullptr!");

    	vector<uint8_t> res (bytes, bytes + len);
    	env->ReleaseByteArrayElements ((jbyteArray) mObject, bytes, JNI_ABORT);

    	return move (res);
    }

	string getString () const {
		int len = length ();
		if (len <= 0)
			return string ();

		JNI_JavaByteArray& jni = JNI_JavaByteArray::Get ();
		JNIEnv* env = JNI::GetEnv ();

		jbyte* bytes = nullptr;
    	if (env->IsInstanceOf (mObject, jni.mClass)) //byte[]
    		bytes = env->GetByteArrayElements ((jbyteArray) mObject, nullptr);
    	else //NIO buffer
    		bytes = reinterpret_cast<jbyte*> (env->GetDirectBufferAddress (mObject));

    	CHECKMSG (bytes != nullptr, "JavaByteArray ()::getBytes () - bytes cannot be nullptr!");

    	string res ((const char*) bytes, (size_t)len);
    	env->ReleaseByteArrayElements ((jbyteArray) mObject, bytes, JNI_ABORT);

    	return move (res);
    }

    int length() const {
    	if (mObject == nullptr)
    		return 0;
    	
    	JNI_JavaByteArray& jni = JNI_JavaByteArray::Get ();
    	JNIEnv* env = JNI::GetEnv ();

    	if (env->IsInstanceOf (mObject, jni.mClass)) //byte[]
    		return env->GetArrayLength ((jbyteArray) mObject);

	    //NIO buffer
    	return env->GetDirectBufferCapacity (mObject);
    }

private:
	void InitWithBytes (const char* bytes, int length) {
		CHECKMSG (length >= 0, "JavaByteArray::JavaByteArray (const char* bytes, int length) - Cannot create byte[] with negative length!");
		CHECKMSG (length == 0 || (length > 0 && bytes != nullptr), "JavaByteArray::JavaByteArray (const char* bytes, int length) - bytes is nullptr!");

		JNIEnv* env = JNI::GetEnv ();

		JNI::AutoLocalRef<jobject> jobj = env->NewByteArray (length);
		CHECKMSG (!env->ExceptionCheck (), "JavaByteArray::JavaByteArray (const char* bytes, int length) - Cannot allocat byte[]! Java exception occured!");

		mObject = JNI::GlobalReferenceObject (jobj.get ());

		if (bytes != nullptr && length > 0) {
			env->SetByteArrayRegion ((jbyteArray) mObject, 0, length, (const jbyte*) bytes);
			CHECKMSG (!env->ExceptionCheck (), "JavaByteArray::JavaByteArray (const char* bytes, int length) - Cannot set content! Java exception occured!");
		}
	}
};

#endif
