#!/usr/local/bin/python

# copyright a.greenhalgh@cs.ucl.ac.uk, 2004
# use entirely at your own risk.

# Things to fix 
# - hard coded permission of 755
# - hard coded manifest location
# - code in floppy location , can use manifest file for now

import sys
import shutil
import os
import pwd 
import grp

# File entry group
class FileEntry:
  def __init__(self,source,destination,permission,owner,group,md5):
    self.source = source
    self.destination = destination
    self.permission = permission
    self.owner = owner
    self.group = group
    self.md5 = md5
  def print_fields(self):
    return "source : " + self.source + " destination : " + self.destination + " permission : " + self.permission + " owner : " + self.owner + " group : " + self.group + " md5 : " + self.md5;
  
# Load manifest
def loadManifest(filename):
  manifest = [];
  manifest_file = open(filename,'r')
  lines = manifest_file.readlines()
  manifest_file.close()

  for j in lines:
    i = j.split();
    x = FileEntry(i[0],i[1],i[2],i[3],i[4],i[5])
    manifest.append(x);
  return manifest

# Copy file in
def copyFileIn(fe):
  shutil.copyfile(fe.source,fe.destination);
  os.chown(fe.destination,pwd.getpwnam(fe.owner).pw_uid,grp.getgrnam(fe.group).gr_gid);
  os.chmod(fe.destination, int(fe.permission, 8));
  print "copied in file " + fe.source + "."

# Copy all files in
def copyAllFilesIn(manifest):
  print "Copying All files to floppy."
  for i in manifest:
    copyFileIn(i)

# Copy file out

def copyFileOut(fe):
  shutil.copyfile(fe.destination,fe.source);
  os.chown(fe.source,pwd.getpwnam(fe.owner).pw_uid,grp.getgrnam(fe.group).gr_gid);
  os.chmod(fe.source,0755);
  print "copied out file " + fe.destination + "."

# Copy all files out

def copyAllFilesOut(manifest):
  print "Copying All files to floppy."
  for i in manifest:
    copyFileOut(i)

# Save file from os to disk, only called by router manager

def singleFileSaveToFloppy(filename):
  manifest_file_location = "manifest.dat";
  manifest_files = loadManifest(manifest_file_location)
  for f in manifest_files:
    if filename == f.destination:
	copyFileOut(f)
	return
	

def helpMsg(arguments):
	print "Wrong number of arguments."
        print " " + arguments[0] + " <filename>"
        print "    called by rtrmgr when a file is saved"
	print " " + arguments[0] + " -mode loadin <manifest file>"
	print "    loads files on bootup"
	print " " + arguments[0] + " -mode saveout <manifest file>"
        print "    save files to floppy on system shutdown"
	return 1

# Main

arguments = sys.argv

#### 1 argument - help
if len(arguments) == 1:
   sys.exit(helpMsg(arguments))
  
#### 2 arguments - save file to floppy

if len(arguments) == 2 :
	singleFileSaveToFloppy(arguments[1])

#### 3 arguments - nothing
if len(arguments) == 3:
  sys.exit(helpMsg(arguments))

#### 4 arguments various modes

if len(arguments) == 4:
  if arguments[1] == "-mode":

    if arguments[2] == "loadin" :
	copyAllFilesIn ( loadManifest(arguments[3]) )
    else :
      if arguments[2] == "saveout":
	copyAllFilesOut ( loadManifest(arguments[3]) )
      else :
        sys.exit(helpMsg(arguments))
  else : 
    sys.exit(helpMsg(arguments))		

#### > 4 arguments - nothing
if len(arguments) > 4:
  sys.exit(helpMsg(arguments))
