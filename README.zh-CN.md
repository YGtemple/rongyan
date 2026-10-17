<div align="right">

[English](README.md) · **[简体中文](README.zh-CN.md)**

</div>

# rongyan

英语写作批改，网页端和 Qt 桌面端两套前端。

<sub>HTML · C++ / Qt · 七类考试的评分标准</sub>

---

## 做什么

用户粘贴一篇作文，工具按所选考试类型的评分标准给反馈。覆盖七类——四级、六级、考研、雅思、托福、专四专八、通用。

两套前端共用同一份评分标准定义：

| | 网页 | 桌面 |
|---|---|---|
| 技术栈 | 单 HTML 文件 | Qt，约 1,300 行 |
| 布局 | 标签页 + 侧栏 | 三栏 |
| 存储 | `localStorage` | `QSettings` |
| 网络 | fetch | `QNetworkAccessManager` |

## 网页端

单文件，不需要构建。实际做的事：

- **考试类型切换**——每种类型加载自己的标准：字数区间、评分维度、权重
- **实时字数统计**——输入即算，按当前类型的字数区间着色
- **句式库**——可折叠的可用句式列表，按功能筛选
- **本地历史**——作文和结果存在 `localStorage`，不需要账号

评分数据是一个按考试类型建键的普通对象，加第八种考试就意味着加一条。

## 桌面端

Qt 6，三栏布局：导航、编辑区、反馈面板。

三处值得看：

**用 `QSettings` 做状态持久化**——窗口几何、上次选中的考试类型、分栏宽度都能挺过重启。细节很小，日常体验差别很大。

**用 `QNetworkAccessManager` 做网络请求**——异步，有进度状态和取消路径。同步请求会把窗口卡住。

**分栏宽度可调且会被记住**——`QSplitter`，关闭时把尺寸写回。

## 目录

```
index.html                     网页
qt-desktop/
├── mainwindow.cpp             清理后约 740 行
├── mainwindow.h
├── mainwindow.ui
└── RongyanWisdomPen.pro
融研智笔·英语写作智能体.docx       设计文档
融研智笔·英语写作智能体_决赛答辩汇报.pptx  决赛答辩
```

## 构建桌面端

```bash
cd qt-desktop
qmake RongyanWisdomPen.pro
make            # 或 mingw32-make / nmake
```

需要 Qt 6，含 Widgets 和 Network 模块。

## 说明

桌面端源码里原本硬编码了开发时的 API 端点和凭据；发布前都已替换为占位符，请在 `mainwindow.cpp` 里填入自己的。

清理过程中删掉了约 500 行被注释掉的死代码，二十个函数全部保留并核对过。

## 许可证

MIT —— 见 [LICENSE](LICENSE)。
