#!/usr/bin/env bash

# Upload html documentation to SF.net

# srcdir: path to directory containing documentation (e.g., server/doc/html)
#
# destdir: path to copy to on the server, relative to htdocs/doc 
#          (e.g., Player-cvs-html)
#
# name: name for directory on server (e.g., player)

USAGE="USAGE: upload_doc.sh <srdcir> <destdir> <name>"

if [ $# -ne 3 ]; then
  echo $USAGE
  exit
fi

# Figure out the SF user name; define SFUSER in your startup scripts
# is this is different from your username.
if [ -n "$SFUSER" ]; then 
  _SFUSER=$SFUSER
else
  _SFUSER=$USER
fi
echo "assuming your SourceForge username is $_SFUSER..."

# Document destination and name
SRC=$1
DEST=$2
NAME=$3

echo "building tarball $NAME.tgz from $SRC..."

# Create a tarball with the correct name and layout
mkdir -p tmpdoc
rm -rf tmpdoc/$NAME
cp -r $SRC tmpdoc/$NAME
cd tmpdoc
tar cvzf $NAME.tgz $NAME

echo "copying $NAME.tgz to $DEST on server"

# Copy tarball to website
scp $NAME.tgz $SFUSER@shell.sourceforge.net:/home/groups/p/pl/playerstage/htdocs/doc/$DEST

echo "unpacking $NAME.tgz on server"

# Untar the file and re-jig the permissions
ssh $_SFUSER@shell.sourceforge.net 'cd /home/groups/p/pl/playerstage/htdocs/doc/'$DEST'; tar xvzf '$NAME'.tgz; find . -type d | xargs chmod -fR 2775; find . -type f | xargs chmod -fR 664'
