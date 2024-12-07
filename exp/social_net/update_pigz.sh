#!/bin/bash

# install the latest pigz if it has any problem on your machine

INSTALL_PATH="/usr/local/bin"

cd /tmp
git clone https://github.com/madler/pigz.git
cd pigz
make -j$(nproc)
sudo cp pigz $INSTALL_PATH
sudo cp unpigz $INSTALL_PATH
sudo ldconfig
