#! /usr/bin/env bash
#
# Quick script that compiles and runs the samples, then cleans up.
# Used for release testing.

set -exuo pipefail

shark -ocpp addressbook.shk
c++ -std=c++14 -Wall addressbook.c++ addressbook.shk.c++ \
    $(pkg-config --cflags --libs shark) -o addressbook
./addressbook write | ./addressbook read
./addressbook dwrite | ./addressbook dread
rm addressbook addressbook.shk.c++ addressbook.sk.h

shark -ocpp calculator.shk
c++ -std=c++14 -Wall calculator-client.c++ calculator.shk.c++ \
    $(pkg-config --cflags --libs shark-rpc) -o calculator-client
c++ -std=c++14 -Wall calculator-server.c++ calculator.shk.c++ \
    $(pkg-config --cflags --libs shark-rpc) -o calculator-server
rm -f /tmp/shark-calculator-example-$$
./calculator-server unix:/tmp/shark-calculator-example-$$ &
sleep 0.1
./calculator-client unix:/tmp/shark-calculator-example-$$
kill %+
wait %+ || true
rm calculator-client calculator-server calculator.shk.c++ calculator.sk.h /tmp/shark-calculator-example-$$
