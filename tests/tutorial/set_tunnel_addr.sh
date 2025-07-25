#!/bin/sh
##############################################################################
# 5G-MAG Reference Tools: MBS Traffic Function: Helper script to update JSON
##############################################################################
# Copyright: (C)2025 British Broadcasting Corporation
# Author(s): David Waring <david.waring2@bbc.co.uk>
# License: 5G-MAG Public License v1
#
# Licensed under the License terms and conditions for use, reproduction, and
# distribution of 5G-MAG software (the “License”).
#
# You may not use this file except in compliance with the License. You may
# obtain a copy of the License at https://www.5g-mag.com/reference-tools.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an “AS IS” BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the License for the specific language governing permissions and
# limitations under the License.
##############################################################################

scriptdir=`dirname "$0"`
scriptdir=`realpath "$scriptdir"`
scriptname=`basename "$0"`

addr="$1"
port="$2"

syntax() {
    echo "Syntax: $scriptname <tunnel-address> <tunnel-port>"
}

if [ -z "$addr" -o -z "$port" ]; then
    syntax >&2
    exit 1
fi

if ! echo "$addr" | grep -qE '^([0-9A-Fa-f:]*|[0-9.]*|[a-zA-Z][-0-9a-zA-Z.]*)$'; then
    echo '<tunnel-address> must be either IPv6, IPv4 or a hostname' >&2
    exit 1
fi

if ! echo "$port" | grep -qE '^[1-9][0-9]{0,4}$'; then
    echo '<tunnel-port> must be an unsigned integer between 1 and 65535' >&2
    exit 1
elif [ "$port" -lt 1 -o "$port" -gt 65535 ]; then
    echo '<tunnel-port> must be an unsigned integer between 1 and 65535' >&2
    exit 1
fi

for i in DistSession-PULL-request DistSession-PUSH-request DistSession-DASH-PULL-request DistSession-DASH-PUSH-request; do
    jq '{distSession: (.distSession + { "mbUpfTunAddr": { "ipv4Addr": "'"$addr"'", "portNumber": '"$port"' } })}' "${scriptdir}/$i.json" > "${scriptdir}/$i.mbsmf.json"
    echo "Created $i.mbsmf.json with new tunnel address and port"
done
