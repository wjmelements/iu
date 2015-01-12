#!/bin/bash
SRV=bin/iuserver
CLT=bin/iuclient
RG=2
NIDS=()
function spawnServer {
    mkdir $1
    cd $1
    (../$SRV $1 $RG &)
    cd ..
    NIDS+=($1)
}
function iushutdown {
    pkill iu
    rm -r ${NIDS[@]}
}
spawnServer 1
spawnServer 2
spawnServer 3
iushutdown
