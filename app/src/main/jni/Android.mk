#############################
# libgame.so (main module)
#############################

LOCAL_PATH := $(abspath $(call my-dir))

include $(CLEAR_VARS)

LOCAL_MODULE    := game

#LOCAL_C_INCLUDES :=		 					\
#	$(LOCAL_PATH)/../src/arch/win32/msvc 		\
#	$(LOCAL_PATH)/../src/arch/win32 			\
#	$(LOCAL_PATH)/../src						\
#	$(LOCAL_PATH)/../src/drive               	\
#	$(LOCAL_PATH)/../src/vdrive              	\
#	$(LOCAL_PATH)/../src/platform            	\
#	$(LOCAL_PATH)/../src/lib/p64             	\
#	$(LOCAL_PATH)/../src/monitor

#LOCAL_DISABLE_FORMAT_STRING_CHECKS := true
ifeq ($(TARGET_ARCH),x86) #gcc 4.8 need this because of a configuration bug in NDK (https://code.google.com/p/android/issues/detail?id=73843)
	LOCAL_CFLAGS += -DUSE_SSE4=0 -mtune=atom -m32 -mno-sse4.1 -mno-sse4.2 -mno-popcnt -mno-movbe
endif
LOCAL_CPP_EXTENSION := .cpp

LOCAL_SRC_FILES +=					\
	gl_code.cpp

#LOCAL_SHARED_LIBRARIES :=
#LOCAL_STATIC_LIBRARIES :=

LOCAL_LDLIBS := -llog -lGLESv1_CM -landroid

include $(BUILD_SHARED_LIBRARY)

