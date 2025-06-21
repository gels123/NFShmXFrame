#! /bin/sh

status=`git status -s`
if [[ ${status} == "" ]]; then
    echo "None Proto Change!!!"
    exit 0
fi

git commit -am "auto commit all"

git push
