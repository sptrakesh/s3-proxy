FROM sptrakesh/aws-sdk-s3 as aws
FROM sptrakesh/config-db:alpine as config
FROM sptrakesh/encrypter as encrypter
FROM sptrakesh/mongocxx as mongo
FROM sptrakesh/nghttp2 as base

COPY --from=aws /opt/local /opt/local
COPY --from=mongo /opt/local /opt/local
COPY CMakeLists.txt /opt/spt/s3-proxy/CMakeLists.txt
COPY src /opt/spt/s3-proxy/src

WORKDIR /opt/spt/s3-proxy/build
RUN apk add --no-cache curl-dev snappy-dev openssl-libs-static \
  && rm -rf * \
  && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=mold" -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=mold" .. \
  && make -j8 \
  && make install

FROM sptrakesh/cppruntime

RUN apk add --no-cache libcurl snappy \
  && addgroup spt \
  && adduser -DS -g "SPT User" -G spt -s /bin/sh -h /home/spt spt \
  && mkdir -p /opt/spt/logs /opt/spt/data \
  && chown spt:spt /opt/spt/logs /opt/spt/data

COPY --from=config /opt/spt/bin /opt/spt/bin
COPY --from=config /opt/spt/certs/ca.crt /opt/spt/certs/ca.crt
COPY --from=config /opt/spt/certs/client.crt /opt/spt/certs/client.crt
COPY --from=config /opt/spt/certs/client.key /opt/spt/certs/client.key
COPY --from=encrypter /opt/spt/bin /opt/spt/bin
COPY --from=base /opt/spt/bin /opt/spt/bin
COPY docker/scripts/*.sh /opt/spt/bin/

WORKDIR /opt/spt
ENV LD_LIBRARY_PATH=/usr/lib
USER spt
ENTRYPOINT [ "/opt/spt/bin/entrypoint.sh" ]