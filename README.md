# s3-proxy
A simple proxy server for serving contents of an AWS S3 bucket as a static website.
Useful in cases where it is not feasible to use traditional CloudFront distribution
or similar CDN on top of a bucket.

In particular it is possible to serve static content from a private S3 bucket.