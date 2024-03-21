#!/usr/bin/env bash

################################################################################
##
##  This file is part of the Hades GBA Emulator, and is made available under
##  the terms of the GNU General Public License version 2.
##
##  Copyright (C) 2021-2024 - The Hades Authors
##
################################################################################

rm -rf roms

sudo docker run --platform linux/amd64 -it --workdir=/app -v $(pwd):/app --user $UID:$UID devkitpro/devkitarm make "$@" -j
