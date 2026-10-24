// - adds the parameters which do not exist in the job (the existing parameters are not changed,
//   except a choice parameter whose choices differ from the definition : it is replaced)
//   * _definitions : [type: 'string' / 'boolean' / 'choice', name:, value: (default), choices: (choice), description:]
//   * _obsoletes   : names of the parameters to remove (not used any more)
//   * the parameters added are used from the next build (this build uses the default values in the script)
//   * uses the Jenkins API : approve the signatures once in 'Manage Jenkins > In-process Script Approval'
//     (the choice lists are compared with equals() of the definitions : no getter needing an approval)
@NonCPS
def addMissingParameters(String _job_name, List _definitions, List _obsoletes = []) {
    def job = jenkins.model.Jenkins.get().getItemByFullName(_job_name)
    if (job == null) {
        return []
    }

    def property = job.getProperty(hudson.model.ParametersDefinitionProperty)
    def existing = (property != null) ? new ArrayList(property.getParameterDefinitions()) : []
    def names = existing.collect { it.getName() } as Set

    // 1) the parameters not existing
    def added = []
    _definitions.each { d ->
        if (names.contains(d.name)) {
            return
        }
        def description = d.description ?: ''
        def definition
        switch (d.type) {
            case 'boolean':
                definition = new hudson.model.BooleanParameterDefinition(d.name, (d.value == true), description)
                break
            case 'choice':
                definition = new hudson.model.ChoiceParameterDefinition(d.name, (d.choices as String[]), description)
                break
            default:
                definition = new hudson.model.StringParameterDefinition(d.name, (d.value ?: '').toString(), description)
                break
        }
        existing.add(definition)
        added.add(d.name)
    }

    // 2) the choice parameters whose choices differ from the definition (replaced : an old choice list of the job would be used otherwise)
    def replaced = []
    _definitions.findAll { it.type == 'choice' && names.contains(it.name) }.each { d ->
        def index = existing.findIndexOf { it.getName() == d.name }
        def current = existing[index]
        // - compared with equals() of the definition (name, choices, description) : no getChoices() = no script approval needed
        def definition = new hudson.model.ChoiceParameterDefinition(d.name, (d.choices as String[]), d.description ?: '')
        if (definition.equals(current)) {
            return
        }
        existing[index] = definition
        replaced.add(d.name)
    }

    // 3) the obsolete parameters
    def removed = existing.findAll { _obsoletes.contains(it.getName()) }.collect { it.getName() }
    existing.removeAll { _obsoletes.contains(it.getName()) }

    // check) nothing changed
    if (added.isEmpty() && replaced.isEmpty() && removed.isEmpty()) {
        return []
    }

    // 4) update the job
    if (property != null) {
        job.removeProperty(property)
    }
    job.addProperty(new hudson.model.ParametersDefinitionProperty(existing))   // (saved by addProperty)

    return added.collect { "+${it}" } + replaced.collect { "~${it}" } + removed.collect { "-${it}" }
}

// - runs a svn command with the SVN credentials of Jenkins
//   * the password is passed by stdin (not shown in the process list) and not cached on the node (--no-auth-cache)
//   * _args : arguments of svn (ex. 'update', "checkout 'svn://...' .")
def svnWithCredentials(String _args, String _credentials_id = 'cgdk-svn') {
    withCredentials([usernamePassword(credentialsId: _credentials_id, usernameVariable: 'SVN_USER', passwordVariable: 'SVN_PASSWORD')]) {
        sh 'printf "%s" "$SVN_PASSWORD" | svn ' + _args + ' --non-interactive --no-auth-cache --username "$SVN_USER" --password-from-stdin'
    }
}

// - parameters of the job : only the parameters which do not exist are added (the existing parameters are not changed)
def parametersChanged = addMissingParameters(env.JOB_NAME, [
    [type: 'string',  name: 'NODE_NAME', value: 'NODE_UBUNTU', description: 'build node (label or node name)'],
    [type: 'string',  name: 'SVN_REMOTE', value: 'svn://192.168.0.60/ProjectCGDK/CGDK10.C++.public', description: 'SVN URL of CGDK'],
    [type: 'string',  name: 'WORKSPACE', value: '/project/CGDK10.Cpp', description: 'working copy of CGDK on the node'],
    [type: 'choice',  name: 'BUILD_TYPE', choices: ['All', 'Release', 'Debug'], description: 'build type (All : Release and Debug)'],
    [type: 'boolean', name: 'REBUILD', value: false, description: 'clean build'],
    [type: 'boolean', name: 'BUILD_CORE', value: true, description: 'build the core libraries (build/) and the unit tests'],
    [type: 'boolean', name: 'BUILD_TUTORIALS', value: false, description: 'build the tutorials'],
    [type: 'boolean', name: 'BUILD_EXAMPLES', value: false, description: 'build the examples'],
    [type: 'boolean', name: 'UNIT_TESTS', value: false, description: 'run the unit tests (they run also when BUILD_CORE is checked)'],
    [type: 'boolean', name: 'E2E_TESTS', value: true, description: 'run the admin e2e test (service -> host_agent -> admin_server. uses the ports 21887/21888/22044/22052/22053 of the node)'],
    [type: 'boolean', name: 'SANITIZER_TESTS', value: false, description: 'build the core with the sanitizers (in a copy of the sources : ~/.cache/cgdk_sanitizer) and run the unit tests and the admin e2e test (build/run_sanitizer_tests.sh)'],
    [type: 'choice',  name: 'SANITIZER_TYPE', choices: ['address,undefined', 'thread'], description: 'sanitizers of SANITIZER_TESTS : address,undefined (memory errors, undefined behavior) / thread (data races)'],
    [type: 'string',  name: 'FUZZ_ITERATIONS', value: '', description: 'iterations of the random input tests (http_fuzz, admin_fuzz, message_fuzz, certification_request). empty : the defaults (thousands). ex) 200000 for a nightly build'],
    [type: 'string',  name: 'FUZZ_SEED', value: '', description: "seed of the random input tests. empty : the default, 'build' : the build number (a different input each build)"],
    [type: 'string',  name: 'WINDOWS_NODE', value: '', description: 'windows node (label) for the windows admin e2e test (empty: skip). needs Visual Studio 18, svn and curl'],
    [type: 'string',  name: 'WINDOWS_WORKSPACE', value: 'C:\\jenkins\\CGDK10.Cpp', description: 'working copy of CGDK on the windows node'],
    [type: 'boolean', name: 'COMMIT', value: true, description: 'submit the result (only when the build and the unit tests succeeded)'],
    [type: 'string',  name: 'SUBMIT_REPOSITORY', value: 'svn://192.168.0.60/ProjectCGDK/release', description: 'SVN URL to submit the result']
])
if (parametersChanged) {
    echo "parameters of the job changed (used from the next build): ${parametersChanged}"
}

// - environment of the random input tests (FUZZ_ITERATIONS / FUZZ_SEED. empty : the defaults of the tests)
//   * CGDK_FUZZ_ITERATIONS, CGDK_FUZZ_SEED are read by the tests. a long run raises the timeout of each executable (UNIT_TEST_TIMEOUT)
@NonCPS
def fuzzEnvironment() {
    def environment = []
    def iterations = (params['FUZZ_ITERATIONS'] ?: '').toString().trim()
    def seed = (params['FUZZ_SEED'] ?: '').toString().trim()
    if (seed == 'build') {
        seed = env.BUILD_NUMBER
    }
    if (iterations != '') {
        environment << "CGDK_FUZZ_ITERATIONS=${iterations}"
        environment << 'UNIT_TEST_TIMEOUT=3600'
    }
    if (seed != '') {
        environment << "CGDK_FUZZ_SEED=${seed}"
    }
    return environment
}

// - description of the random input tests (to run the failed input again : the same iterations and seed)
@NonCPS
def fuzzDescription() {
    def environment = fuzzEnvironment()
    return environment.isEmpty() ? '' : " (fuzz: ${environment.findAll { !it.startsWith('UNIT_TEST_TIMEOUT') }.join(', ')})"
}

// - the build types of this build (BUILD_TYPE : All -> Release and Debug)
def buildTypes() {
    return env.CGDK_BUILD_TYPES.split(',') as List
}

// - configures the build of a type
//   (in-source cmake : 'make' builds the type configured last. so each type is configured right before it is built)
def configureBuild(String _build_type) {
    sh """
        set -e
        echo "Configuring ${_build_type} build..."
        cmake . -DCMAKE_BUILD_TYPE=${_build_type}
    """
}

pipeline {
    agent { label "${params.NODE_NAME}" }

    // - no checkout of this repository into the workspace of the node (the working copy is managed by the stages)
    options {
        skipDefaultCheckout()
    }

    stages {
        stage('Prepare') {
            steps {
                script {
                    env.BUILD_CORES = sh(script: 'nproc', returnStdout: true).trim()
                    echo "Build cores: ${env.BUILD_CORES}"

                    // - build types : All -> Release and Debug (in this order). an unknown value of an old job parameter is built as All
                    env.CGDK_BUILD_TYPES = (params.BUILD_TYPE == 'Release' || params.BUILD_TYPE == 'Debug') ? params.BUILD_TYPE : 'Release,Debug'
                    echo "Build types: ${env.CGDK_BUILD_TYPES}"
                }
            }
        }

        stage('Update') {
            steps {
                script {
                    echo "Checking out from SVN: ${params.SVN_REMOTE}"
                    echo "Workspace: ${env.WORKSPACE}"

                    // - checkout / update with the SVN credentials of Jenkins (svnWithCredentials : 'cgdk-svn')
                    //   (a plain 'svn update' needs the credentials cached on the node : fails on a node without them)
                    dir("${env.WORKSPACE}") {
                        if (!fileExists('.svn')) {
                            echo "Performing SVN checkout..."
                            svnWithCredentials("checkout '${params.SVN_REMOTE}' .")
                        } else {
                            echo "Performing SVN update..."
                            svnWithCredentials('update')
                        }
                    }

                    // - revision of the working copy
                    //   (the jobs calling this job get it from 'buildVariables.CGDK_REVISION' : ex. gruma.BuildServers)
                    dir("${env.WORKSPACE}") {
                        env.CGDK_REVISION = sh(script: 'svn info --show-item revision', returnStdout: true).trim()
                    }
                    echo "CGDK revision: r${env.CGDK_REVISION}"

                    def revision = env.SVN_REVISION ?: env.SVN_REVISION_1 ?: env.CGDK_REVISION
                    if (revision) {
                        currentBuild.displayName = "#${env.BUILD_NUMBER} r${revision}"
                        currentBuild.description = "SVN revision: r${revision}"
                    }
                }
            }
        }

        stage('Prepare Build') {
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        // - configures each build type (REBUILD : cleans the objects. the object files are shared by the types : in-source build)
                        for (build_type in buildTypes()) {
                            configureBuild(build_type)
                            if (params.REBUILD == true) {
                                sh 'make clean'
                            }
                        }
                    }
                }
            }
        }

        stage('Build Core') {
            when {
                expression {
                    return params['BUILD_CORE'] == true
                }
            }
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        for (build_type in buildTypes()) {
                            configureBuild(build_type)
                            echo "Building Core (${build_type}) with ${env.BUILD_CORES} cores..."
                            sh "make -j${env.BUILD_CORES} -C build"
                        }
                    }
                }
            }
        }

        stage('Build Tutorials') {
            when {
                expression {
                    return params['BUILD_TUTORIALS'] == true
                }
            }
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        for (build_type in buildTypes()) {
                            configureBuild(build_type)
                            echo "Building Tutorials (${build_type}) with ${env.BUILD_CORES} cores..."
                            sh "make -j${env.BUILD_CORES} -C tutorials"
                        }
                    }
                }
            }
        }

        stage('Build Examples') {
            when {
                expression {
                    return params['BUILD_EXAMPLES'] == true
                }
            }
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        for (build_type in buildTypes()) {
                            configureBuild(build_type)
                            echo "Building Examples (${build_type}) with ${env.BUILD_CORES} cores..."
                            sh "make -j${env.BUILD_CORES} -C examples"
                        }
                    }
                }
            }
        }

        // - run after the build succeeded (unit tests are built with the core when GoogleTest is installed)
        //   * results are published as JUnit (Test Result / Test Result Trend of the job)
        //   * a failed test makes the build UNSTABLE (the stages after are still executed, 'Commit' is skipped)
        stage('Unit Test') {
            when {
                expression {
                    return params['BUILD_CORE'] == true || params['UNIT_TESTS'] == true
                }
            }
            options {
                timeout(time: 180, unit: 'MINUTES')
            }
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        // - each build type : test-results/<type>/*.xml (JUnit), *.log
                        for (build_type in buildTypes()) {
                            def result = 0
                            withEnv(fuzzEnvironment()) {
                                result = sh(script: "bash build/run_unit_tests.sh ${build_type} test-results/${build_type}", returnStatus: true)
                            }

                            if (result == 2) {
                                // - no unit test executable : GoogleTest is not installed on this node
                                unstable("No unit test executable : install GoogleTest on '${env.NODE_NAME}' ('apt install libgtest-dev libgmock-dev')")
                                currentBuild.description = (currentBuild.description ? currentBuild.description + "<br>" : "") + "unit test (${build_type}) : not built (no GoogleTest)"
                            } else {
                                def summary = junit(testResults: "test-results/${build_type}/*.xml", allowEmptyResults: false)
                                def text = "unit test (${build_type}) : ${summary.passCount} passed, ${summary.failCount} failed, ${summary.skipCount} skipped (total ${summary.totalCount})" + fuzzDescription()
                                echo text
                                currentBuild.description = (currentBuild.description ? currentBuild.description + "<br>" : "") + text
                            }
                        }
                    }
                }
            }
            post {
                always {
                    // - output of each unit test executable
                    dir("${env.WORKSPACE}") {
                        archiveArtifacts(artifacts: 'test-results/Release/*.log, test-results/Debug/*.log', allowEmptyArchive: true)
                    }
                }
            }
        }

        // - unit tests with AddressSanitizer / UndefinedBehaviorSanitizer (build/run_sanitizer_tests.sh)
        //   * the sources are copied to a work directory and built there (the normal build is not changed)
        //   * a sanitizer error or a failed test makes the build UNSTABLE ('Commit' is skipped)
        stage('Sanitizer Test') {
            when {
                expression {
                    return params['SANITIZER_TESTS'] == true
                }
            }
            options {
                timeout(time: 240, unit: 'MINUTES')
            }
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        def result = 0
                        // - e2e ports : 21887.. + 500 + EXECUTOR_NUMBER*100 (not the ports of 'Admin E2E Test')
                        def port_offset = 500 + ((env.EXECUTOR_NUMBER ?: '0') as int) * 100
                        def sanitizer = (params['SANITIZER_TYPE'] ?: 'address,undefined').toString()
                        withEnv(fuzzEnvironment() + ["SANITIZER=${sanitizer}", "CGDK_E2E_PORT_OFFSET=${port_offset}"]) {
                            result = sh(script: "bash build/run_sanitizer_tests.sh test-results-sanitizer", returnStatus: true)
                        }
                        def summary = junit(testResults: 'test-results-sanitizer/*.xml', allowEmptyResults: true)
                        def text = (result == 0)
                            ? "sanitizer test (${sanitizer}) : ${summary.passCount} passed, ${summary.skipCount} skipped (total ${summary.totalCount})" + fuzzDescription()
                            : "sanitizer test (${sanitizer}) : failed (exit ${result}, ${summary.failCount} failed)" + fuzzDescription()
                        if (result != 0) {
                            unstable(text)
                        }
                        echo text
                        currentBuild.description = (currentBuild.description ? currentBuild.description + "<br>" : "") + text
                    }
                }
            }
            post {
                always {
                    dir("${env.WORKSPACE}") {
                        archiveArtifacts(artifacts: 'test-results-sanitizer/*.log, test-results-sanitizer/e2e/**/*.log, test-results-sanitizer/e2e/reports/*', allowEmptyArchive: true)
                    }
                }
            }
        }

        // - e2e test of the admin system (service -> host_agent -> admin_server)
        //   * build/src/server.admin/unit_test/e2e/run_admin_e2e.sh (the same scenarios as run_admin_e2e.ps1 of windows)
        //   * needs the core build (host_agent, admin_server, unit_test.server_system.service) and curl
        //   * ports are shifted by EXECUTOR_NUMBER*100 (CGDK_E2E_PORT_OFFSET)
        //   * a failed check makes the build UNSTABLE ('Commit' is skipped)
        stage('Admin E2E Test') {
            when {
                expression {
                    // - E2E_TESTS : true when the parameter does not exist yet (the first build after it is added)
                    return params['BUILD_CORE'] == true && params['E2E_TESTS'] != false
                }
            }
            options {
                timeout(time: 30, unit: 'MINUTES')
            }
            steps {
                dir("${env.WORKSPACE}") {
                    script {
                        sh 'rm -rf test-results/e2e && mkdir -p test-results/e2e'
                        // - ports : 21887.. + EXECUTOR_NUMBER*100 (builds on the same node do not conflict)
                        def port_offset = ((env.EXECUTOR_NUMBER ?: '0') as int) * 100
                        // - each build type : test-results/e2e/<type>
                        for (build_type in buildTypes()) {
                            sh "mkdir -p test-results/e2e/${build_type}"
                            def result = sh(script: "CGDK_E2E_PORT_OFFSET=${port_offset} TMPDIR='${env.WORKSPACE}/test-results/e2e/${build_type}' bash build/src/server.admin/unit_test/e2e/run_admin_e2e.sh ${build_type} > test-results/e2e/${build_type}/run_admin_e2e.log 2>&1", returnStatus: true)
                            sh "cat test-results/e2e/${build_type}/run_admin_e2e.log"
                            def text = (result == 0) ? "admin e2e (${build_type}) : passed" : "admin e2e (${build_type}) : failed (exit ${result})"
                            if (result != 0) {
                                unstable(text)
                            }
                            echo text
                            currentBuild.description = (currentBuild.description ? currentBuild.description + "<br>" : "") + text
                        }
                    }
                }
            }
            post {
                always {
                    // - output of the script and the logs of admin_server/host_agent
                    dir("${env.WORKSPACE}") {
                        archiveArtifacts(artifacts: 'test-results/e2e/**/*.log', allowEmptyArchive: true)
                    }
                }
            }
        }

        // - windows : build (VS18 Debug / Release) + unit tests of each (build\run_unit_tests.ps1) + admin e2e test (run_admin_e2e.ps1 : Debug)
        //   * runs only when WINDOWS_NODE is set
        //   * a failed check makes the build UNSTABLE ('Commit' is skipped)
        stage('Windows E2E Test') {
            when {
                expression {
                    return (params['WINDOWS_NODE'] ?: '') != ''
                }
            }
            agent { label "${params.WINDOWS_NODE}" }
            options {
                skipDefaultCheckout()
                timeout(time: 180, unit: 'MINUTES')
            }
            steps {
                script {
                    def workspace_windows = params['WINDOWS_WORKSPACE'] ?: 'C:\\jenkins\\CGDK10.Cpp'

                    // 1) checkout / update (the password is passed by stdin)
                    dir(workspace_windows) {
                        withCredentials([usernamePassword(credentialsId: 'cgdk-svn', usernameVariable: 'SVN_USER', passwordVariable: 'SVN_PASSWORD')]) {
                            if (!fileExists('.svn')) {
                                powershell "\$env:SVN_PASSWORD | svn checkout '${params.SVN_REMOTE}' . --non-interactive --no-auth-cache --username \$env:SVN_USER --password-from-stdin; exit \$LASTEXITCODE"
                            } else {
                                powershell "\$env:SVN_PASSWORD | svn update --non-interactive --no-auth-cache --username \$env:SVN_USER --password-from-stdin; exit \$LASTEXITCODE"
                            }
                        }
                    }

                    // 2) build (Debug / Release x64. the libraries, host_agent, admin_server, the unit tests)
                    //    * Release : some tests run only in Release (ex. message_fuzz : the Debug libraries assert on a buffer overflow)
                    def configurations = ['Debug', 'Release']
                    dir("${workspace_windows}\\build\\src") {
                        def solutions = [
                            'system\\CGDK10.system.vs18.slnx',
                            'net.socket\\CGDK10.net.socket.vs18.slnx',
                            'server.system\\CGDK10.server.system.vs18.slnx',
                            'server.admin\\CGDK10.server.admin.host_agent.service.vs18.slnx',
                            'server.admin\\CGDK10.server.admin.admin_server.vs18.slnx'
                        ]
                        for (configuration in configurations) {
                            for (solution in solutions) {
                                bat "call \"C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat\" x64 && msbuild ${solution} -m -nologo -v:m -p:Configuration=${configuration} -p:Platform=x64"
                            }
                        }
                    }

                    // 3) unit tests of each configuration (build\run_unit_tests.ps1. benchmarks and server_query excluded)
                    //    * results : test-results-windows\<configuration>\*.xml (JUnit). a failed test makes the build UNSTABLE
                    dir(workspace_windows) {
                        for (configuration in configurations) {
                            def dir_result = "test-results-windows\\${configuration}"
                            def result = 0
                            withEnv(fuzzEnvironment()) {
                                result = powershell(script: "powershell -ExecutionPolicy Bypass -File build\\run_unit_tests.ps1 -Configuration ${configuration} -ResultDir ${dir_result}; exit \$LASTEXITCODE", returnStatus: true)
                            }
                            def text = ''
                            if (result == 2) {
                                text = "windows unit test (${configuration}) : no unit test executable"
                            } else {
                                def summary = junit(testResults: "test-results-windows/${configuration}/*.xml", allowEmptyResults: true)
                                text = "windows unit test (${configuration}) : ${summary.passCount} passed, ${summary.failCount} failed, ${summary.skipCount} skipped (total ${summary.totalCount})" + fuzzDescription()
                            }
                            if (result != 0) {
                                unstable(text)
                            }
                            echo text
                            currentBuild.description = (currentBuild.description ? currentBuild.description + "<br>" : "") + text
                        }
                        archiveArtifacts(artifacts: 'test-results-windows/**/*.log', allowEmptyArchive: true)
                    }

                    // 4) e2e
                    dir(workspace_windows) {
                        def port_offset = ((env.EXECUTOR_NUMBER ?: '0') as int) * 100
                        def result = powershell(script: "\$env:CGDK_E2E_PORT_OFFSET = '${port_offset}'; powershell -ExecutionPolicy Bypass -File build\\src\\server.admin\\unit_test\\e2e\\run_admin_e2e.ps1 -Configuration Debug; exit \$LASTEXITCODE", returnStatus: true)
                        def text = (result == 0) ? 'windows admin e2e : passed' : "windows admin e2e : failed (exit ${result})"
                        if (result != 0) {
                            unstable(text)
                        }
                        echo text
                        currentBuild.description = (currentBuild.description ? currentBuild.description + "<br>" : "") + text
                    }
                }
            }
        }

        stage('Commit') {
            when {
                expression {
                    // - only when the build and the unit tests succeeded
                    return params['COMMIT'] == true && currentBuild.currentResult == 'SUCCESS'
                }
            }
            steps {
                sh 'echo "submit"'
            }
        }
    }

    post {
        success {
            echo 'Build succeeded!'
        }
        unstable {
            echo 'Build succeeded, but unit tests failed (or were not built)!'
        }
        failure {
            echo 'Build failed!'
        }
    }
}
