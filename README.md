# s3-proxy

* [Versions](#versions)
* [Configuration](#configuration)
    * [Metric](#metric)
        * [BSON Document](#bson-document)
        * [Akumuli Series](#akumuli-series)
        * [Akumuli Tags](#akumuli-tags)
* [Testing](#testing)
* [Docker](#docker)
    * [Run Locally](#run-locally)
* [Cache Management](#cache-management)
* [Acknowledgements](#acknowledgements)

A simple proxy server for serving contents of an AWS S3 bucket as a static website.
Useful in cases where it is not feasible to use traditional CloudFront distribution
or similar CDN on top of a bucket.

In particular it is possible to serve static content from a private S3 bucket.

## Versions
Two versions (branches) of the service are available.  The original version
(tracked in the `master` branch) runs the service as a HTTP/1.1 service using
Boost:Beast.  The other version (tracked in the `http2` branch) runs the service
as a pure HTTP/2 service using [nghttp2](https://github.com/nghttp2/nghttp2).
The version are published to docker hub as the following:
* `0.x.x` - The original HTTP/1.1 service.
* `2.x.x` - The new HTTP/2 service.

## Configuration
The server can be configured with the following options:
* `--auth-key` - A `bearer token` value to use to access internal management
API endpoints.
    * At present the only management endpoint is `/_proxy/_private/_cache/clear`
     to clear the cache.
* `--key` - The *AWS Access key ID* to use to authorise the AWS SDK.
* `--secret` - The *AWS Secret access key* to use to authorise the AWS SDK.
* `--region` - The *AWS Region* in which the `bucket` exists.
* `--bucket` - The target *AWS S3 Bucket* from which objects are retrieved.
* `--port` - The *port* on which the server listens.  Default `8000`.
* `--threads` - Number of I/O threads for the server.  Defaults to `std::thread::hardware_concurrency`.
* `--ttl` - Cache expiry TTL in seconds.  Default `300`.  All S3 objects are
cached for efficiency.
* `--cache-dir` - The directory under which cached S3 objects are stored.
    * Objects are stored under a filename that is the result of invoking
    `std::hash<std::string>` on the full path of the object.
    * Since hash values are used as filenames, the directory structure will be
    *flat*.  Makes it easy to have external jobs (eg. `cron` jobs) that delete
    the files on a regular basis.  No real need for it, unless a lot of files
    have been removed from the backing S3 bucket.
* `--reject-query-strings` - A flag to indicate that requests that contain query
strings should be rejected as bad request.  Query strings in the context of serving
out S3 resources may be viewed as a malicious request.
* `--console` - Set to `true` to echo log messages to `stdout` as well.  Default `false`.
* `--log-level` - Set the desired log level for the service.  Valid values are one of
`critical`, `warn`, `info` or `debug`.
* `--dir` - Directory under which log files are to be stored.  Log files will be
rotated daily.  Default `logs/` directory under the current working directory.
    * **Note:** if the directory does not exist, no logs will be output.
    * A trailing `/` is mandatory for the specified directory (eg. `/var/logs/proxy/`).
    * External scheduled job (`cron` or similar) that keeps a bound on the number of log
    files is required.
        * If run as a *Docker container* without the log directory being mounted
        as a volume, restarting the container will clear logs.  This will
        delete **all** historical logs.
        * Since we are dealing with a backing *S3 bucket*, the proxy will almost
        certainly be run on AWS infrastructure.  In this case, it is safe to
        restart the container, since the logs should end up in CloudWatch.
            * Ensure the *Docker* daemon has been configured to log to CloudWatch.
            * Ensure the server is started with `--console true`.
* `--mmdb-host` - If integration with [mmdb-ws](https://github.com/sptrakesh/mmdb-ws)
is desired, specify the hostname for the service.
* `--mmdb-port` - Port on which the `mmdb-ws` service is listening.  Default is
`2010`.  Only relevant if `--mmdb-host` is specified.
* `--akumuli-host` - If metrics are to be published to [Akumuli](https://akumuli.org/).
Works in combination with `--mmdb-host`.  Disabled if not specified.
* `--akumuli-port` - Port on which *Akumuli* TCP write service listens.  Default
is `8282`.
* `--mongo-uri` - If metrics are to be published to [MongoDB](https://mongodb.com/).
Should follow [Connection String](https://docs.mongodb.com/manual/reference/connection-string/)
URI format.  Works in combination with `--mmdb-host`.  Disabled if not specified.
* `--mongo-database` - Mongo *database* to write metrics to.  Default is `metrics`.
* `--mongo-collection` - Mongo *collection* to write metrics to.  Default is `request`.

Full list of options can always be consulted by running the following:

```shell script
<path to>/s3proxy --help
# Install path specified for cmake is /opt/spt/bin
/opt/spt/bin/s3proxy --help
```

### Metric
Metric is captured in a simple [struct](src/model/metric.h).  If **MMDB**
integration is enabled, additional information about the *visitors* geo-location
is retrieved and added to the metric.  The combined result is then saved to
**Akumuli** and/or **MongoDB**.

#### BSON Document
The following shows a sample `bson` document as generated by the integration
test suite.

```json
{
  "_id" : ObjectId("5ee296de49d5eb77ef6580ef"),
  "method" : "GET",
  "resource" : "/index.html",
  "mimeType" : "text/html",
  "ipaddress" : "184.105.163.155",
  "size" : NumberLong(2950),
  "time" : NumberLong(640224),
  "status" : 200,
  "compressed" : false,
  "timestamp" : NumberLong("1591908061657005325"),
  "created" : ISODate("2020-06-11T20:41:01.657Z"),
  "location" : {
    "query_ip_address" : "184.105.163.155",
    "continent" : "North America",
    "subdivision" : "Oklahoma",
    "query_language" : "en",
    "city" : "Oklahoma City (Central Oklahoma City)",
    "country" : "United States",
    "longitude" : "-97.597100",
    "country_iso_code" : "US",
    "latitude" : "35.524800"
  }
}
```

#### Akumuli Series
Most metric properties are saved as *tags* in the corresponding *series* stored
in Akumuli.  The following series are stored:
* `<prefix>.counter` - A simple counter series.  Stores individual requests with
a fixed value of `1` to track requests.
* `<prefix>.size` - A series used to track the response sizes sent.  **Note:**
this value will differ based on whether `compressed` response was requested or not.
* `<prefix>.time` - A series used to track the time consumed to send the response.
* `<prefix>.location` - A event series used to track the geo-location of the site
visitor.  This is only as accurate as the *IP Address* information inferred about
the request.

#### Akumuli Tags
Most metric properties are saved as *tags* in the corresponding *series* stored
in Akumuli.  Akumuli tags are specified as `space` separated values.  The official
documentation recommends escaping `space` with a `backslash` character following
standard conventions.  This scheme however has not worked well, especially with
longer city names.  To work around this, we use a special `__#SPACE#__` escape
pattern (extremely unlikely to collide with any other valid text).  Downstream
users of the corresponding Akumuli series must keep this in mind and perform
appropriate pattern replacement.

For instance the [geojson-ds](https://github.com/sptrakesh/grafana-geojson-datasource)
project, replaces the pattern when returning *tag values*, and also performs the
reverse conversion when the *tag value* is specified as an *adhoc filter* in
the *query*.

## Testing
[Integration Test](test/lua/README.md)

Basic performance testing has been performed using Apache Benchmarking utility.

```shell script
ab -H 'x-forwarded-for: 184.105.163.155' -n 10000 -c 100 -r http://127.0.0.1:8000/
ab -H 'x-forwarded-for: 184.105.163.155' -n 10000 -c 200 -r http://127.0.0.1:8000/
ab -H 'x-forwarded-for: 184.105.163.155' -n 10000 -c 300 -r http://127.0.0.1:8000/
ab -H 'x-forwarded-for: 184.105.163.155' -n 10000 -c 400 -r http://127.0.0.1:8000/
ab -H 'x-forwarded-for: 184.105.163.155' -n 100000 -c 100 -r http://127.0.0.1:8000/
```

Also tested using `httperf` for a much bigger run with the focus on ensuring no
errors over an extended run.

```shell script
httperf --client=0/1 --server=localhost --port=8000 --uri=/ --send-buffer=4096 --recv-buffer=16384 --num-conns=20000 --num-calls=200
httperf: warning: open file limit > FD_SETSIZE; limiting max. # of open files to FD_SETSIZE
Maximum connect burst length: 1

Total: connections 20000 requests 4000000 replies 4000000 test-duration 1549.168 s

Connection rate: 12.9 conn/s (77.5 ms/conn, <=1 concurrent connections)
Connection time [ms]: min 62.0 avg 77.5 max 596.9 median 74.5 stddev 14.6
Connection time [ms]: connect 0.2
Connection length [replies/conn]: 200.000

Request rate: 2582.0 req/s (0.4 ms/req)
Request size [B]: 62.0

Reply rate [replies/s]: min 1324.9 avg 2581.7 max 3018.2 stddev 288.2 (309 samples)
Reply time [ms]: response 0.4 transfer 0.0
Reply size [B]: header 247.0 content 2950.0 footer 0.0 (total 3197.0)
Reply status: 1xx=0 2xx=4000000 3xx=0 4xx=0 5xx=0

CPU time [s]: user 311.45 system 1229.94 (user 20.1% system 79.4% total 99.5%)
Net I/O: 8217.6 KB/s (67.3*10^6 bps)

Errors: total 0 client-timo 0 socket-timo 0 connrefused 0 connreset 0
Errors: fd-unavail 0 addrunavail 0 ftab-full 0 other 0
```

## Docker
Docker images for the server will be published on *Docker Hub* as changes are
made to the implementation.  The image can be *pulled* from `sptrakesh/s3-proxy`.

```shell script
# Build docker image locally
<path to project>/docker/build.sh
```

### Run Locally
Following shows a sample for running the docker image from *Docker Hub*

```shell script
mkdir -p proxy/data proxy/logs
docker run -d --rm \
  -v $PWD/proxy/data:/opt/spt/data \
  -v $dir/proxy/logs:/opt/spt/logs \
  -p 8000:8000 \
  -e AWS_REGION="<your region>" \
  -e S3_BUCKET="<your bucket>" \
  -e AWS_KEY="<your key>" \
  -e AWS_SECRET="<your secret>" \
  -e AUTH_KEY="<your desired bearer token>" \
  --name s3-proxy sptrakesh/s3-proxy
```

## Cache Management
Clearing the cache can be achieved via either of the following options:
* Restart the service.  On restart the in-memory metadata is lost, and objects
will be re-fetched from S3.
* Invoke the `/_proxy/_private/_cache/clear` endpoint on the service with the
configured `--auth-key` *Bearer* authorisation token.

```shell script
# For clearing cache on instance running on localhost
curl -i --header "Authorization: Bearer <authKey>" http://127.0.0.1:8000/_proxy/_private/_cache/clear
```

## Acknowledgements
This software has been developed mainly using work other people have contributed.
The following are the components used to build this software:
* **[Boost:Beast](https://github.com/boostorg/beast)** - We use *Beast* for the
`http` server implementation.  The implementation is a modified version of the
[async](https://github.com/boostorg/beast/tree/develop/example/http/server/async)
example.
* **[nghttp2](https://github.com/nghttp2/nghttp2)** - We use *nghttp2* for the
  `HTTP/2` server implementation.
* **[AWS SDK](https://github.com/aws/aws-sdk-cpp)** - We use the **S3** module
to access the underlying *S3 bucket*.
* **[Clara](https://github.com/catchorg/Clara)** - Command line options parser.
* **[NanoLog](https://github.com/Iyengar111/NanoLog)** - Logging framework used
for the server.  I modified the implementation for daily rolling log files.
* **[expirationCache](https://github.com/zapredelom/expirationCache)** - Cache
manager for S3 object metadata.
* **[concurrentqueue](https://github.com/cameron314/concurrentqueue)** - Lock
free concurrent queue implementation for metrics.
