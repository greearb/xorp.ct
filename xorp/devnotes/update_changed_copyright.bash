#!/bin/bash

# Called as: $0 [start-git-md5] [end-git-md5]
#set -x

START=$1
END=$2

for i in `git diff --name-only $START $END`
do
  if [ -f $i ]
      then
      ./xorp/devnotes/update_copyright.pl 2011 $i
  fi
done

