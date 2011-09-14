#!/bin/bash

# Called as: $0 [start-git-md5] [end-git-md5] [year]
#set -x

START=$1
END=$2
YEAR=$3

for i in `git diff --name-only $START $END`
do
  if [ -f $i ]
      then
      ./xorp/devnotes/update_copyright.pl $YEAR $i
  fi
done

