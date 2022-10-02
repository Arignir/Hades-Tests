#!/usr/bin/env bash

sudo docker run --platform linux/amd64 -it --workdir=/app -v $(pwd):/app devkitpro/devkitarm make "$@"