#!/bin/bash
# Exit immediately and do not process any further if one of the following
# commands fail.
set -e

# Extract the directory of this script and its exact name, so that this script
# can be called from anywhere.
DIR="$( cd "$( dirname "$0" )" && pwd )"
FILE="$( basename "$0" )"
echo "Running the script '$DIR/$FILE'"
cd $DIR/../..

################################################################################

NAME="trustleech"
TAG="latest"

REPO_DIR="$( pwd )"

docker run -it --rm \
  --name trustleech \
  -u $UID  \
  --ulimit "nofile=1024:1048576" \
  --volume $REPO_DIR:/home/user/trustleech \
  --volume $REPO_DIR/src/buildroot-ccache:/home/user/.buildroot-ccache \
  $NAME:$TAG \
  "cd /home/user/trustleech/artifact/buildroot && make all && cp /home/user/trustleech/artifact/buildroot/output/images/rootfs.ext4 /home/user/trustleech/artifact/overlay && make all && /bin/bash"

