#!/bin/sh

if which ninja >/dev/null; then
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    ninja -C build  $1
else
    cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    make -j $(getconf _NPROCESSORS_ONLN) -C build $1 && \
    echo "done. P.S.: Consider installing ninja - it's faster"
fi

if test -d /volumes/RPI-RP2; then
    echo "###############################"
    echo "## Copied to /volumes/RPI-RP2 #"
    echo "###############################"
    cp build/OpenAce.uf2 /volumes/RPI-RP2

else
    echo "########################"
    echo "## Please mount the PI #"
    echo "########################"
fi

exit $?