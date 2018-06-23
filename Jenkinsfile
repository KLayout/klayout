
//  a test ...
org.klayout.osconfig()

target = params.platform
currentBuild.description = "Pipelined "+target

node("master") {

    branch = 'staging'
    artefacts = pwd() + "/artefacts"
    src_dir = pwd()

    target_dir = artefacts + "/" + target
    work_dir = pwd() + "/" + target

    stage("Building target ${target}")
        
    sh("rm -rf ${target_dir}")
    sh("mkdir -p ${target_dir}")
        
    withDockerContainer(image: "jenkins-${target}") {
    
        sh """
echo "UID=\$UID"
echo "GID=\$GID"
ls -al scripts/rpm-data/klayout.spec

cat /etc/redhat-release

. ./version.sh
rpmbuild -ba scripts/rpm-data/klayout.spec \\
  -D "_topdir ${work_dir}" \\
  -D "git_source ${src_dir}" \\
  -D "git_version \${KLAYOUT_VERSION}" \\
  -D "target_system ${target}"
  
cp ${work_dir}/RPMS/*/*.rpm ${target_dir}
"""

    }
        
    stage("Publish and test")
        
    parallel(
    "Unit testing": {
            
        withDockerContainer(image: "jenkins-${target}") {
                
            sh """
cd ${work_dir}/BUILD/build.linux-release
set +e
LD_LIBRARY_PATH=. TESTSRC=../../.. TESTTMP=testtmp xvfb-run ./ut_runner -c -a | tee ut_runner.xml
set -e            
"""

        }
            
        junit(testResults: "${target}/BUILD/build.linux-release/ut_runner.xml")

    }, 
    "Installtest": {
            
        withDockerContainer(image: "jenkins-${target}-basic") {

            sh """
cd ${target_dir}

echo \$USER
sudo yum -y install klayout-*.x86_64.rpm

klayout -b -h
"""
                
        }

    })
        
}

