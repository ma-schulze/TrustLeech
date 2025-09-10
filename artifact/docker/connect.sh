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

docker exec -it \
  -u $UID \
  trustleech \
  "/bin/bash"
