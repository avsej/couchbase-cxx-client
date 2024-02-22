#!/usr/bin/env bash

#  Copyright 2020-2021 Couchbase, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

PROJECT_ROOT="$( cd "$(dirname "$0")/.." >/dev/null 2>&1 ; pwd -P )"

echo "HOSTNAME=${HOSTNAME}"
echo "NODE_NAME=${NODE_NAME}"
echo "CONTAINER_TAG=${CONTAINER_TAG}"
echo "JENKINS_SLAVE_LABELS=${JENKINS_SLAVE_LABELS}"
echo "NODE_LABELS=${NODE_LABELS}"
echo "TEST_DEPLOYMENT_TYPE=${TEST_DEPLOYMENT_TYPE}"

set -u

[ -f /proc/sys/kernel/core_pattern ] && echo -e "/proc/sys/kernel/core_pattern:\n$(cat /proc/sys/kernel/core_pattern)"
if [ -e /usr/bin/apport-unpack ]
then
    mkdir -p $HOME/.config/apport
    cat <<EOF >$HOME/.config/apport/settings
[main]
unpackaged=true
EOF
fi

ulimit -c unlimited
rm -f /tmp/core.* "${PWD}/core*" /var/crash/*

mkdir -p logs

for TEST in $(find ./test/ -type f -name 'test_*' | grep -v transaction)
do
  chmod a+x $TEST
  echo -e "\n$(date -u) ${TEST}" | tee "${TEST}.stderr.log"
  ls -lh $TEST >> "${TEST}.stderr.log"
  cbdinocluster ps 2>/dev/null >> "${TEST}.stderr.log"
  cbdinocluster get-def $CLUSTER_ID 2>/dev/null >> "${TEST}.stderr.log"
  echo "$TEST --durations=yes --reporter=JUnit::out='${TEST}.report.xml' --reporter console" >> "${TEST}.stderr.log"
  /usr/bin/time bash -c "$TEST --durations=yes --reporter=JUnit::out='${TEST}.report.xml' --reporter console >> '${TEST}.stderr.log'  2>&1"

  STATUS=$?

  if [ "x${STATUS}" != "x0" ]
  then
      if [ -e /usr/bin/coredumpctl ]
      then
          /usr/bin/coredumpctl list --no-pager
          /usr/bin/coredumpctl -1 info
      elif [ -e /usr/bin/apport-unpack ]
      then
          for CRASH in /var/crash/*
          do
              if [ -f $CRASH ]
              then
                  echo $CRASH
                  /usr/bin/apport-unpack $CRASH /tmp/the_crash/
                  EXECUTABLE=$(cat /tmp/the_crash/ExecutablePath)
                  if [ ! -f $EXECUTABLE ]
                  then
                      EXECUTABLE=$(realpath $TEST)
                  fi
                  gdb $EXECUTABLE /tmp/the_crash/CoreDump --batch -ex "thread apply all bt"
                  rm -rf $CRASH /tmp/the_crash
              fi
          done
      fi
      for CORE in /tmp/core.* "${PWD}/core*"
      do
          if [ -f $CORE ]
          then
              echo $CORE
              EXECUTABLE=$(file $CORE | ruby -e "print ARGF.read[/execfn: '([^']+)'/, 1]")
              echo $EXECUTABLE
              if [ ! -f $EXECUTABLE ]
              then
                  EXECUTABLE=$(realpath $TEST)
              fi
              gdb $EXECUTABLE $CORE --batch -ex "thread apply all bt"
              rm -f $CORE
          fi
      done
      exit $STATUS
  fi
done
