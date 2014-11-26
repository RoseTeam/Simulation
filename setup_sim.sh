#!/bin/bash

GAZEBO_HOME_DIR=~/.gazebo/

echo
echo
echo "create links from github dir to gazebo home dir in $GAZEBO_HOME_DIR"

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )


if ! [ -a $GAZEBO_HOME_DIR ]; then
  echo "ERROR: Cannot find Gazebo home dir at $GAZEBO_HOME_DIR: have you installed gazebo?"
  echo "Abort"
  exit 1
fi


GAZEBO_PLUGIN_DIR="$GAZEBO_HOME_DIR"plugins/
GAZEBO_MODELS_DIR="$GAZEBO_HOME_DIR"models/

echo
echo "link to models"
for dirpath in $(find $DIR/models -maxdepth 1 -mindepth 1 -type d); do

  dir=$(basename $dirpath)
  rm -f $GAZEBO_MODELS_DIR$dir
  CMD="ln -s $dirpath $GAZEBO_MODELS_DIR$dir"
  echo $CMD
  eval $CMD

done

echo
echo "link to world file"
WORLD_FILE=robomovies.world
rm -f $GAZEBO_HOME_DIR$WORLD_FILE
CMD="ln -s $DIR/worlds/$WORLD_FILE $GAZEBO_HOME_DIR$WORLD_FILE"
echo $CMD
eval $CMD

echo
echo "link to plugins libs"
shopt -s nullglob
for libfilepath in $DIR/plugins/build/*.so
do
  libfile=$(basename $libfilepath)
  rm -f $GAZEBO_PLUGIN_DIR$libfile
  CMD="ln -s $libfilepath $GAZEBO_PLUGIN_DIR$libfile"
  echo $CMD
  eval $CMD
done
