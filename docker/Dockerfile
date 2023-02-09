FROM sptrakesh/aws-sdk-s3:gcc as aws
FROM sptrakesh/config-db as config
FROM sptrakesh/encrypter:gcc as encrypter
FROM sptrakesh/mongocxx:gcc as mongo
FROM sptrakesh/nghttp2:gcc as base

COPY --from=aws /opt/local /opt/local
COPY --from=mongo /opt/local /opt/local
COPY CMakeLists.txt /opt/spt/s3-proxy/CMakeLists.txt
COPY src /opt/spt/s3-proxy/src

WORKDIR /opt/spt/s3-proxy/build
RUN  apt-get install -y libcurl4 libsnappy1v5 \
  && rm -rf * \
  && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/local .. \
  && make -j8 \
  && make install

FROM sptrakesh/cppruntime:gcc

RUN  apt-get install -y libcurl4 libsnappy1v5 \
  && adduser --shell /bin/sh --home /home/spt spt \
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