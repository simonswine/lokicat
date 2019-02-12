#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

url=$1
hash=$2
dest=$3

# download from url
tmpfile=$(mktemp /tmp/lokicat-hack.XXXXXX)
curl -Lo "${tmpfile}" "${url}"

cleanup() {
    rm -f "${tmpfile}"
}
trap "cleanup" EXIT SIGINT

# check sum
echo "${hash}  ${tmpfile}" | sha256sum -c

mkdir -p "${dest}"
tar xf "${tmpfile}" --strip-components 1 -C "${dest}"

# link download folder accross
mkdir -p "${dest}/../openwrt-shared/dl"
rm -rf "${dest}/dl"
ln -s "../openwrt-shared/dl" "${dest}/dl"
