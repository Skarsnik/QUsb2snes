#!/usr/bin/bash

VERSION=0.7.26.1
PROJECT=QUsb2snes

# required to build on centos, not on fedora
sudo yum install epel-release -y

sudo yum install git fedpkg -y

rm -rf ${PROJECT}
git clone https://github.com/Skarsnik/${PROJECT}.git --branch v${VERSION}

cd ${PROJECT}
git submodule update --init

cd ..
tar czf v${VERSION}.tar.gz ${PROJECT}

fedpkg --release f37 mockbuild

rm -rf ${PROJECT} v${VERSION}.tar.gz
