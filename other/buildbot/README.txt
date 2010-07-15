

To have your own buildbot:

Install buildbot 7.12 or later
Ask Ben Greear for a user/password for your bot.
Run this command to build the slave in a directory of
your choosing:
buildbot create-slave slave dmz2.candelatech.com:9989 [bot-name] [bot-passwd]
vi slave/info/admin
vi slave/info/host
buildbot start slave


To set up a build-bot master, do normal buildbot install, then use
something similar to the master.cfg file located in this directory.
After renaming the Makefile.sample files to Makefile in the buildbot
directory, the following crontab for buildbot starts everything on
boot:

@reboot cd /home/buildbot/xorp.ct.github && make start
@reboot cd /home/buildbot/slaves/xorp.ct/slave && make start
@reboot /usr/local/bin/github_buildbot.py&


This file details the packages added to the default installations of the different os's used in the buildbot system. Dependencies aren't listed.

debian 5.0.4 sparc (buildbot3.cs.ucl.ac.uk)

openssh-server
subversion
scons
build-essential
libboost-regex-dev 
libssl-dev 
libpcap-dev
buildbot

fedora 12 i386 (buildbot1.cs.ucl.ac.uk)

subversion
scons
boost-devel
boost-regex
openssl-devel
buildbot
libpcap-devel
gcc
gcc-c++

ubuntu 9.10 i386 (buildbot4.cs.ucl.ac.uk)

openssh-server
buildbot 
scons 
subversion 
libssl-dev 
libpcap-dev 
libboost-regex1.40-dev 
build-essential
libncurses-dev

freebsd 8.0 i386 (buildbot2.cs.ucl.ac.uk)

devel/scons
devel/subversion
lang/gcc42
devel/boost-libs
devel/buildbot

master config

set the buildbot to retry the checkout 5 times with a 10 second delay between each try.
