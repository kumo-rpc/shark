#! /bin/sh
# Copyright (c) 2013-2014 Sandstorm Development Group, Inc. and contributors
# Licensed under the MIT License:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Tests the `shark` tool's various commands, other than `compile`.

set -eu

fail() {
  echo "FAILED: $@" >&2
  exit 1
}

if test -n "${SHARK:-}"; then
  :
elif test -f ./shark; then
  SHARK=./shark
elif test -f ./shark.exe; then
  SHARK=./shark.exe
else
  SHARK=shark
fi

SCHEMA=`dirname "$0"`/../test.shk
JSON_SCHEMA=`dirname "$0"`/../compat/json-test.shk
TESTDATA=`dirname "$0"`/../testdata
SRCDIR=`dirname "$0"`/../..
IMPORT=-I"$SRCDIR"

PREFIX=$TESTDATA/

# ========================================================================================
# convert

$SHARK convert text:binary $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/binary - || fail encode
$SHARK convert text:flat $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/flat - || fail encode flat
$SHARK convert text:packed $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/packed - || fail encode packed
$SHARK convert text:flat-packed $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/packedflat - || fail encode packedflat
$SHARK convert text:binary $SCHEMA TestAllTypes $IMPORT < $TESTDATA/pretty.txt | cmp $TESTDATA/binary - || fail parse pretty

$SHARK convert binary:text $SCHEMA TestAllTypes $IMPORT < $TESTDATA/binary | cmp $TESTDATA/pretty.txt - || fail decode
$SHARK convert flat:text $SCHEMA TestAllTypes $IMPORT < $TESTDATA/flat | cmp $TESTDATA/pretty.txt - || fail decode flat
$SHARK convert packed:text $SCHEMA TestAllTypes $IMPORT < $TESTDATA/packed | cmp $TESTDATA/pretty.txt - || fail decode packed
$SHARK convert flat-packed:text $SCHEMA TestAllTypes $IMPORT < $TESTDATA/packedflat | cmp $TESTDATA/pretty.txt - || fail decode packedflat
$SHARK convert binary:text --short $SCHEMA TestAllTypes $IMPORT < $TESTDATA/binary | cmp $TESTDATA/short.txt - || fail decode short

$SHARK convert binary:text $SCHEMA TestAllTypes $IMPORT < $TESTDATA/segmented | cmp $TESTDATA/pretty.txt - || fail decode segmented
$SHARK convert packed:text $SCHEMA TestAllTypes $IMPORT < $TESTDATA/segmented-packed | cmp $TESTDATA/pretty.txt - || fail decode segmented-packed

$SHARK convert binary:packed < $TESTDATA/binary | cmp $TESTDATA/packed - || fail binary to packed
$SHARK convert packed:binary < $TESTDATA/packed | cmp $TESTDATA/binary - || fail packed to binary

$SHARK convert binary:json $SCHEMA TestAllTypes $IMPORT < $TESTDATA/binary | cmp $TESTDATA/pretty.json - || fail binary to json
$SHARK convert binary:json --short $SCHEMA TestAllTypes $IMPORT < $TESTDATA/binary | cmp $TESTDATA/short.json - || fail binary to short json

$SHARK convert json:binary $SCHEMA TestAllTypes $IMPORT < $TESTDATA/pretty.json | cmp $TESTDATA/binary - || fail json to binary
$SHARK convert json:binary $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.json | cmp $TESTDATA/binary - || fail short json to binary

$SHARK convert json:binary $JSON_SCHEMA TestJsonAnnotations -I"$SRCDIR" < $TESTDATA/annotated.json | cmp $TESTDATA/annotated-json.binary - || fail annotated json to binary
$SHARK convert binary:json $JSON_SCHEMA TestJsonAnnotations -I"$SRCDIR" < $TESTDATA/annotated-json.binary | cmp $TESTDATA/annotated.json - || fail annotated binary to json

[ "$(echo '(foo = (text = "abc"))' | $SHARK convert text:text "$SRCDIR/tests/test.shk" BrandedAlias $IMPORT)" = '(foo = (text = "abc"), uv = void)' ]  || fail branded alias
[ "$(echo '(foo = (text = "abc"))' | $SHARK convert text:text "$SRCDIR/tests/test.shk" BrandedAlias.Inner $IMPORT)" = '(foo = (text = "abc"))' ]  || fail branded alias
[ "$(echo '(foo = (text = "abc"))' | $SHARK convert text:text "$SRCDIR/tests/test.shk" 'TestGenerics(BoxedText, Text)' $IMPORT)" = '(foo = (text = "abc"), uv = void)' ]  || fail branded alias
[ "$(echo '(baz = (text = "abc"))' | $SHARK convert text:text "$SRCDIR/tests/test.shk" 'TestGenerics(TestAllTypes, List(Int32)).Inner2(BoxedText)' $IMPORT)" = '(baz = (text = "abc"))' ]  || fail branded alias

# ========================================================================================
# DEPRECATED encode/decode

$SHARK encode $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/binary - || fail encode
$SHARK encode --flat $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/flat - || fail encode flat
$SHARK encode --packed $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/packed - || fail encode packed
$SHARK encode --packed --flat $SCHEMA TestAllTypes $IMPORT < $TESTDATA/short.txt | cmp $TESTDATA/packedflat - || fail encode packedflat
$SHARK encode $SCHEMA TestAllTypes $IMPORT < $TESTDATA/pretty.txt | cmp $TESTDATA/binary - || fail parse pretty

$SHARK decode $SCHEMA TestAllTypes $IMPORT < $TESTDATA/binary | cmp $TESTDATA/pretty.txt - || fail decode
$SHARK decode --flat $SCHEMA TestAllTypes $IMPORT < $TESTDATA/flat | cmp $TESTDATA/pretty.txt - || fail decode flat
$SHARK decode --packed $SCHEMA TestAllTypes $IMPORT < $TESTDATA/packed | cmp $TESTDATA/pretty.txt - || fail decode packed
$SHARK decode --packed --flat $SCHEMA TestAllTypes $IMPORT < $TESTDATA/packedflat | cmp $TESTDATA/pretty.txt - || fail decode packedflat
$SHARK decode --short $SCHEMA TestAllTypes $IMPORT < $TESTDATA/binary | cmp $TESTDATA/short.txt - || fail decode short

$SHARK decode $SCHEMA TestAllTypes $IMPORT < $TESTDATA/segmented | cmp $TESTDATA/pretty.txt - || fail decode segmented
$SHARK decode --packed $SCHEMA TestAllTypes $IMPORT < $TESTDATA/segmented-packed | cmp $TESTDATA/pretty.txt - || fail decode segmented-packed

# ========================================================================================
# eval

test_eval() {
  test "x`$SHARK eval $SCHEMA $1 $IMPORT | tr -d '\r'`" = "x$2" || fail eval "$1 == $2"
}

test_eval TestDefaults.uInt32Field 3456789012
test_eval TestDefaults.structField.textField '"baz"'
test_eval TestDefaults.int8List "[111, -111]"
test_eval 'TestDefaults.structList[1].textField' '"structlist 2"'
test_eval globalPrintableStruct '(someText = "foo")'
test_eval TestConstants.enumConst corge
test_eval 'TestListDefaults.lists.int32ListList[2][0]' 12341234

test "x`$SHARK eval $SCHEMA -ojson globalPrintableStruct $IMPORT | tr -d '\r'`" = "x{\"someText\": \"foo\"}" || fail eval json "globalPrintableStruct == {someText = \"foo\"}"

$SHARK eval $TESTDATA/no-file-id.sk.nobuild foo >/dev/null || fail eval "file without file ID can be parsed"
test "x`$SHARK eval $TESTDATA/no-file-id.sk.nobuild foo | tr -d '\r'`" = 'x"bar"' || fail eval "file without file ID parsed correctly"

$SHARK compile --no-standard-import --src-prefix="$PREFIX" -ofoo $TESTDATA/errors.sk.nobuild 2>&1 | sed -e "s,^.*errors[.]sk[.]nobuild:,file:,g" | tr -d '\r' |
    diff -u $TESTDATA/errors.txt - || fail error output

$SHARK compile --no-standard-import --src-prefix="$PREFIX" -ofoo $TESTDATA/errors2.sk.nobuild 2>&1 | sed -e "s,^.*errors2[.]sk[.]nobuild:,file:,g" | tr -d '\r' |
    diff -u $TESTDATA/errors2.txt - || fail error2 output
