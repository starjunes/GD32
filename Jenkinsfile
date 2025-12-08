//Jenkinsfile和sonar-project.properties文件需要在master上先上传；
pipeline {
    agent {
        node {
            label 'yf3' //根据部门指定标签，软件一部=yf1 软件二部=yf2 软件三部=yf3
        }
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
                                    // 检查是否有Webhook变量
                    def hasWebhookData = env.project != null && env.project != 'null' && env.project != ''
                    
                    if (!hasWebhookData) {
                        echo "未检测到Webhook数据，可能是手动触发，使用默认SCM配置"
                        env.branch = 'master'
                        checkout scm
                        return  // 提前返回，跳过后续处理
                    }
                    
                    // 解析 project JSON（仅当有Webhook数据时）
                    def projectJson = [:]
                    def ssh_url = ''
                    try {
                        projectJson = readJSON text: "${env.project}"
                        ssh_url = projectJson.ssh_url ?: ''
                        echo "解析到的SSH URL: ${ssh_url}"
                    } catch (Exception e) {
                        echo "解析project JSON失败: ${e.message}，使用默认SCM"
                        env.branch = 'master'
                        checkout scm
                        return
                    }

                    // 初始化分支变量
                    env.branch = ''
                    env.source_branch = ''
                    env.target_branch = ''

                    // 解析 object_attributes JSON
                    def objectAttributesJson = [:]
                    if (env.object_attributes && env.object_attributes != 'null' && env.object_attributes != '') {
                        try {
                            objectAttributesJson = readJSON text: "${env.object_attributes}"
                            echo "objectAttributesJson: ${objectAttributesJson}"
                        } catch (Exception e) {
                            echo "解析object_attributes失败: ${e.message}"
                        }
                    }

                    // 根据事件类型智能提取分支
                    def objectKind = "${env.object_kind}" ?: ''

                    if (objectKind == 'push') {
                        def ref = "${env.ref}" ?: ''
                        if (ref && ref != 'null' && !ref.isEmpty()) {
                            env.branch = ref.replace('refs/heads/', '')
                            echo "Push 事件分支: ${env.branch}"
                        }
                    } else if (objectKind == 'merge_request') {
                        if (objectAttributesJson && !objectAttributesJson.isEmpty()) {
                            env.source_branch = objectAttributesJson.source_branch ?: ''
                            env.target_branch = objectAttributesJson.target_branch ?: ''
                            echo "MR 事件源分支: ${env.source_branch}, 目标分支: ${env.target_branch}"

                            if (env.source_branch && !env.source_branch.isEmpty()) {
                                env.branch = env.source_branch
                                echo "merge_request 使用源分支构建: ${env.branch}"
                            }
                        }
                    }

                    // 确保分支变量有值
                    if (!env.branch || env.branch.isEmpty() || env.branch == 'null') {
                        env.branch = 'master'
                        echo "使用默认分支: ${env.branch}"
                    }

                    echo "最终使用的分支: ${env.branch}"

                    // 执行分支checkout
                    if (ssh_url && !ssh_url.isEmpty()) {
                        checkout scm: [
                            $class: 'GitSCM',
                            branches: [[name: "*/${env.branch}"]],
                            extensions: [[$class: 'LocalBranch']],
                            userRemoteConfigs: [[url: ssh_url]]
                        ]
                    } else {
                        echo "SSH URL为空，使用Pipeline任务配置的SCM"
                        checkout scm
                    }
                }
            }
        }

stage('cppcheck 代码质量检测') {
    steps {
        script {
            echo "开始 cppcheck 代码质量检测..."
            
            // 检测操作系统类型
            def isUnix = isUnix()
            
            // 定义源代码路径
            def sourcePath = 'GD32/source/Code_Files/_app/can'
            def reportDir = 'cppcheck-reports'
            def reportFile = "${reportDir}/cppcheck-report.xml"
            def txtReport = "${reportDir}/cppcheck-report.txt"
            
            // 创建报告目录（跨平台）
            if (isUnix) {
                sh "mkdir -p ${reportDir}"
            } else {
                bat """
                    if not exist "${reportDir}" mkdir "${reportDir}"
                """
            }
            
            // 设置超时时间（2小时）
            def timeoutMinutes = 120
            
            // 执行 cppcheck 分析（跨平台，优化版本）
            if (isUnix) {
                timeout(time: timeoutMinutes, unit: 'MINUTES') {
                    sh """
                        cppcheck \\
                        --enable=warning,performance,portability,style,information \\
                        --xml \\
                        --xml-version=2 \\
                        -j 4 \\
                        --suppress=missingIncludeSystem \\
                        --suppress=unusedFunction \\
                        -i GD32/source/Code_Files/lib \\
                        -i GD32/source/project/Objects \\
                        -i GD32/source/project/Listings \\
                        -i GD32/source/Document \\
                        --output-file=${reportFile} \\
                        ${sourcePath} 2>&1 || echo "cppcheck completed with warnings"
                    """
                }
            } else {
                timeout(time: timeoutMinutes, unit: 'MINUTES') {
                    bat """
                        cppcheck ^
                        --enable=warning,performance,portability,style,information ^
                        --xml ^
                        --xml-version=2 ^
                        -j 4 ^
                        --suppress=missingIncludeSystem ^
                        --suppress=unusedFunction ^
                        -i GD32/source/Code_Files/lib ^
                        -i GD32/source/project/Objects ^
                        -i GD32/source/project/Listings ^
                        -i GD32/source/Document ^
                        --output-file=${reportFile} ^
                        ${sourcePath} 2>&1 || echo "cppcheck completed with warnings"
                    """
                }
            }
            
            // 从 XML 报告生成文本摘要（不再重新运行 cppcheck）
            if (fileExists(reportFile)) {
                echo "=== cppcheck 检测摘要 ==="
                if (isUnix) {
                    // 使用 grep 提取关键信息
                    sh """
                        echo "检查报告文件: ${reportFile}"
                        if [ -s ${reportFile} ]; then
                            grep -c 'severity="error"' ${reportFile} || echo "0"
                            grep -c 'severity="warning"' ${reportFile} || echo "0"
                        fi
                    """
                } else {
                    // Windows: 读取 XML 并提取摘要
                    def content = readFile(reportFile)
                    def errorCount = (content =~ /severity="error"/).count
                    def warningCount = (content =~ /severity="warning"/).count
                    echo "错误数量: ${errorCount}"
                    echo "警告数量: ${warningCount}"
                }
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
                        def maxErrors = 5  // 不允许有错误
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