#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QStringList>
#include<QNetworkReply>

#include <QGraphicsDropShadowEffect>  // 卡片阴影特效（Qt6 安全写法）
#include <QLinearGradient>            // 按钮渐变

// 精简前置声明（无冗余，加速编译）
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QTextEdit;
class QLineEdit;
class QComboBox;
class QTabWidget;
class QGroupBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // ===== 界面功能 =====
    void selectImage();                       // 选择作文图片
    void pasteText();                         // 从剪贴板粘贴文本
    void clearContent();                      // 清空输入区
    void updateWordCount();                   // 实时字数统计
    void startAnalyze();                      // 开始分析（按输入状态分流）
    void cancelBtnClicked();                  // 中止当前网络请求

    // ===== 网络请求 =====
    void requestHiagentAPI(const QString &draft, const QString &topic, const QString &examType);
    void parseHiagentResult(const QJsonObject &result);
    void generateSampleEssay(const QString& topic, const QString& examType);
    void parseSampleEssayResult(const QJsonObject &result);

protected:
    // void closeEvent(QCloseEvent *event) override;   // 旧版：关闭时保存用户数据

private:
    // ===== 初始化分块 =====
    void initSentenceLibrary();    // 句式库（清空 + 从 QSettings 加载历史）
    void initUI();                 // 三栏布局搭建
    void initConnect();            // 信号槽绑定

    // ===== 用户数据持久化 =====
    void loadUserData();
    void saveUserData();
    void updateDataPanel();

    // ===== 通用同步工作流调用 =====
    // 一次调用 = 一次 POST；回调里按 Outputs / Data / 顶层三级兜底提取结果
    void callSyncWorkflow(const QJsonObject &inputs,
                          std::function<void(const QJsonObject&)> callback);

    // ===== 动态句式推荐 =====
    void updateExpressionByScene();

    // ===== 业务状态 =====
    QString m_currentUserID = "user_local_001";   // HiAgent 侧固定用户标识（1–20 字符）

    // 句式库数据
    QMap<QString, QStringList> levelSentences;    // 按水平分级
    QMap<QString, QStringList> examSentences;     // 按考试类型分级
    QHash<QString,QStringList> m_sentenceLib;     // 薄弱点 → 高分替换例句

    // ===== 左栏：学习数据分析 =====
    QWidget *leftPanel;
    QVBoxLayout *leftLayout;
    QLabel *leftTitleLabel;
    QGroupBox *dataGroup;
    QLabel *totalEssayLabel;
    QLabel *scoreLevelLabel;
    QLabel *knowledgeCountLabel;
    QLabel *weakPointTitle;
    QTextEdit *weakPointEdit;
    QTextEdit *sampleEssayEdit;

    // ===== 中栏：核心工作区 =====
    QWidget *centerPanel;
    QVBoxLayout *centerLayout;
    QLabel *centerTitleLabel;
    QHBoxLayout *examTypeLayout;
    QLabel *examTypeLabel;
    QComboBox *examTypeCombo;
    QHBoxLayout *topicLayout;
    QLabel *topicLabel;
    QLineEdit *topicEdit;
    QHBoxLayout *inputBtnLayout;
    QPushButton *selectImgBtn;
    QPushButton *pasteBtn;
    QPushButton *clearBtn;
    QLabel *draftLabel;
    QTextEdit *draftEdit;
    QLabel *wordCountLabel;
    QHBoxLayout *funcBtnLayout;
    QPushButton *startAnalyzeBtn;
    QPushButton *cancelBtn;
    QTabWidget *resultTab;
    QTextEdit *correctionEdit;
    QTextEdit *polishedEdit;
    QTextEdit *learningPlanEdit;

    // ===== 右栏：优秀万能句式 =====
    QWidget *rightPanel;
    QVBoxLayout *rightLayout;
    QLabel *rightTitleLabel;
    QTextEdit *expressionEdit;
    QLabel *copyrightLabel;

    // ===== 业务变量 =====
    QSettings *settings;
    QNetworkAccessManager *networkManager;   // 全类唯一实例
    QNetworkReply *m_reply = nullptr;         // 供「取消」按钮 abort
    int totalEssayCount;
    QStringList weakPoints;
    QString currentScoreLevel;
    QString m_selectedImagePath;              // 仅存路径，OCR 交由服务端
    QNetworkReply *m_currentReply;
};

#endif // MAINWINDOW_H
