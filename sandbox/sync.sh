#!/bin/bash
sudo mount raw.img mnt/
sudo cp -r data/* mnt/
sync
sudo umount mnt/