#!/bin/sh

LOGDIR=/opt/spt/logs
ttl=''

Check()
{
  if [ -z "$AWS_REGION" ]
  then
    echo "AWS_REGION not set.  Will default to us-east-1"
  fi

  if [ -z "$S3_BUCKET" ]
  then
    echo "S3_BUCKET must be set."
    exit 1
  fi

  if [ -z "$AWS_KEY" ]
  then
    echo "AWS_KEY must be set."
    exit 2
  fi

  if [ -z "$AWS_SECRET" ]
  then
    echo "AWS_SECRET must be set."
    exit 3
  fi
}

Defaults()
{
  if [ -z "$AUTH_KEY" ]
  then
    AUTH_KEY="2YnjxYUcQ30IVYxhC/818VsfoRrL9yt9qPsOxNx5YVM="
    echo "AUTH_KEY not set.  Will default to $AUTH_KEY"
  fi

  if [ -z "$TTL" ]
  then
    TTL=300
    echo "TTL not set.  Will default to $TTL (s)"
  fi

  if [ -z "$CACHE_DIR" ]
  then
    CACHE_DIR="/opt/spt/data"
    echo "CACHE_DIR not set.  Will default to $CACHE_DIR"
  fi

  if [ -z "$PORT" ]
  then
    PORT=8000
    echo "PORT not set.  Will default to $PORT"
  fi

  if [ -z "$THREADS" ]
  then
    THREADS=8
    echo "THREADS not set.  Will default to $THREADS"
  fi
}

Service()
{
  if [ ! -d $LOGDIR ]
  then
    mkdir -p $LOGDIR
    chown spt:spt $LOGDIR
  fi

  echo "Starting up AWS S3 proxy server"
  /opt/spt/bin/s3proxy -c true -o ${LOGDIR}/ \
    -t $TTL -d $CACHE_DIR -p $PORT -n $THREADS \
    -r "$AWS_REGION" -b "$S3_BUCKET" \
    -k "$AWS_KEY" -s "$AWS_SECRET" \
    -a $AUTH_KEY
}

Check && Defaults && Service