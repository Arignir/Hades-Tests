#!/usr/bin/env bash

################################################################################
##
##  This file is part of the Hades GBA Emulator, and is made available under
##  the terms of the GNU General Public License version 2.
##
##  Copyright (C) 2021-2022 - The Hades Authors
##
################################################################################

sudo docker run --platform linux/amd64 -it --workdir=/app -v $(pwd):/app devkitpro/devkitarm make "$@" re -j