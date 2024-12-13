#!/bin/bash
dd if=/dev/zero of=raw.img bs=1M count=256
mkfs.fat raw.img -F 32
echo "Created RAW image file: raw.img"