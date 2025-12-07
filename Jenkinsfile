//Jenkinsfile和sonar-project.properties文件需要在master上先上传；
pipeline {
    agent {
        node {
            label 'yf3' //根据部门指定标签，软件一部=yf1 软件二部=yf2 软件三部=yf3
        }
    }
    tools {
        // C语言项目不需要JDK，可以移除或保留（不影响）
        // jdk 'jdk17'
    }
    triggers {//触发器，参数内除了token需要根据项目设置外，其他参数不变
        GenericTrigger(
            genericVariables: [
                [key: 'ref', value: '$.ref', expressionType: 'JSONPath'],
                [key: 'object_kind', value: '$.object_kind', expressionType: 'JSONPath'],
                [key: 'project', value: '$.project', expressionType: 'JSONPath'],
                [key: 'object_attributes', value: '$.object_attributes', expressionType: 'JSONPath']
            ],
            token: 'yf3-bike', //项目唯一标识，[产品线缩写]-[客户/项目标识]
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
                }
            }
        }

        stage('cppcheck 代码质量检测') {
            steps {
                script {
                    echo "开始 cppcheck 代码质量检测..."
                    
                    // 定义源代码路径
                    def sourcePath = 'GD32/source/Code_Files'
                    def reportDir = 'cppcheck-reports'
                    def reportFile = "${reportDir}/cppcheck-report.xml"
                    def txtReport = "${reportDir}/cppcheck-report.txt"
                    
                    // 创建报告目录
                    sh "mkdir -p ${reportDir}"
                    
                    // 执行 cppcheck 分析
                    // Windows 环境使用 bat，Linux 使用 sh
                    bat """
                        cppcheck ^
                        --enable=all ^
                        --xml ^
                        --xml-version=2 ^
                        --suppress=missingIncludeSystem ^
                        --suppress=unusedFunction ^
                        -i GD32/source/Code_Files/lib ^
                        -i GD32/source/project/Objects ^
                        -i GD32/source/project/Listings ^
                        -i GD32/source/Document ^
                        --output-file=${reportFile} ^
                        ${sourcePath} 2>&1 || echo "cppcheck completed with warnings"
                    """
                    
                    // 同时生成文本格式报告（便于查看）
                    bat """
                        cppcheck ^
                        --enable=all ^
                        --suppress=missingIncludeSystem ^
                        --suppress=unusedFunction ^
                        -i GD32/source/Code_Files/lib ^
                        -i GD32/source/project/Objects ^
                        -i GD32/source/project/Listings ^
                        -i GD32/source/Document ^
                        ${sourcePath} > ${txtReport} 2>&1 || echo "cppcheck completed"
                    """
                    
                    // 显示报告摘要
                    if (fileExists(txtReport)) {
                        echo "=== cppcheck 检测摘要 ==="
                        sh "head -50 ${txtReport}"  // Linux
                        // bat "type ${txtReport}"  // Windows，如果需要可以取消注释
                    }
                    
                    echo "cppcheck 分析完成，报告已生成: ${reportFile}"
                }
            }
        }
        
        stage('质量门禁检查') {
            steps {
                script {
                    def reportFile = 'cppcheck-reports/cppcheck-report.xml'
                    if (fileExists(reportFile)) {
                        def reportContent = readFile(reportFile)
                        
                        // 统计不同严重程度的问题
                        def errorCount = (reportContent =~ /severity="error"/).count
                        def warningCount = (reportContent =~ /severity="warning"/).count
                        def styleCount = (reportContent =~ /severity="style"/).count
                        
                        echo "=== cppcheck 质量门禁检查 ==="
                        echo "错误数量: ${errorCount}"
                        echo "警告数量: ${warningCount}"
                        echo "代码风格问题: ${styleCount}"
                        
                        // 如果错误超过阈值，失败构建
                        def maxErrors = 0  // 不允许有错误
                        if (errorCount > maxErrors) {
                            error "发现 ${errorCount} 个错误，超过阈值 ${maxErrors}，构建失败"
                        }
                        
                        // 如果警告超过阈值，可以警告但不失败（可选）
                        def maxWarnings = 100
                        if (warningCount > maxWarnings) {
                            echo "警告: 发现 ${warningCount} 个警告，超过阈值 ${maxWarnings}，建议修复"
                        } else {
                            echo "质量门禁检查通过！"
                        }
                    } else {
                        echo "未找到 cppcheck 报告文件，跳过质量门禁检查"
                    }
                }
            }
        }
    }
    
    post {
        always {
            echo "流水线执行完成，使用的分支: ${env.branch}"
            // 归档 cppcheck 报告
            archiveArtifacts artifacts: 'cppcheck-reports/**', allowEmptyArchive: true
        }
        success {
            echo "流水线执行成功！"
        }
        failure {
            echo "流水线执行失败！"
        }
    }
}