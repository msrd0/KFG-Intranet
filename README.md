# KFG-Intranet [![Build Status](https://travis-ci.com/msrd0/KFG-Intranet.svg?token=fqEVUjqYjQFvWLurRvUX&branch=http2)](https://travis-ci.com/msrd0/KFG-Intranet)

*Although this project itself is stable, this branch uses the unstable version of QtWebApp from the http2 branch!*

You either need to clone this repository with the --recursive option or call `git submodule init` and `git submodule update` after cloning.

## Build & Run

1. Make sure `gcc` (>= 4.8) or `clang` (>= 3.5) and `Qt5` are both installed.
2. Create `/usr/share/intranet` with read & write privileges for the user that should execute the intranet.
3. Run `./intranet.sh release` - That's it!

## Copyright

The code can be used under the terms of the GPL License version 3 or higher. See the `LICENSE` file in the repository.
