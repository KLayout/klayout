
@Library("osconfig") _

//  from shared library
target = osconfig()

currentBuild.description = "Pipelined " + target

node("master") {

  artefacts = pwd() + "/artefacts"
  target_dir = artefacts + "/" + target

  stage("Checkout sources") {

    checkout scm

  }

  stage("Building target ${target}") {
        
    withDockerContainer(image: "jenkins-${target}") {
      //  from shared library
      build(target, target_dir)
    }

  }
        
  stage("Publish and test") {
        
    parallel(
    "Publish": {

      //  from shared library
      publish(BRANCH_NAME, target, target_dir)

    },
    "Unit testing": {
            
      work_dir = pwd() + "/" + target + "/BUILD/build.linux-release"

      withDockerContainer(image: "jenkins-${target}") {
                
        sh """
cd ${work_dir}
set +e
LD_LIBRARY_PATH=. TESTSRC=../../.. TESTTMP=testtmp xvfb-run ./ut_runner -c -a | tee ut_runner.xml
set -e            
"""

      }
            
      junit(testResults: "${work_dir}/ut_runner.xml")

    }, 
    "Installtest": {
            
      withDockerContainer(image: "jenkins-${target}-basic") {
        //  from shared library
        installtest(target, target_dir)
      }

    })

  }
        
}

