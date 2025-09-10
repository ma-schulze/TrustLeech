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

# NAME="nitrogen8m"
# TAG="latest"

if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    exit 1
fi

docker image load -i $1

echo "Leaving the script '$DIR/$FILE'"
