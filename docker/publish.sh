#!/bin/sh

. `dirname $0`/env.sh
MONGO_VERSION=3.5.0
docker tag mongocxx-boost sptrakesh/mongocxx-boost:$MONGO_VERSION
docker push sptrakesh/mongocxx-boost:$MONGO_VERSION
docker tag sptrakesh/mongocxx-boost:$MONGO_VERSION sptrakesh/mongocxx-boost:latest
docker push sptrakesh/mongocxx-boost:latest
