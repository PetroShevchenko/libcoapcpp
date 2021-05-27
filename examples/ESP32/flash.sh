#!/bin/bash

EXAMPLE=coap-client
PORT=/dev/ttyUSB0

cd $EXAMPLE && idf.py -p $PORT flash && idf.py monitor
