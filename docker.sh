#!/bin/bash
set -o nounset;  # Abort on unbound variable.
set -o pipefail; # Don't hide errors within pipes.
set -o errexit;  # Abort on non-zero exit status.

docker build --progress=plain . -t tomasz-lisowski/swicc-pcsc:1.0.0 2>&1 | tee docker.log;
docker run --user $(id -u):$(id -g) -v .:/opt/swicc-pcsc --tty --interactive --rm tomasz-lisowski/swicc-pcsc:1.0.0;
