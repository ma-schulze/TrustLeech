#!/bin/bash 

if [ ! -f ./venv/bin/activate.sh ]; then
  python3 -m venv ./venv
  source ./venv/bin/activate
  pip3 install numpy
else 
  source ./venv/bin/activate
fi
python3 memdump.py
