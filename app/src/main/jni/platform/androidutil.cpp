#include "../pch.h"
#include "androidutil.h"
#include "../jnihelper/jniload.h"

void AndroidUtil::Log (const string& log) {
	LOGD ("%s", log.c_str ());
}

double AndroidUtil::GetTime () const {
	timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (double) now.tv_sec + (double) now.tv_nsec / 1e9;
}