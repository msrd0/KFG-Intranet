#!/bin/bash

if [ "$1" == "--help" ]; then
        echo "Usage: $0 [release]"
        echo "       $0 --help"
        exit 0
fi

debug="CONFIG+=debug"
if [ "$1" == "release" ]; then
        debug=""
fi

cwd="$PWD"
cd "`dirname "$0"`"

echo -e "\e[1m==> Building intranet ...\e[0m"
mkdir -p build/
cd build/
qmake ../KFG-Intranet.pro "$debug" || exit 1
make -j4 || exit 1

export "PATH=$PWD:$PATH"
export "LD_LIBRARY_PATH=$PWD:$LD_LIBRARY_PATH"
for path in "$PWD"/*/
do
	export "PATH=$path:$PATH"
	export "LD_LIBRARY_PATH=$path:$LD_LIBRARY_PATH"
done
echo $PATH
echo $LD_LIBRARY_PATH
cd "$cwd"

echo -e "\e[1m==> Starting intranet ...\e[0m"
echo -e "run \"`dirname "$0"`/config.ini\"\nbt\nq\ny" | gdb -q "`dirname "$0"`/build/Intranet/intranet"