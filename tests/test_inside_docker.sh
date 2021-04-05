#!/bin/sh -xe

OS_VERSION=$1
BUILD_ENV=$2

# Source repo version
git clone https://github.com/brianhlin/osg-test.git
pushd osg-test
git checkout separate-user-cert-generation
git rev-parse HEAD
make install
popd
git clone https://github.com/opensciencegrid/osg-ca-generator.git
pushd osg-ca-generator
git rev-parse HEAD
make install
popd

# Bind on the right interface and skip hostname checks.
cat << EOF > /etc/condor/config.d/99-local.conf
BIND_ALL_INTERFACES = true
GSI_SKIP_HOST_CHECK=true
ALL_DEBUG=\$(ALL_DEBUG) D_FULLDEBUG D_CAT
SCHEDD_INTERVAL=1
SCHEDD_MIN_INTERVAL=1
JOB_ROUTER_POLLING_PERIOD=1
GRIDMANAGER_JOB_PROBE_INTERVAL=1
EOF
cp /etc/condor/config.d/99-local.conf /etc/condor-ce/config.d/99-local.conf

# Reduce the trace timeouts
export _condor_CONDOR_CE_TRACE_ATTEMPTS=60

# Ok, do actual testing
set +e # don't exit immediately if osg-test fails
echo "------------ OSG Test --------------"
osg-test -mvad --hostcert --no-cleanup
test_exit=$?
set -e

exit $test_exit