#!/bin/bash
set -e

function integration_tests() {
    mmm -j external/nos/test/system-test-harness || return 1
    adb push \
        out/target/product/blueline/vendor/bin/hw/citadel_integration_tests \
        /data/local/tmp || return 1
    adb exec-out \
        '/data/local/tmp/citadel_integration_tests --release-tests' || return 1
}

# TODO: add AVB / Weaver / Keymaster VTS / CTS tests with filters here.
function avb_cts_tests() {
    return 0
}

function avb_vts_tests() {
    return 0
}

function keymaster_cts_tests() {
    return 0
}

function keymaster_vts_tests() {
    return 0
}

function weaver_cts_tests() {
    return 0
}

function weaver_vts_tests() {
    return 0
}

# TODO: add any other tests

source "${PWD}"/build/envsetup.sh
lunch blueline-userdebug
adb root

for t in integration_tests \
	     avb_cts_tests \
	     avb_vts_tests \
	     keymaster_cts_tests \
	     keymaster_vts_tests \
	     weaver_cts_tests \
	     weaver_vts_tests ; do
    if eval "${t}"; then
	echo "PASS: ${t}"
    else
	echo "FAIL: ${t}"
	exit 1
    fi
done
