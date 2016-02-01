#############################
# c64emu (prebuilt module)
#############################

LOCAL_PATH := $(abspath $(call my-dir))

include $(CLEAR_VARS)
LOCAL_MODULE := c64emu-prebuilt
LOCAL_SRC_FILES := ../../../build/intermediates/c64/$(TARGET_ARCH_ABI)/libc64emu.so

include $(PREBUILT_SHARED_LIBRARY)

#############################
# libgame.so (main module)
#############################

#libgame.so
include $(CLEAR_VARS)

LOCAL_MODULE    := game

#LOCAL_C_INCLUDES :=

#LOCAL_DISABLE_FORMAT_STRING_CHECKS := true
ifeq ($(TARGET_ARCH),x86) #gcc 4.8 need this because of a configuration bug in NDK (https://code.google.com/p/android/issues/detail?id=73843)
	LOCAL_CFLAGS += -DUSE_SSE4=0 -mtune=atom -m32 -mno-sse4.1 -mno-sse4.2 -mno-popcnt -mno-movbe
endif
LOCAL_CPP_EXTENSION := .cpp

LOCAL_SRC_FILES +=						\
	jnihelper/jniload.cpp				\
	platform/androidcontentmanager.cpp	\
	platform/androidutil.cpp			\
	platform/audiomanager.cpp			\
	management/game.cpp					\
	content/animation.cpp				\
	content/geom.cpp					\
	content/mesh2D.cpp					\
	content/imagemesh.cpp				\
	content/qte.cpp						\
	content/rigidbody2D.cpp				\
	game/mayhemgame.cpp					\
	game/gamescene.cpp					\
	jni_GameActivity.cpp				\
	jni_GameLib.cpp

LOCAL_SHARED_LIBRARIES := c64emu-prebuilt
#LOCAL_STATIC_LIBRARIES :=

LOCAL_LDLIBS := -llog -lGLESv1_CM -lEGL -landroid -ljnigraphics -lOpenSLES

include $(BUILD_SHARED_LIBRARY)

