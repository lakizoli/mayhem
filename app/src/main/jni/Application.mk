APP_PLATFORM := android-10
NDK_TOOLCHAIN_VERSION := 4.9
APP_MODULES := game
APP_STL := c++_shared
APP_CFLAGS := -Wno-error=format-security -Wno-multichar $(GAME_CFLAGS)
APP_CPPFLAGS := -Wno-error=format-security -Wno-format-security --rtti -fexceptions $(GAME_CPPFLAGS)
