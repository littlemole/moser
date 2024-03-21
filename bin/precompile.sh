#!/bin/bash
cd lib
find -type f -name "*.mbc" -exec rm {} \;
find -type f -not -name "*.mbc" -not -name "*.dll"  -not -name "*.winmd" -exec bash -c " echo {} &&  ../build/moser -c {}" \;
cd ..
cp -r lib build
