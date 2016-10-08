# KFG-Intranet [![Build Status](https://travis-ci.com/msrd0/KFG-Intranet.svg?token=fqEVUjqYjQFvWLurRvUX)](https://travis-ci.com/msrd0/KFG-Intranet)

To clone this repository, just execute

```bash
git clone --depth=1 https://msrd0.duckdns.org/git/msrd0/kfg-intranet.git
```

## Build & Run

1. Make sure a compiler supporting C++11, CMake and Qt5 are installed
2. Download https://github.com/msrd0/QtWebApp and install it
3. Go to the source directory and execute
	```bash
	mkdir build && cd build
	cmake -DCMAKE_BUILD_TYPE=Release ..
	make -j4
	sudo make install
	```

4. To execute the code, just enter
	```bash
	intranet
	```
	If you encounter any problems, try this:
	```bash
	LD_LIBRARY_PATH=/usr/local/lib intranet
	```

## Copyright

The code can be used under the terms of the GPL License version 3 or higher. See the `LICENSE` file in the repository.
