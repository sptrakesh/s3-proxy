#!/bin/sh

CACHE_DIR="/opt/spt/data"
LOGDIR=/opt/spt/logs
MMDB_HOST="mmdb-ws"
MMDB_PORT=2010
MONGO_DATABASE="metrics"
MONGO_COLLECTION="request"
PORT=8000
THREADS=8
TTL=300
args="--mmdb-host $MMDB_HOST --mmdb-port $MMDB_PORT"
args="$args --mongo-uri $MONGO_URI --mongo-database $MONGO_DATABASE --mongo-collection $MONGO_COLLECTION"
args="$args --reject-query-strings $REJECT_QUERY_STRINGS"
gdb -ex run --args /opt/spt/bin/s3proxy --dir ${LOGDIR}/ --log-level $LOG_LEVEL \
    --ttl $TTL --cache-dir $CACHE_DIR --port $PORT --threads $THREADS \
    --region "$AWS_REGION" --bucket "$S3_BUCKET" \
    --key "$AWS_KEY" --secret "$AWS_SECRET" \
    --auth-key $AUTH_KEY $args
