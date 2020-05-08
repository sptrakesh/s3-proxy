# s3-proxy
A simple proxy server for serving contents of an AWS S3 bucket as a static website.
Useful in cases where it is not feasible to use traditional CloudFront distribution
or similar CDN on top of a bucket.

In particular it is possible to serve static content from a private S3 bucket.

## Configuration
The server can be configured with the following options:
* `--auth-key` - A `bearer token` value to use to access internal management
API endpoints.
    * At present there is only the `/_proxy/_private/_cache/clear` endpoint to 
    clear the cache.
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
* `--console` - Set to `true` to echo log messages to `stdout` as well.  Default `false`.
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

Full list of options can always be consulted by running the following:

```shell script
<path to>/s3proxy --help
# Install path specified for cmake is /opt/spt/bin
/opt/spt/bin/s3proxy --help
```

## Testing
[Integration Test](test/lua/README.md)

Basic performance testing has been performed using Apache Benchmarking utility.

```shell script
ab -n 10000 -c 100 -r http://127.0.0.1:8000/
ab -n 10000 -c 200 -r http://127.0.0.1:8000/
ab -n 10000 -c 300 -r http://127.0.0.1:8000/
ab -n 10000 -c 400 -r http://127.0.0.1:8000/
ab -n 100000 -c 100 -r http://127.0.0.1:8000/
```

## Docker
Docker images for the server will be published on *Docker Hub* as changes are
made to the implementation.  The images can be `pulled` from `sptrakesh/s3-proxy`.

## Clearing Cache
Clearing the cache can be achieved via either of the following options:
* Restart the service.  On restart the in-memory metadata is lost, and objects
will be re-fetched from S3.
* Invoke the `/_proxy/_private/_cache/clear` endpoint on the service with the
configured `--auth-key` *Bearer* authorisation token.

## Acknowledgements
This software has been developed mainly using work other people have contributed.
The following are the components used to build this software:
* **[Boost:Beast](https://github.com/boostorg/beast)** - We use *Beast* for the
`http` server implementation.  The implementation is a modified version of the
[stackless](https://github.com/boostorg/beast/tree/develop/example/http/server/stackless)
example.
    * **Note:** I initially started with the [fast](https://github.com/boostorg/beast/tree/develop/example/http/server/fast)
    implementation.  That implementation seems to have some kind of race/locking
    issue which was very repeatable (service has been tested with `ab`).  Every
    16000 or so requests, the server would become unresponsive until the built
    in `30s` watch expired and requests were rejected.
* **[AWS SDK](https://github.com/aws/aws-sdk-cpp)** - We use the **S3** module
to access the underlying *S3 bucket*.
* **[Clara](https://github.com/catchorg/Clara)** - Command line options parser.
* **[NanoLog](https://github.com/Iyengar111/NanoLog)** - Logging framework used
for the server.  I modified the implementation for daily rolling log files.
* **[expirationCache](https://github.com/zapredelom/expirationCache)** - Cache
manager for S3 object metadata.