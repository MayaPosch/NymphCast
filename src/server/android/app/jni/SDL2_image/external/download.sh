#!/bin/sh

set -e

cd $(dirname "$0")/..
cat .gitmodules | \
while true; do
    read module || break
    read line; set -- $line
    path=$3
    read line; set -- $line
    url=$3
    read line; set -- $line
    branch=$3
    git clone $url $path -b $branch --recursive
done
