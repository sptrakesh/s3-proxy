#!/bin/sh

cd `dirname $0`/..
. docker/env.sh

Docker()
{
  docker buildx build --builder mybuilder --platform linux/arm64,linux/amd64 --compress --force-rm -f docker/Dockerfile --push -t sptrakesh/$NAME:$VERSION -t sptrakesh/$NAME:latest .
}

if [ "$1" = "local" ]
then
  docker build --compress --force-rm -f docker/Dockerfile -t $NAME .
else
  Docker
fi
