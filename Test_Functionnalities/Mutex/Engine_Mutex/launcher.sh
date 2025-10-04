#!/bin/bash

# Exit immediately if any command fails
set -e

echo "Building server..."
make

echo "Launching Python stress test..."
python3 mutex_concurrency_test.py

echo "Cleaning up build files..."
make realclean

echo "All done!"