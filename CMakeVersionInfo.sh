#!/bin/bash

pushd "`dirname "$0"`" >/dev/null

if [ -d .git ]
then
	version=$(git describe --tags --abbrev=0 | tr -d v)
	vername=$(git describe --tags --dirty)
else
	version="?.?.?"
	vername="v?.?.?-unknown"
fi

popd >/dev/null

echo "// AUTO GENERATED - DO NOT EDIT
#pragma once

#define INTRANET_VERSION \"$version\"
#define INTRANET_VERSION_STRING \"$vername\"
" >"$1"/intranet.h
