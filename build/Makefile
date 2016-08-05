result=debug
target=linux
arch=x64

cc_linux_x86=gcc -m32 -mtune=i686
cc_linux_x64=gcc -m64 -mtune=generic
cc_win_x86=i686-w64-mingw32-gcc
cc_win_x64=x86_64-w64-mingw32-gcc
cc=$(cc_$(target)_$(arch))

cpp_linux_x86=g++ -m32 -mtune=i686
cpp_linux_x64=g++ -m64 -mtune=generic
cpp_win_x86=i686-w64-mingw32-g++
cpp_win_x64=x86_64-w64-mingw32-g++
cpp=$(cpp_$(target)_$(arch))


c_lang=-std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror
cpp_lang=-std=c++11 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror

opt_debug=-O0
opt_release=-O3
opt=$(opt_$(result))

dbg_debug=-g3 -ggdb -pg
dbg_release=
dbg=$(dbg_$(result))

c_platform_linux=-rdynamic -lm -lpthread -ldl
c_platform_win=-static -Wno-pedantic -IWin32 Win32/*.c -Wl,-export-all-symbols -lm -lpsapi -lpthread
c_platform=$(c_platform_$(target))

misc=

cflags=$(c_lang) $(opt) $(dbg) -I../yoyo-util -I../yoyo-runtime -I../yoyo-runtime/headers -I../yoyo-parser/headers -I../yoyo-vm/headers -I../yoyo-jit/headers $(c_platform) $(misc)
c_source=../yoyo-util/*.c ../yoyo-vm/*.c ../YStd/native/*.c ../yoyo-parser/*.c ../yoyo-runtime/*.c ../yoyo-runtime/types/*.c

yoyo_linux=yoyo
yoyo_win=yoyo.exe
yoyo=$(yoyo_$(target))

yji_linux=libyji.so
yji_win=yji.dll
yji=$(yji_$(target))

libyoyo_linux=libyoyo.so
libyoyo_win=yoyo.dll
libyoyo=$(libyoyo_$(target))

jyoyo=jyoyo.jar


yoyo-jit:
	$(cpp) $(cpp_flags) -shared -fPIC $(opt) $(dbg) -I../yoyo-util -I../yoyo-runtime -I../yoyo-runtime/headers -I../yoyo-parser/headers -I../yoyo-vm/headers $(misc) -o libyjit.so ../yoyo-jit/*.cpp -lasmjit
yoyo:
	./gen_opcodes.sh
	$(cc) $(cflags) -o $(yoyo) $(c_source) 
libyoyo:
	./gen_opcodes.sh
	$(cc) $(cflags) -shared -fPIC -o $(libyoyo.so) $(c_source)
yoll:
	$(cpp) $(cppflags) -o $(yoll) $(cpp_source)  `llvm-config --system-libs --libs core` $(cpp_platform)
$(jyoyo):
	@mkdir -p .jyoyo-build
	find .. -name "*.java" | xargs javac -d .jyoyo-build
	jar cf $(jyoyo) -C .jyoyo-build yoyo
	javah -d ../yji/yoyo-jni/yoyo-jni/ -classpath .jyoyo-build yoyo.runtime.YRuntime yoyo.runtime.ILBytecode yoyo.runtime.ILProcedure yoyo.yji.YRunnable yoyo.yji.YBindings
	@rm -rf .jyoyo-build
yji:
	$(cc) $(cflags) -std=gnu11 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -shared -fPIC -o $(yji) -I/usr/lib/jvm/default/include -I/usr/lib/jvm/default/include/linux ../yji/yoyo-jni/*.c -L/usr/lib/jvm/default/jre/lib/amd64/server -ljvm
clean:
	@rm -rf $(yoyo)
	@rm -rf $(jyoyo)
	@rm -rf $(yji)
	@rm -rf .jyoyo-build
