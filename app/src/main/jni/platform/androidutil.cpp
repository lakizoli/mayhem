#include "../pch.h"
#include "androidutil.h"
#include "../jnihelper/jniload.h"

void AndroidUtil::Log (const string& log) {
	LOGD ("%s", log.c_str ());
}
