#!/bin/bash

set -e

UNAME=$(uname)

if [[ ! -f ./CPPLINT.cfg ]]; then
    if [[ -f ../CPPLINT.cfg ]]; then
        cd ..
    else
        echo "This script must be run in the Viccyware repo. ./wire/build.sh"
        exit 1
    fi
fi

VICDIR="$(pwd)"

cd ~
if [[ ! -d .anki ]]; then
    echo "Downloading ~/.anki folder contents..."
    git clone https://github.com/kercre123/anki-deps
    mv anki-deps .anki
fi

if [[ ${UNAME} == "Darwin" ]]; then
    echo "Checking out macOS branch..."
    cd .anki
    git checkout macos
    git lfs install
    git lfs pull
    echo "Disabling security..."
    sudo spctl --master-disable
    sudo spctl --global-disable
fi

cd "${VICDIR}"

git lfs update --force

echo "Building Viccyware..."

echo "Thank you Wire <3"

./project/victor/scripts/victor_build_release.sh

echo "Copying vic-cloud and vic-gateway..."
cp -a bin/* _build/vicos/Release/bin/

echo "Copying sb_server binary..."
cp -a 3rd/snowboy/vicos/bin/sb_server _build/vicos/Release/bin/
chmod +rwx _build/vicos/Release/bin/sb_server

echo

echo "Thank you Wire <3"

echo "Build was successful!"

if [[ ${UNAME} == "Darwin" ]]; then
    echo "Deploy on bot using ./wire/deploy.sh after echoing your robot ip to "robot_ip.txt" and adding your ssh key as "robot_sshkey""
else
    echo "Deploy on bot using ./wire/deploy-d.sh after echoing your robot ip to "robot_ip.txt" and adding your ssh key as "robot_sshkey""
fi
