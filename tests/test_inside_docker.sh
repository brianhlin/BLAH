#!/bin/sh -xe

OS_VERSION=$1
SPEC="blahp/config/blahp.spec"

ls -l /home

# Clean the yum cache
yum -y clean all
yum -y clean expire-cache

# First, install all the needed packages.
rpm -Uvh https://dl.fedoraproject.org/pub/epel/epel-release-latest-${OS_VERSION}.noarch.rpm

yum -y install yum-plugin-priorities yum-utils
rpm -Uvh https://repo.grid.iu.edu/osg/3.3/osg-3.3-el${OS_VERSION}-release-latest.rpm
yum -y install rpm-build git gcc gcc-c++ make

# Prepare the RPM environment
mkdir -p /tmp/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cat >> /etc/rpm/macros.dist << EOF
%dist .osg.el${OS_VERSION}
%osg 1
EOF

yum-builddep -y $SPEC
cp $SPEC /tmp/rpmbuild/SPECS
package_version=`awk '/Version/ {print $2}' $SPEC`
pushd blahp
git archive --format=tar HEAD  | gzip >/tmp/rpmbuild/SOURCES/blahp-${package_version}.tar.gz
popd

# Build the RPM
rpmbuild --define '_topdir /tmp/rpmbuild' -ba /tmp/rpmbuild/SPECS/blahp.spec

# After building the RPM, try to install it
# Fix the lock file error on EL7.  /var/lock is a symlink to /var/run/lock
# mkdir -p /var/run/lock # we don't need this no more, right?

ls -lR /tmp/rpmbuild

# Install batch systems that will exercise the blahp in osg-test
yum install -y osg-ce-condor
yum install -y slurm slurm-munge slurm-perlapi slurm-plugins slurm-sql slurm-slurmdbd mariadb-server mysql-server --enablerepo=osg-contrib
yum install -y torque-server torque-mom torque-client torque-scheduler

# Source repo version
git clone https://github.com/opensciencegrid/osg-test.git
pushd osg-test
git rev-parse HEAD
make install
popd
git clone https://github.com/opensciencegrid/osg-ca-generator.git
pushd osg-ca-generator
git rev-parse HEAD
make install
popd

# HTCondor really, really wants a domain name.  Fake one.
sed /etc/hosts -e "s/`hostname`/`hostname`.unl.edu `hostname`/" > /etc/hosts.new
/bin/cp -f /etc/hosts.new /etc/hosts

# Bind on the right interface and skip hostname checks.
cat << EOF > /etc/condor/config.d/99-local.conf
NETWORK_INTERFACE=eth0
GSI_SKIP_HOST_CHECK=true
SCHEDD_DEBUG=\$(SCHEDD_DEBUG) D_FULLDEBUG
SCHEDD_INTERVAL=1
SCHEDD_MIN_INTERVAL=1
JOB_ROUTER_POLLING_PERIOD=1
GRIDMANAGER_JOB_PROBE_INTERVAL=1
EOF
cp /etc/condor/config.d/99-local.conf /etc/condor-ce/config.d/99-local.conf

# Reduce the trace timeouts
export _condor_CONDOR_CE_TRACE_ATTEMPTS=120

# Enable PBS/Slurm BLAH debugging
mkdir /var/tmp/{qstat,slurm}_cache_vdttest/
touch /var/tmp/qstat_cache_vdttest/pbs_status.debug
touch /var/tmp/slurm_cache_vdttest/slurm_status.debug

# Ok, do actual testing
set +e # don't exit immediately if osg-test fails
echo "------------ OSG Test --------------"
osg-test -mvad --hostcert --no-cleanup
test_exit=$?
set -e

# Some simple debug files for failures.
openssl x509 -in /etc/grid-security/hostcert.pem -noout -text
echo "------------ CE Logs --------------"
cat /var/log/condor-ce/JobRouterLog
echo "------------ Condor Logs --------------"
# Verify preun/postun in the spec file
cat /var/log/condor/GridmanagerLog*
echo "------------ Munge Logs --------------"
ls -l /var/log
ls -l /var/log/munge/
echo "------------ Slurm Logs --------------"
ls -l /var/log/slurm/
cat /var/log/slurm/slurm.log
cat /var/log/slurm/slurmctld.log
echo "------------ Torque Logs --------------"
ls -l /var/log/torque/
cat /var/log/torque/mom_logs/*
cat /var/log/torque/sched_logs/*
cat /var/log/torque/server_logs/*

yum remove -y 'blahp'

exit $test_exit
