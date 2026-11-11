#! /usr/bin/env bash
#
# Quick script that compiles and runs the samples, then cleans up.
# Used for release testing.

set -exuo pipefail

capnpc -oc++ addressbook.shk
c++ -std=c++14 -Wall addressbook.c++ addressbook.shk.c++ \
    $(pkg-config --cflags --libs capnp) -o addressbook
./addressbook write | ./addressbook read
./addressbook dwrite | ./addressbook dread
rm addressbook addressbook.shk.c++ addressbook.sk.h

capnpc -oc++ calculator.shk
c++ -std=c++14 -Wall calculator-client.c++ calculator.shk.c++ \
    $(pkg-config --cflags --libs capnp-rpc) -o calculator-client
c++ -std=c++14 -Wall calculator-server.c++ calculator.shk.c++ \
    $(pkg-config --cflags --libs capnp-rpc) -o calculator-server
rm -f /tmp/capnp-calculator-example-$$
./calculator-server unix:/tmp/capnp-calculator-example-$$ &
sleep 0.1
./calculator-client unix:/tmp/capnp-calculator-example-$$
kill %+
wait %+ || true
rm calculator-client calculator-server calculator.shk.c++ calculator.sk.h /tmp/capnp-calculator-example-$$
