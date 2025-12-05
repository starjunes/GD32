//Jenkinsfile和sonar-project.properties文件需要在master上先上传；
pipeline {
    agent {
        node {
            label 'yf3' //根据部门指定标签，软件一部=yf1 软件二部=yf2 软件三部=yf3
        }
    }
    tools {
        jdk 'jdk17'  // 这里使用在 Jenkins 中配置的 JDK 名称
    }
    triggers {//触发器，参数内除了token需要根据项目设置外，其他参数不变
        GenericTrigger(
            genericVariables: [
                [key: 'ref', value: '$.ref', expressionType: 'JSONPath'],
                [key: 'object_kind', value: '$.object_kind', expressionType: 'JSONPath'],
                [key: 'project', value: '$.project', expressionType: 'JSONPath'],
                [key: 'object_attributes', value: '$.object_attributes', expressionType: 'JSONPath']
            ],
            token: 'yf4-bike', //项目唯一标识，[产品线缩写]-[客户/项目标识]
            printContributedVariables: true,
            printPostContent: true
        )
    }

    stages {
        stage('智能分析提取') {
            steps {
                script {
                    // 解析 project JSON
                    def projectJson = readJSON text: "${env.project}"
                    def ssh_url = projectJson.ssh_url

                    // 初始化分支变量
                    env.branch = ''
                    env.source_branch = ''
                    env.target_branch = ''

                    // 解析 object_attributes JSON
                    def objectAttributesJson = [:]
                    if (env.object_attributes && env.object_attributes != 'null') {
                        try {
                            objectAttributesJson = readJSON text: "${env.object_attributes}"
                            echo "objectAttributesJson: ${objectAttributesJson}"
                        } catch (Exception e) {
                            echo "解析object_attributes失败: ${e.message}"
                        }
                    }

                    // 根据事件类型智能提取分支
                    def objectKind = "${env.object_kind}"

                    if (objectKind == 'push') {
                        def ref = "${env.ref}"
                        if (ref) {
                            env.branch = ref.replace('refs/heads/', '')
                            echo "Push 事件分支: ${env.branch}"
                        }
                    } else if (objectKind == 'merge_request') {
                        if (objectAttributesJson) {
                            env.source_branch = objectAttributesJson.source_branch ?: ''
                            env.target_branch = objectAttributesJson.target_branch ?: ''
                            echo "MR 事件源分支: ${env.source_branch}, 目标分支: ${env.target_branch}"

                            // 修正逻辑：使用源分支进行构建
                            if (env.source_branch && !env.source_branch.isEmpty()) {
                                env.branch = env.source_branch
                                echo "merge_request 使用源分支构建: ${env.branch}"
                            }
                        } else {
                            echo "object_attributes 数据不可用"
                        }
                    }

                    // 确保分支变量有值
                    if (!env.branch || env.branch.isEmpty() || env.branch == 'null') {
                        env.branch = 'master'
                        echo "使用默认分支: ${env.branch}"
                    }

                    echo "最终使用的分支: ${env.branch}"

                    // 执行分支checkout（移除master分支的限制）
                    checkout scm: [
                        $class: 'GitSCM',
                        branches: [[name: "*/${env.branch}"]],
                        extensions: [[$class: 'LocalBranch']],
                        userRemoteConfigs: [[url: ssh_url]]
                    ]
                    // 读取sonar配置并存入环境变量，有其他参数时，可以继续增加
                    if (fileExists('sonar-project.properties')) {
                        def sonarProps = readProperties file: 'sonar-project.properties'
                        env.SONAR_PROJECT_KEY = sonarProps.SONAR_PROJECT_KEY ?: ''
                        env.SONAR_PROJECT_NAME = sonarProps.SONAR_PROJECT_NAME ?: ''
                        env.SONAR_HOST_URL = sonarProps.SONAR_HOST_URL ?: ''
                        env.SONAR_TOKEN = sonarProps.SONAR_TOKEN ?: ''
                        env.SONAR_EXCLUSIONS = sonarProps.SONAR_EXCLUSIONS ?: ''
                        env.SONAR_SOURCES = sonarProps.SONAR_SOURCES ?: ''
                        env.SONAR_TESTS = sonarProps.SONAR_TESTS ?: ''
                        echo "SonarQube配置已加载到环境变量"
                    }
                }
            }
        }

        stage('Maven 编译') {
            steps {
                script {
                    sh "mvn clean compile -P dev -Dmaven.test.skip=true"
                }
            }
        }
        stage('SonarQube 代码分析') {
            steps {
                script {
                    if (env.SONAR_PROJECT_KEY) {
                        echo "开始SonarQube代码分析..."
                        sh """
                            mvn sonar:sonar -P dev \
                            -Dsonar.projectKey=${env.SONAR_PROJECT_KEY} \
                            -Dsonar.projectName=${env.SONAR_PROJECT_NAME} \
                            -Dsonar.host.url=${env.SONAR_HOST_URL} \
                            -Dsonar.token=${env.SONAR_TOKEN} \
                            -Dsonar.sources=${env.SONAR_SOURCES} \
                            -Dsonar.exclusions=${env.SONAR_EXCLUSIONS}
                        """
                        echo "SonarQube分析完成，等待质量门禁检查..."
                    } else {
                        echo "未找到SonarQube配置，跳过代码分析"
                    }
                }
            }
        }
    }
    stage('质量门禁检查') {

    }
    post {
        always {
            echo "流水线执行完成，使用的分支: ${env.branch}"
            // 清理工作空间（可选）
            //cleanWs()
        }
        success {
            echo "流水线执行成功！"
        }
        failure {
            echo "流水线执行失败！"
        }
    }
}

