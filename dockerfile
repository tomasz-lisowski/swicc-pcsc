FROM ubuntu:22.04 AS base

RUN set -eux; \
    apt-get -qq update; \
    apt-get -qq --yes dist-upgrade; \
    apt-get -qq --yes --no-install-recommends install cmake gcc gcc-multilib pkg-config make libpcsclite-dev libpcsclite1;

ENTRYPOINT [ "bash", "-c", "cd /opt/swicc-pcsc && make clean && make -j $(nproc) main" ]
