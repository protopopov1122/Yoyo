target=release
platform=linux
arch=x64
native=no

PREFIX=
BIN_DIRECTORY=$(PREFIX)/usr/bin
SHARE_DIRECTORY=$(PREFIX)/usr/share
YSTD=$(SHARE_DIRECTORY)/yoyo/YStd

native_yes=-march=native
native_no=

cc_linux_x86=gcc -m32 -mtune=i686 
cc_linux_x64=gcc -m64 -mtune=generic
cc_win_x86=i686-w64-mingw32-gcc
cc_win_x64=x86_64-w64-mingw32-gcc
cc_freebsd_x86=clang -m32 -mtune=i686
cc_freebsd_x64=clang -m64 -mtune=generic
cxx_linux_x86=g++ -m32 -mtune=i686
cxx_linux_x64=g++ -m64 -mtune=generic
cxx_win_x86=i686-w64-mingw32-g++
cxx_win_x64=x86_64-w64-mingw32-g++
cc=$(cc_$(platform)_$(arch))
cxx=$(cxx_$(platform)_$(arch))

c_lang=$(native_$(native)) -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror
cxx_lang=$(native_$(native)) -std=c++11 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror

opt_debug=-O0
opt_release=-O3
opt=$(opt_$(target))

dbg_debug=-g3 -ggdb -pg
dbg_release=
dbg=$(dbg_$(target))

misc=

c_platform_linux=-rdynamic -lm -lpthread -ldl -DYSTD=$(YSTD)
c_platform_win=-static -Wno-pedantic -Wno-unused-result -IWin32 Win32/*.c -Wl,-export-all-symbols -lm -lpsapi -lpthread
c_platform_freebsd=-rdynamic -lc -lm -lpthread -lexecinfo -DYSTD=$(YSTD)
c_platform=$(c_platform_$(platform)) $(misc)
cxx_platform_linux=-lasmjit
cxx_platform_win=
cxx_platform=$(cxx_platform_$(platform)) $(misc)

cflags=$(c_lang) $(opt) $(dbg) -I../yoyo-util -I../yoyo-runtime/headers -I../yoyo-parser/headers -I../yoyo-vm/headers -I../yoyo-runtime/collections
c_source=../yoyo-util/*.c ../yoyo-vm/*.c ../YStd/native/*.c ../yoyo-parser/*.c ../yoyo-runtime/*.c ../yoyo-runtime/types/*.c ../yoyo-runtime/collections/*.c
cxxflags=$(cxx_lang) $(opt) $(dbg) -I../yoyo-util -I../yoyo-runtime/headers -I../yoyo-parser/headers -I../yoyo-vm/headers -I../yoyo-runtime/collections

yoyo_linux=yoyo
yoyo-jit_linux=libyoyojit.so
yoyo_win=yoyo.exe
yoyo_freebsd=yoyo
yoyo=$(yoyo_$(platform))
yoyo-jit=$(yoyo-jit_$(platform))

all: yoyo
install:
	@install -D $(yoyo) $(BIN_DIRECTORY)/$(yoyo)
	@mkdir -p $(YSTD)
	@cp -r ../YStd/*.yoyo $(YSTD)
	@cp ../examples/repl.yoyo $(YSTD)
uninstall:
	@rm -rf $(SHARE_DIRECTORY)/yoyo
	@rm -rf $(BIN_DIRECTORY)/$(yoyo)
yoyo:
	./gen_opcodes.sh
	$(cc) $(cflags) $(c_source) -o $(yoyo) $(c_platform)
yoyo-jit:
	$(cxx) $(cxxflags) -fPIC -shared ../yoyo-jit/*.cpp -o $(yoyo-jit) $(cxx_platform)
yoyo-myjit:
	$(cc) $(cflags) -c -fPIC -Imyjit/myjit ../yoyo-myjit/*.c
	$(cc) -shared -o $(yoyo-jit) myjit/*.o *.o $(c_platform)
clean:
	@./clean.sh
