find build/intermediates/ndk/obj/local/* -iname "*.so" | cut -sd / -f 6- | xargs -I{} cp -Rv build/intermediates/ndk/obj/local/{} src/main/jniLibs/{}
