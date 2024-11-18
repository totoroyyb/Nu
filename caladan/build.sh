#!/bin/bash

sudo apt-get update

# Try to install python-dev, check if it exists
if apt-cache policy python-dev | grep -q 'Candidate: (none)'; then
    echo "python-dev not available, attempting to install python-dev-is-python3..."
    if apt-cache policy python-dev-is-python3 | grep -q 'Candidate: (none)'; then
        echo "Neither python-dev nor python-dev-is-python3 could be installed."
        exit 1
    else
        sudo apt-get install -y python-dev-is-python3
    fi
else
    sudo apt-get install -y python-dev
fi

sudo apt-get install -y make gcc cmake pkg-config libnl-3-dev libnl-route-3-dev  \
                        libnuma-dev uuid-dev libssl-dev libaio-dev libcunit1-dev \
                        libclang-dev libncurses-dev meson python3-pyelftools libboost-all-dev

make submodules -j`nproc`
make clean && make -j`nproc`
pushd ksched
make clean && make -j`nproc`
popd
pushd bindings/cc/
make -j`nproc`
popd
sudo ./scripts/setup_machine.sh
