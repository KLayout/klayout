
@Library("osconfig") _

properties([disableConcurrentBuilds()])

//  from shared library
target = osconfig()

currentBuild.description = "Pipelined " + target

node("master") {

  artefacts = pwd() + "/artefacts"
  target_dir = artefacts + "/" + target

  stage("Checkout sources") {

    checkout scm
    checkout_private()

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

      //  from shared library - only publish for normal branch, not for PR
      if (! BRANCH_NAME.startsWith('PR')) {
        publish(BRANCH_NAME, target, target_dir)
      }

    },
    "Unit testing": {
            
      ut_result = "no-result"
      withDockerContainer(image: "jenkins-${target}") {
        ut_result = run_ut(target)
      }
            
      junit(testResults: ut_result)

    }, 
    "Installtest": {
            
      withDockerContainer(image: "jenkins-${target}-basic") {
        //  from shared library
        installtest(target, target_dir)
      }

    })

  }
        
}

