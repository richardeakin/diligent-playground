#! /usr/bin/env bash

CURRENT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cmake -S . -B ./build -G "Visual Studio 17 2022"
