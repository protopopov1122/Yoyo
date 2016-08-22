#!/bin/bash

make -B $1 misc+="-fprofile-generate"
./yoyo -Dystd=../YStd ../tests/pgo.yoyo $2
make -B $1 misc+="-fprofile-use -fprofile-correction"

