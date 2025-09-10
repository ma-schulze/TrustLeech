#!/bin/bash
# Exit immediately and do not process any further if one of the following
# commands fail.
set -e

# Extract the directory of this script and its exact name, so that this script
# can be called from anywhere.
DIR="$( cd "$( dirname "$0" )" && pwd )"
FILE="$( basename "$0" )"
echo "Running the script '$DIR/$FILE'"
cd $DIR

################################################################################

NAME="trustleech"
TAG="latest"

USER_ID=$(id -u)
GROUP_ID=$(id -g)
# USER_ID=999
# GROUP_ID=999
docker build --no-cache -t $NAME:$TAG --build-arg USER_ID=$USER_ID --build-arg GROUP_ID=$GROUP_ID .

echo "Leaving the script '$DIR/$FILE'"
