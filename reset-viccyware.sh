#!/bin/bash
git reset --hard
git submodule sync --recursive
git submodule update --init --force --recursive
git clean -ffdx
git submodule foreach --recursive git clean -ffdx
cd EXTERNALS
git checkout viccyware
git reset --hard
git clean -ffdx
cd ..

