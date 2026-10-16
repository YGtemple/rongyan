// ============================================================
// mainwindow.cpp — 融研智笔 Qt 桌面端实现
//
// 与 web 版（index.html）共用同一套业务逻辑：
//   三栏布局 + HiAgent 同步工作流 + QSettings 本地持久化
//
// 结构顺序：构造/析构 → 初始化 → 持久化 → 界面交互
//           → 动态句式 → 网络请求 → 结果解析
//
// 注：源码中的 HiAgent Apikey 已替换为占位符 YOUR_HIAGENT_API_KEY，
//     使用前请替换为你自己申请的值。
// ============================================================
#include "mainwindow.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QTabWidget>
#include <QGroupBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include<utility>
#include<QCloseEvent>
#include<QJsonDocument>
#include<QJsonArray>
#include<QJsonParseError>
#include<QJsonObject>
#include <QJsonValue>
#include<QHash>
#include <QDebug>   // 用于 qWarning
#include <QVariantMap>
#include<QString>
#include<QApplication>
#include<QClipboard>
#include<QMessageBox>
#include<QFileDialog>
#include <QUrl>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , settings(new QSettings(this))
    , networkManager(new QNetworkAccessManager(this))
    , totalEssayCount(0)
    , currentScoreLevel("初级")

{
    initUI();      // 界面初始化
    initConnect(); // 信号槽绑定
    initSentenceLibrary();
    loadUserData();// 加载用户数据
}

MainWindow::~MainWindow()
{
}

// ---------- 初始化 ----------
void MainWindow::initSentenceLibrary()
{
    m_sentenceLib.clear(); // 程序启动时清空，避免脏数据

    QSettings settings("YourCompany", "融研智笔");
    QVariantMap libMap = settings.value("user/sentenceLibrary").toMap();
    for (auto it = libMap.begin(); it != libMap.end(); ++it) {
        m_sentenceLib[it.key()] = it.value().toStringList();
    }
}

// ---------- 三栏界面构建（样式全部内联，无 .qss 外部依赖）----------
void MainWindow::initUI()
{
    setWindowTitle("融研智笔·英语写作系列智能体");
    setMinimumSize(1200, 800);
    setStyleSheet(R"(
        QWidget {
            font-family: "Microsoft YaHei UI", "PingFang SC", sans-serif;
            font-size: 14px;
            background-color: #FCFCFD;
            color: #333333;
        }
    )");

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    setCentralWidget(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0);
    mainLayout->setSpacing(0);

    leftPanel = new QWidget(this);
    leftLayout = new QVBoxLayout(leftPanel);
    leftPanel->setFixedWidth(270);
    leftPanel->setStyleSheet(R"(
        QWidget {
            background-color: #F5F7FA;
        }
    )");
    leftLayout->setContentsMargins(22, 24, 22, 24);
    leftLayout->setSpacing(18);

    leftTitleLabel = new QLabel("📊 学习数据分析", leftPanel);
    leftTitleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 17px;
            font-weight: 500;
            color: #2A2A2A;
        }
    )");
    leftLayout->addWidget(leftTitleLabel);

    dataGroup = new QGroupBox("统计数据", leftPanel);
    QVBoxLayout *dataLayout = new QVBoxLayout(dataGroup);
    dataLayout->setSpacing(14);
    dataLayout->setContentsMargins(16, 18, 16, 18);
    dataGroup->setStyleSheet(R"(
        QGroupBox {
            font-weight: 500;
            color: #555555;
            font-size: 15px;
            border: none;
            border-radius: 12px;
            background-color: #FFFFFF;
            padding: 4px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 8px;
        }
    )");
    // Qt6 安全阴影写法：挂在本对象上，避免被 QCss 引擎误管
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(12);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0,0,0,10));
    dataGroup->setGraphicsEffect(shadow);

    totalEssayLabel = new QLabel("总作文数: 0", dataGroup);
    dataLayout->addWidget(totalEssayLabel);
    scoreLevelLabel = new QLabel("当前评分等级: 初级", dataGroup);
    dataLayout->addWidget(scoreLevelLabel);
    knowledgeCountLabel = new QLabel("掌握知识点: 0个", dataGroup);
    dataLayout->addWidget(knowledgeCountLabel);
    leftLayout->addWidget(dataGroup);

    weakPointTitle = new QLabel("🔍 薄弱知识点", leftPanel);
    weakPointTitle->setStyleSheet(R"(
        QLabel {
            font-weight: 500;
            color: #F27272;
            font-size: 15px;
        }
    )");
    leftLayout->addWidget(weakPointTitle);

    weakPointEdit = new QTextEdit(leftPanel);
    weakPointEdit->setReadOnly(true);
    weakPointEdit->setStyleSheet(R"(
        QTextEdit {
            border: none;
            border-radius: 12px;
            padding: 14px;
            background-color: #FFFFFF;
            color: #666666;
            selection-background-color: #E8F0FF;
        }
    )");
    weakPointEdit->setHtml("<span style='color:#666'>系统将自动分析您的薄弱知识点：<br>"
                           "• <span style='color:#F27272'>语法错误</span>（时态/单复数/从句）<br>"
                           "• <span style='color:#6495ED'>词汇搭配不当</span><br>"
                           "• <span style='background:#FFE8A3; padding:3px 6px; border-radius:4px'>逻辑衔接缺失</span></span>");
    leftLayout->addWidget(weakPointEdit);

    leftLayout->addStretch(1);
    mainLayout->addWidget(leftPanel);

    centerPanel = new QWidget(this);
    centerLayout = new QVBoxLayout(centerPanel);
    centerPanel->setMinimumWidth(650);
    centerLayout->setContentsMargins(28, 28, 28, 28);
    centerLayout->setSpacing(20);

    centerTitleLabel = new QLabel("✍️ 融研智笔·英语写作系列智能体", centerPanel);
    centerTitleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 24px;
            font-weight: 500;
            color: #1F1F1F;
            qproperty-alignment: AlignCenter;
        }
    )");
    centerLayout->addWidget(centerTitleLabel);

    examTypeLayout = new QHBoxLayout();
    examTypeLabel = new QLabel("考试类型:");
    examTypeLabel->setStyleSheet("font-weight: 500; color: #444;");
    examTypeCombo = new QComboBox();
    examTypeCombo->addItems({"大学英语四级", "大学英语六级", "专四", "专八", "考研英语", "雅思托福"});
    examTypeCombo->setStyleSheet(R"(
        QComboBox {
            border: none;
            border-radius: 10px;
            padding: 8px 14px;
            min-height: 36px;
            background-color: #FFFFFF;
        }
        QComboBox::drop-down { border: none; }
        QComboBox::drop-down:hover { background-color: #F8F9FA; }
    )");
    examTypeLayout->addWidget(examTypeLabel);
    examTypeLayout->addWidget(examTypeCombo);
    examTypeLayout->addStretch(1);
    examTypeLayout->setSpacing(14);
    centerLayout->addLayout(examTypeLayout);

    topicLayout = new QHBoxLayout();
    topicLabel = new QLabel("作文主题:");
    topicLabel->setStyleSheet("font-weight: 500; color: #444;");
    topicEdit = new QLineEdit();
    topicEdit->setPlaceholderText("请输入作文主题...");
    topicEdit->setStyleSheet(R"(
        QLineEdit {
            border: none;
            border-radius: 10px;
            padding: 10px 14px;
            min-height: 36px;
            background-color: #FFFFFF;
            color: #333;
        }
        QLineEdit:focus { background-color: #F8F9FA; }
    )");
    topicLayout->addWidget(topicLabel);
    topicLayout->addWidget(topicEdit);
    topicLayout->addStretch(1);
    topicLayout->setSpacing(14);
    centerLayout->addLayout(topicLayout);

    inputBtnLayout = new QHBoxLayout();
    selectImgBtn = new QPushButton("📷 选择图片");
    pasteBtn = new QPushButton("📋 粘贴文本");
    clearBtn = new QPushButton("🗑️ 清空内容");
    QString lightBtn = R"(
        QPushButton {
            border: none;
            border-radius: 18px;
            padding: 9px 20px;
            min-height: 38px;
            background-color: #FFFFFF;
            color: #444;
        }
        QPushButton:hover { background-color: #F0F7FF; }
    )";
    selectImgBtn->setStyleSheet(lightBtn);
    pasteBtn->setStyleSheet(lightBtn);
    clearBtn->setStyleSheet(lightBtn);

    inputBtnLayout->addWidget(selectImgBtn);
    inputBtnLayout->addWidget(pasteBtn);
    inputBtnLayout->addWidget(clearBtn);
    inputBtnLayout->addStretch(1);
    inputBtnLayout->setSpacing(14);
    centerLayout->addLayout(inputBtnLayout);

    draftLabel = new QLabel("作文草稿:");
    draftLabel->setStyleSheet("font-weight: 500; color: #444; font-size:15px;");
    centerLayout->addWidget(draftLabel);

    draftEdit = new QTextEdit();
    draftEdit->setPlaceholderText("请输入作文草稿，或者点击上方按钮选择图片或粘贴文本...");
    draftEdit->setStyleSheet(R"(
        QTextEdit {
            border: none;
            border-radius: 12px;
            padding: 16px;
            background-color: #FFFFFF;
            min-height: 200px;
        }
        QTextEdit:focus { background-color: #F8F9FA; }
    )");
    centerLayout->addWidget(draftEdit);

    wordCountLabel = new QLabel("字数: 0");
    wordCountLabel->setStyleSheet("color:#888;");
    centerLayout->addWidget(wordCountLabel);

    funcBtnLayout = new QHBoxLayout();
    startAnalyzeBtn = new QPushButton("🚀 开始分析");
    cancelBtn = new QPushButton("❌ 取消");

    startAnalyzeBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            border-radius: 18px;
            padding: 11px 28px;
            min-height: 42px;
            background-color: #4A90E2;
            color: white;
            font-weight: 500;
        }
        QPushButton:hover { background-color: #3B82D6; }
    )");

    cancelBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            border-radius: 18px;
            padding: 11px 28px;
            min-height: 42px;
            background-color: #F27272;
            color: white;
            font-weight: 500;
        }
        QPushButton:hover { background-color: #F06060; }
    )");

    funcBtnLayout->addWidget(startAnalyzeBtn);
    funcBtnLayout->addWidget(cancelBtn);
    funcBtnLayout->addStretch(1);
    funcBtnLayout->setSpacing(18);
    centerLayout->addLayout(funcBtnLayout);

    // ---------- 结果标签页：批改 / 润色 / 学习计划 / AI 范文 ----------
    resultTab = new QTabWidget();
    resultTab->setStyleSheet(R"(
    QTabWidget { border: none; }
    QTabWidget::pane {
        border: none;
        border-radius: 12px;
        background-color: #FFFFFF;
        top: 0px;
    }
    QTabBar::tab {
        min-width: 110px;
        min-height: 38px;
        border: none;
        border-radius: 10px 10px 0 0;
        background-color: #F5F7FA;
        color: #666;
        margin: 0 3px;
    }
    QTabBar::tab:selected {
        background-color: #65B1F9;
        color: white;
    }
)");

    correctionEdit = new QTextEdit();
    correctionEdit->setReadOnly(true);
    correctionEdit->setPlaceholderText("批改结果将显示在这里...");
    correctionEdit->setStyleSheet("border:none; padding:16px;");

    polishedEdit = new QTextEdit();
    polishedEdit->setReadOnly(true);
    polishedEdit->setPlaceholderText("润色后的作文将显示在这里...");
    polishedEdit->setStyleSheet("border:none; padding:16px;");

    learningPlanEdit = new QTextEdit();
    learningPlanEdit->setReadOnly(true);
    learningPlanEdit->setPlaceholderText("学习计划将显示在这里...");
    learningPlanEdit->setStyleSheet("border:none; padding:16px;");

    sampleEssayEdit = new QTextEdit();
    sampleEssayEdit->setReadOnly(true);
    sampleEssayEdit->setPlaceholderText("AI生成范文将显示在这里...");
    sampleEssayEdit->setStyleSheet("border:none; padding:16px; font-size:15px;");

    resultTab->addTab(correctionEdit, "📝 批改结果");
    resultTab->addTab(polishedEdit, "✨ 润色版本");
    resultTab->addTab(learningPlanEdit, "📚 学习计划");
    resultTab->addTab(sampleEssayEdit, "📄 AI范文");

    resultTab->setTabPosition(QTabWidget::South);
    centerLayout->addWidget(resultTab);

    centerLayout->addStretch(1);
    mainLayout->addWidget(centerPanel);

    rightPanel = new QWidget(this);
    rightLayout = new QVBoxLayout(rightPanel);
    rightPanel->setFixedWidth(310);
    rightPanel->setStyleSheet(R"(
        QWidget {
            background-color: #F5F7FA;
        }
    )");
    rightLayout->setContentsMargins(22, 24, 22, 24);
    rightLayout->setSpacing(18);

    rightTitleLabel = new QLabel("📖 优秀万能句式", rightPanel);
    rightTitleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 17px;
            font-weight: 500;
            color: #2A2A2A;
        }
    )");
    rightLayout->addWidget(rightTitleLabel);

    expressionEdit = new QTextEdit(rightPanel);
    expressionEdit->setReadOnly(true);
    expressionEdit->setStyleSheet(R"(
        QTextEdit {
            border: none;
            border-radius: 12px;
            padding: 16px;
            background-color: #FFFFFF;
            line-height: 1.8;
            color: #333;
        }
    )");
    expressionEdit->setHtml(R"(
        <span style='color:#333'>
        1. <b style='color:#6495ED'>It goes without saying that</b> ... 不言而喻<br><br>
        2. <b style='color:#6495ED'>In view of this situation</b> ... 鉴于这种情况<br><br>
        3. <b style='color:#6495ED'>From what has been discussed above</b> ... 从以上讨论可知<br><br>
        4. <b style='color:#6495ED'>It is high time that we took</b> ... 是我们采取行动的时候<br><br>
        5. <b style='color:#6495ED'>Only in this way can we</b> ... 只有这样我们才能
        </span>
    )");
    rightLayout->addWidget(expressionEdit);

    copyrightLabel = new QLabel("© 2026 融研智笔·英语写作系列智能体\n智笔融研团队 版权所有", rightPanel);
    copyrightLabel->setStyleSheet(R"(
        QLabel {
            font-size: 13px;
            color: #AAAAAA;
            qproperty-alignment: AlignCenter;
        }
    )");
    rightLayout->addStretch(1);
    rightLayout->addWidget(copyrightLabel);

    mainLayout->addWidget(rightPanel);
}

// ---------- 信号槽绑定 ----------
void MainWindow::initConnect()
{
    connect(selectImgBtn,    &QPushButton::clicked, this, &MainWindow::selectImage);
    connect(pasteBtn,        &QPushButton::clicked, this, &MainWindow::pasteText);
    connect(clearBtn,        &QPushButton::clicked, this, &MainWindow::clearContent);
    connect(startAnalyzeBtn, &QPushButton::clicked, this, &MainWindow::startAnalyze);
    connect(cancelBtn,       &QPushButton::clicked, this, &MainWindow::cancelBtnClicked);

    // 字数实时统计
    connect(draftEdit, &QTextEdit::textChanged, this, &MainWindow::updateWordCount);

    // 忽略自签证书 SSL 错误（校内 HiAgent 常见）
    connect(m_reply, &QNetworkReply::sslErrors, this, [=](QList<QSslError> const& errors) {
        m_reply->ignoreSslErrors(errors);
    });
}

// ---------- 用户数据持久化（QSettings，随 exe 同级的 ini）----------
void MainWindow::loadUserData()
{
    totalEssayCount = settings->value("user/totalEssay", 0).toInt();
    currentScoreLevel = settings->value("user/scoreLevel", "初级").toString();
    weakPoints = settings->value("user/weakPoints").toStringList();
    QString lastExam = settings->value("user/lastExamType", "大学英语四级").toString();

    totalEssayLabel->setText(QString("总作文数：%1").arg(totalEssayCount));
    scoreLevelLabel->setText(QString("当前评分等级：%1").arg(currentScoreLevel));
    examTypeCombo->setCurrentText(lastExam);

    QString weakHtml;
    for(const QString& w : std::as_const(weakPoints)){
        weakHtml += QString("• <span style='color:red'>%1</span><br>").arg(w);
    }
    weakPointEdit->setHtml(weakHtml.isEmpty() ? "暂无薄弱点" : weakHtml);

    QVariantMap libMap = settings->value("user/sentenceLibrary").toMap();
    for (auto it = libMap.begin(); it != libMap.end(); ++it) {
        m_sentenceLib[it.key()] = it.value().toStringList();
    }
}

void MainWindow::saveUserData()
{
    settings->setValue("user/totalEssay", totalEssayCount);
    settings->setValue("user/scoreLevel", currentScoreLevel);
    settings->setValue("user/weakPoints", weakPoints);
    settings->setValue("user/lastExamType", examTypeCombo->currentText());

    QVariantMap libMap;
    for (auto it = m_sentenceLib.begin(); it != m_sentenceLib.end(); ++it) {
        libMap[it.key()] = it.value();
    }
    settings->setValue("user/sentenceLibrary", libMap);
    settings->sync();   // 立即落盘
}

// ---------- 左栏数据面板刷新 ----------
void MainWindow::updateDataPanel()
{
    totalEssayLabel->setText(QString("总作文数：%1").arg(totalEssayCount));
    scoreLevelLabel->setText(QString("当前评分等级：%1").arg(currentScoreLevel));
    knowledgeCountLabel->setText(QString("掌握知识点：%1个").arg(weakPoints.size()));

    QString weakHtml;
    for(const QString& w : std::as_const(weakPoints)){
        weakHtml += QString("• <span style='color:red'>%1</span><br>").arg(w);
    }
    weakPointEdit->setHtml(weakHtml.isEmpty() ? "暂无薄弱点" : weakHtml);
}

// ---------- 界面交互：图片 / 粘贴 / 清空 / 字数 ----------
void MainWindow::selectImage()
{
    m_selectedImagePath.clear();
    QString path = QFileDialog::getOpenFileName(this, "选择作文图片", "", "图片(*.png *.jpg *.jpeg *.bmp)");
    if(path.isEmpty()) return;
    m_selectedImagePath = path;
    draftEdit->setText("已选择图片：" + path + "\nHiAgent 将自动进行OCR文字识别");
}

void MainWindow::pasteText()
{
    QClipboard *clip = QApplication::clipboard();
    draftEdit->setText(clip->text());
}

void MainWindow::clearContent()
{
    draftEdit->clear();
    topicEdit->clear();
    correctionEdit->clear();
    polishedEdit->clear();
    learningPlanEdit->clear();
}

void MainWindow::updateWordCount()
{
    int cnt = draftEdit->toPlainText().trimmed().length();
    wordCountLabel->setText(QString("字数：%1").arg(cnt));
}

// ---------- 动态句式推荐：按薄弱点从句库匹配 ----------
void MainWindow::updateExpressionByScene()
{
    expressionEdit->clear();
    QString exampleHtml;

    for (const QString& weak : std::as_const(weakPoints)) {
        if (m_sentenceLib.contains(weak)) {
            exampleHtml += QString("<b>针对%1的高分替换例句：</b><br>").arg(weak);

            const QStringList& sentences = m_sentenceLib[weak];
            for (const QString& ex : std::as_const(sentences)) {
                exampleHtml += QString("• %1<br><br>").arg(ex);
            }
        }
    }

    expressionEdit->setHtml(exampleHtml.isEmpty() ? "暂无匹配例句" : exampleHtml);
}

// ---------- 分析入口：按输入状态分流到「批改」或「生成范文」----------
void MainWindow::startAnalyze()
{
    QString draft = draftEdit->toPlainText().trimmed();
    QString topic = topicEdit->text().trimmed();
    QString exam  = examTypeCombo->currentText();

    if(m_selectedImagePath.isEmpty() && draft.isEmpty() && topic.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入主题、作文，或选择图片！");
        return;
    }

    // 仅给了主题 → 走范文生成
    if(draft.isEmpty() && m_selectedImagePath.isEmpty() && !topic.isEmpty())
    {
        startAnalyzeBtn->setEnabled(false);
        startAnalyzeBtn->setText("生成范文中...");
        generateSampleEssay(topic, exam);
        return;
    }

    // 有草稿或图片 → 走批改分析
    requestHiagentAPI(draft, topic, exam);
}

// ---------- 批改结果解析：评分等级 / 薄弱点 / 句式库 / 三项输出 ----------
void MainWindow::parseHiagentResult(const QJsonObject &obj)
{
    currentScoreLevel = obj.value("scoreLevel").toString("初级");

    QJsonArray weakPointsArray = obj.value("weakPoints").toArray();
    weakPoints.clear();
    for (const QJsonValue &val : weakPointsArray) {
        if (!val.toString().isEmpty()) {
            weakPoints << val.toString();
        }
    }

    QJsonObject examplesObj = obj.value("sentenceExamples").toObject();
    for (auto it = examplesObj.begin(); it != examplesObj.end(); ++it) {
        QString weakName = it.key();
        QJsonArray sentenceArray = it.value().toArray();
        QStringList &sentences = m_sentenceLib[weakName];
        for (const QJsonValue &val : sentenceArray) {
            QString s = val.toString();
            if (!s.isEmpty() && !sentences.contains(s)) {
                sentences << s;
            }
        }
    }

    correctionEdit->setText(obj.value("correction").toString());
    polishedEdit->setText(obj.value("polished").toString());
    learningPlanEdit->setText(obj.value("learning_plan").toString());

    totalEssayCount++;
    updateDataPanel();
    updateExpressionByScene();
}

// ---------- 中止请求（只 abort，不关窗）----------
void MainWindow::cancelBtnClicked()
{
    if(m_reply)
    {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    startAnalyzeBtn->setEnabled(true);
    startAnalyzeBtn->setText("🚀 开始分析");

    QMessageBox::information(this, "操作提示", "已停止当前AI分析请求！");
}

// ---------- 范文生成结果解析 ----------
void MainWindow::parseSampleEssayResult(const QJsonObject &obj)
{
    QString essayContent = obj.value("content").toString();
    QString suggestion   = obj.value("suggestion").toString();

    sampleEssayEdit->setText(essayContent);
    correctionEdit->setText("✅ 状态：仅输入主题，已为您生成AI范文");
    learningPlanEdit->setText("📚 学习建议：\n" + suggestion);

    resultTab->setCurrentIndex(3);   // 切到「AI范文」标签页

    totalEssayCount++;
    updateDataPanel();
}

// ---------- HiAgent 同步工作流：统一 POST + 三级输出兜底 ----------
void MainWindow::callSyncWorkflow(const QJsonObject &inputs,
                                  std::function<void(const QJsonObject&)> callback)
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    QUrl url("https://hiagent.shengda.edu.cn/api/proxy/api/v1/sync_run_app_workflow");
    QNetworkRequest req(url);
    req.setRawHeader("Apikey", "YOUR_HIAGENT_API_KEY");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json;charset=UTF-8");

    // 忽略 SSL 证书验证（校内自签常见）
    QSslConfiguration sslConfig = req.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(sslConfig);

    QJsonObject body;
    body["UserID"] = m_currentUserID;
    body["AppKey"] = "YOUR_HIAGENT_API_KEY";
    body["Inputs"] = inputs;

    QNetworkReply *reply = networkManager->post(req, QJsonDocument(body).toJson());
    m_reply = reply;   // 供「取消」按钮 abort

    connect(reply, &QNetworkReply::finished, this, [this, callback, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, "错误", "请求失败：" + reply->errorString());
            startAnalyzeBtn->setEnabled(true);
            startAnalyzeBtn->setText("🚀 开始分析");
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject respObj = doc.object();

        if (respObj.contains("ResponseMetadata") && respObj["ResponseMetadata"].toObject().contains("Error")) {
            QString errMsg = respObj["ResponseMetadata"].toObject()["Error"].toObject()["Message"].toString();
            QMessageBox::warning(this, "工作流错误", errMsg.isEmpty() ? "未知错误" : errMsg);
            startAnalyzeBtn->setEnabled(true);
            startAnalyzeBtn->setText("🚀 开始分析");
            return;
        }

        // 不同部署版本输出位置不一致，三级兜底
        QJsonObject outputs;
        if (respObj.contains("Outputs") && respObj["Outputs"].isObject()) {
            outputs = respObj["Outputs"].toObject();
        } else if (respObj.contains("Data") && respObj["Data"].isObject()) {
            outputs = respObj["Data"].toObject();
        } else {
            outputs = respObj;
        }

        if (outputs.isEmpty()) {
            QMessageBox::warning(this, "错误", "工作流未返回有效数据");
            startAnalyzeBtn->setEnabled(true);
            startAnalyzeBtn->setText("🚀 开始分析");
            return;
        }

        callback(outputs);
        startAnalyzeBtn->setEnabled(true);
        startAnalyzeBtn->setText("🚀 开始分析");
    });
}

// ---------- 批改请求构造 ----------
void MainWindow::requestHiagentAPI(const QString &draft, const QString &topic, const QString &examType)
{
    startAnalyzeBtn->setEnabled(false);
    startAnalyzeBtn->setText("分析中...");

    // 变量名必须与 HiAgent 工作流中定义的一致
    QJsonObject inputs;
    inputs["exam_type"] = examType;
    inputs["topic"] = topic;
    inputs["draft_text"] = draft;
    if (!m_selectedImagePath.isEmpty()) {
        inputs["image_path"] = m_selectedImagePath;
    }

    callSyncWorkflow(inputs, [this](const QJsonObject &result) {
        parseHiagentResult(result);
    });

    m_selectedImagePath.clear();
}

// ---------- 范文请求构造 ----------
void MainWindow::generateSampleEssay(const QString& topic, const QString& examType)
{
    startAnalyzeBtn->setEnabled(false);
    startAnalyzeBtn->setText("生成范文中...");

    QJsonObject inputs;
    inputs["exam_type"] = examType;
    inputs["topic"] = topic;
    inputs["mode"] = "sample_only";

    callSyncWorkflow(inputs, [this](const QJsonObject &result) {
        parseSampleEssayResult(result);
    });
}
