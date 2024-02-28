#!/bin/sh

LOGDIR=/opt/spt/logs
ttl=''

FromDb()
{
  cmd="/opt/spt/bin/configctl -s $1 -p $2"
  if [ -n "$3" ]
  then
    cmd="$cmd $3"
  fi

  status=1
  count=0
  echo "Checking if $1 is available"
  while [ $status -ne 0 ]
  do
    echo "[$count] Config-db Service $1:$2 not available ($status).  Sleeping 1s..."
    count=$(($count + 1 ))
    sleep 1
    nc -z $1 $2
    status=$?
  done

  status=1
  count=0
  echo "Checking if $1 has been bootstrapped"
  while [ $status -ne 0 ]
  do
    echo "[$count] Config-db Service $1:$2 not bootstrapped ($status).  Sleeping 1s..."
    count=$(($count + 1 ))
    sleep 1
    $cmd -a get -k /database/mongo/uri
    status=$?
  done

  if [ -n "$4" ]
  then
    export ALLOWED_ORIGINS=`$cmd -a get -k $4`
  else
    MONGO_URI=`$cmd -a get -k /database/mongo/uri`
    MONGO_URI=`/opt/spt/bin/encrypter -d $MONGO_URI`
  fi
}

ConfigDb()
{
  if [ -z "$CONFIG_DB" ]
  then
    return
  fi

  server=`echo $CONFIG_DB | cut -d ':' -f1`
  port=`echo $CONFIG_DB | cut -d ':' -f2`
  FromDb $server $port
}

SecureConfigDb()
{
  if [ -z "$SECURE_CONFIG_DB" ]
  then
    return
  fi

  server=`echo $SECURE_CONFIG_DB | cut -d ':' -f1`
  port=`echo $SECURE_CONFIG_DB | cut -d ':' -f2`
  FromDb $server $port "--with-ssl"
}

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

  if [ -z "$LOG_LEVEL" ]
  then
    LOG_LEVEL="info"
    echo "LOG_LEVEL not set.  Will default to $LOG_LEVEL"
  fi
}

Extras()
{
  if [ -z "$MMDB_PORT" ]
  then
    MMDB_PORT=2010
    echo "MMDB_PORT not set.  Will default to $MMDB_PORT"
  fi

  if [ -z "$MMDB_HOST" ]
  then
    echo "MMDB_HOST not set.  Database and MMDB integration disabled"
  fi

  if [ -z "$AKUMULI_HOST" ]
  then
    echo "AKUMULI_HOST not set.  Akumuli integration disabled"
  fi

  if [ -z "$AKUMULI_PORT" ]
  then
    AKUMULI_PORT=8282
    echo "AKUMULI_PORT not set.  Will default to $AKUMULI_PORT"
  fi

  if [ -z "$METRIC_PREFIX" ]
  then
    METRIC_PREFIX="request"
    echo "METRIC_PREFIX not set.  Will default to $METRIC_PREFIX"
  fi

  if [ -z "$MONGO_URI" ]
  then
    echo "MONGO_URI not set.  Database integration disabled"
  fi

  if [ -z "$MONGO_DATABASE" ]
  then
    MONGO_DATABASE="metrics"
    echo "MONGO_DATABASE not set.  Will default to $MONGO_DATABASE"
  fi

  if [ -z "$MONGO_COLLECTION" ]
  then
    MONGO_COLLECTION="request"
    echo "MONGO_COLLECTION not set.  Will default to $MONGO_COLLECTION"
  fi

  args=""
  if [ -n "$MMDB_HOST" ]
  then
    if [ -n "$AKUMULI_HOST" ] || [ -n "$MONGO_URI" ]
    then
      args="--mmdb-host $MMDB_HOST --mmdb-port $MMDB_PORT"
      if [ -n "$AKUMULI_HOST" ]
      then
        args="$args --akumuli-host $AKUMULI_HOST --akumuli-metric-prefix $METRIC_PREFIX --akumuli-port $AKUMULI_PORT"
      fi
      if [ -n "$MONGO_URI" ]
      then
        args="$args --mongo-uri $MONGO_URI --mongo-database $MONGO_DATABASE --mongo-collection $MONGO_COLLECTION"
      fi
    fi
  fi

  if [ -n "$REJECT_QUERY_STRINGS" ]
  then
    args="$args --reject-query-strings $REJECT_QUERY_STRINGS"
  fi

  if [ -n "$ALLOWED_ORIGINS_KEY" ]
  then
    if [ -n "$SECURE_CONFIG_DB" ]
    then
      FromDb $server $port "--with-ssl" $ALLOWED_ORIGINS_KEY
    elif [ -n "$CONFIG_DB" ]
    then
      FromDb $server $port ' ' $ALLOWED_ORIGINS_KEY
    else
      echo "ConfigDb not configured"
    fi
  fi
}

Wait()
{
  if [ -n "$NO_CHECK" ]
  then
    return 0
  fi

  if [ -n "$MONGO_URI" ]
  then
    status=1
    count=0
    echo "Checking if Mongo Host is available"
    while [ $status -ne 0 ]
    do
      mh=`echo $MONGO_URI | awk -F/ '{print $3}' | awk -F@ '{print $2}' | awk -F: '{print $1}'`
      mp=`echo $MONGO_URI | awk -F/ '{print $3}' | awk -F@ '{print $2}' | awk -F: '{print $2}'`
      if [ -z "$mp" ]
      then
        mp='27017'
      fi
      echo "[$count] Mongo Service $mh:$mp not available ($status).  Sleeping 1s..."
      count=$(($count + 1 ))
      sleep 1
      nc -z $mh $mp
      status=$?
    done
  fi

  if [ -n "$MMDB_HOST" ]
  then
    status=1
    count=0
    echo "Checking if $MMDB_HOST is available"
    while [ $status -ne 0 ]
    do
      echo "[$count] MMDB Service $MMDB_HOST:$MMDB_PORT not available ($status).  Sleeping 1s..."
      count=$(($count + 1 ))
      sleep 1
      nc -z $MMDB_HOST $MMDB_PORT
      status=$?
    done
  fi
}

runService()
{
  if [ -n "$DEBUG" ]
  then
    echo "DEBUG specified, shell into container and start up with gdb"
    tail -f /dev/null
  fi

  echo "`date` Starting up AWS S3 proxy server" >> /tmp/start.log
  /opt/spt/bin/s3proxy --console true --dir ${LOGDIR}/ --log-level $LOG_LEVEL \
    --ttl $TTL --cache-dir $CACHE_DIR --port $PORT --threads $THREADS \
    --region "$AWS_REGION" --bucket "$S3_BUCKET" \
    --key "$AWS_KEY" --secret "$AWS_SECRET" \
    --auth-key $AUTH_KEY $args
}

Service()
{
  while true
  do
    runService
  done
}

ConfigDb && SecureConfigDb && Check && Defaults && Extras && Wait && Service