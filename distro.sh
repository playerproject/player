#!/bin/bash
# BPG modified RTV 5Dec2000 from BPG's original
# makes a distribution
# e.g. if player is a top level directory for some great software
# ./distro.sh player 1.1 
# creates the distribution file Player-1.1.tgz 

# first argument is the software directory
DIRECTORY=$1

#force the first character of the distro name to upper case
SOFTWARE=$(echo $DIRECTORY | cut -c1-1 | tr [:lower:] [:upper:])$(echo $DIRECTORY | cut -c2-0)

#second argument is the version number for the distro
VERSION=$2

#these are combined to form the distro name
DISTRO=$SOFTWARE-$VERSION-src

echo "Creating distribution $DISTRO from directory $DIRECTORY"
echo "Removing old version link $DISTRO"
rm -f $DISTRO
echo "Creating new version link $DISTRO"
ln -s $DIRECTORY $DISTRO
#echo "Making clean"
#cd $DISTRO && make clean && cd ..
echo "Removing any exisiting tarball $DISTRO.tgz"
/bin/rm -f $DISTRO.tgz
echo "Creating tarball $DISTRO.tgz, excluding CVS directories"
/bin/tar hcvzf $DISTRO.tgz $DISTRO --exclude "*CVS" --exclude "\.#*" --exclude "*distro.sh" --exclude "*copyright_header" --exclude "*java*" --exclude "*python*" --exclude "newc++*" --exclude "*player-manual*"
echo "Removing version link $DISTRO"
rm -f $DISTRO
echo "File $DISTRO.tgz contains $SOFTWARE v.$VERSION"
echo "Done." 
