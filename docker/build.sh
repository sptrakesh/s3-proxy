#!/bin/sh

DIR=`dirname $0`
. $DIR/env.sh

Docker()
{
  (cd $DIR/..;
    docker build --compress --force-rm -f docker/Dockerfile -t $NAME .)
}

Docker