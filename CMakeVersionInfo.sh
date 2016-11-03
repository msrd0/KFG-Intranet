#!/bin/bash

dir="`realpath "$1"`"

#pushd "`dirname "$0"`" >/dev/null
cd "`dirname "$0"`"

if [ -d .git ]
then
	version=$(git describe --tags --abbrev=0 | tr -d v)
	vername=$(git describe --tags --dirty)
else
	version="?.?.?"
	vername="v?.?.?-unknown"
fi

#popd >/dev/null

echo "// AUTO GENERATED - DO NOT EDIT
#pragma once

#define INTRANET_VERSION \"$version\"
#define INTRANET_VERSION_STRING \"$vername\"
" >"$dir"/intranet.h
