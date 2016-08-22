target=release
platform=linux
arch=x64

PREFIX=
BIN_DIRECTORY=$(PREFIX)/usr/bin
SHARE_DIRECTORY=$(PREFIX)/usr/share
YSTD=$(SHARE_DIRECTORY)/yoyo/YStd


cc_linux_x86=gcc -m32 -mtune=i686
cc_linux_x64=gcc -m64 -mtune=generic
cc_win_x86=i686-w64-mingw32-gcc
cc_win_x64=x86_64-w64-mingw32-gcc
cc=$(cc_$(platform)_$(arch))

c_lang=-std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -pedantic -Werror

opt_debug=-O0
opt_release=-O3
opt=$(opt_$(target))

dbg_debug=-g3 -ggdb -pg
dbg_release=
dbg=$(dbg_$(target))

misc=

c_platform_linux=-rdynamic -lm -lpthread -ldl -DYSTD=$(YSTD)
c_platform_win=-static -Wno-pedantic -Wno-unused-result -IWin32 Win32/*.c -Wl,-export-all-symbols -lm -lpsapi -lpthread
c_platform=$(c_platform_$(platform)) $(misc)

cflags=$(c_lang) $(opt) $(dbg) -I../yoyo-util -I../yoyo-runtime/headers -I../yoyo-parser/headers -I../yoyo-vm/headers
c_source=../yoyo-util/*.c ../yoyo-vm/*.c ../YStd/native/*.c ../yoyo-parser/*.c ../yoyo-runtime/*.c ../yoyo-runtime/types/*.c

yoyo_linux=yoyo
yoyo_win=yoyo.exe
yoyo=$(yoyo_$(platform))

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
	$(cc) $(cflags) -o $(yoyo) $(c_source) $(c_platform)
clean:
	@./clean.sh
