#!/bin/bash

set -e

CGROUP_USER=${SUDO_USER:-$USER}

CPU_CGROUP=cpu:wukong
echo "cgcreate -t ${CGROUP_USER}:${CGROUP_USER} -a ${CGROUP_USER}:${CGROUP_USER} -g ${CPU_CGROUP}"
cgcreate -t ${CGROUP_USER}:${CGROUP_USER} -a ${CGROUP_USER}:${CGROUP_USER} -g ${CPU_CGROUP}

CPU_CGROUP=memory:wukong
echo "cgcreate -t ${CGROUP_USER}:${CGROUP_USER} -a ${CGROUP_USER}:${CGROUP_USER} -g ${CPU_CGROUP}"
cgcreate -t ${CGROUP_USER}:${CGROUP_USER} -a ${CGROUP_USER}:${CGROUP_USER} -g ${CPU_CGROUP}