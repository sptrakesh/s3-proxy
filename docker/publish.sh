#!/bin/sh

. `dirname $0`/env.sh
docker tag $NAME sptrakesh/$NAME:$VERSION
docker push sptrakesh/$NAME:$VERSION
docker tag sptrakesh/$NAME:$VERSION sptrakesh/$NAME:latest
docker push sptrakesh/$NAME:latest
