#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

docker build -t victor .
docker build -t victor-${USER} -f Dockerfile-mess.dev --build-arg USER=${USER} --build-arg UID=$(id -u $USER) --build-arg VIC_DIR=${DIR}/../ .
docker run \
       -u ${USER} -it \
       -v ~/:/home/${USER}/:delegated \
       --privileged victor-${USER}
