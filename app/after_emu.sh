mv ../../vice-2.4-android/src/arch/win32/msvc/__inttypes.h ../../vice-2.4-android/src/arch/win32/msvc/inttypes.h
mv ../../vice-2.4-android/src/arch/win32/msvc/__stdint.h ../../vice-2.4-android/src/arch/win32/msvc/stdint.h
mv ../../vice-2.4-android/src/arch/win32/msvc/__dirent.h ../../vice-2.4-android/src/arch/win32/msvc/dirent.h
mv ../../vice-2.4-android/src/arch/win32/msvc/__msvc_ver.h ../../vice-2.4-android/src/arch/win32/msvc/msvc_ver.h
rm -Rf build/intermediates/c64
mkdir build/intermediates/c64
find build/intermediates/ndk/obj/local/* -iname "libc64emu.so" | cut -sd / -f 6-6 | xargs -I{} mkdir build/intermediates/c64/{}
find build/intermediates/ndk/obj/local/* -iname "libc64emu.so" | cut -sd / -f 6- | xargs -I{} cp -Rv build/intermediates/ndk/obj/local/{} build/intermediates/c64/{}