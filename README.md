<div align="right">

**[English](README.md)** · [简体中文](README.zh-CN.md)

</div>

# rongyan

English writing feedback, as a web page and as a Qt desktop app.

<sub>HTML · C++ / Qt · scoring criteria for seven exam types</sub>

---

## What it does

A user pastes an essay; the tool returns feedback against the scoring rubric of a chosen exam type. Seven types are covered — CET-4, CET-6, postgraduate entrance, IELTS, TOEFL, TEM and general.

Two front ends share one rubric definition:

| | Web | Desktop |
|---|---|---|
| Stack | single HTML file | Qt, ~1,300 lines |
| Layout | tabs, sidebar | three-column |
| Storage | `localStorage` | `QSettings` |
| Network | fetch | `QNetworkAccessManager` |

## The web page

Single file, no build step. What it actually does:

- **Exam-type switching** — each type loads its own rubric: word-count band, scoring dimensions, weighting
- **Live word count** — recomputed on input, colour-coded against the current type's target band
- **Sentence-pattern library** — a collapsible list of usable constructions, filterable by function
- **Local history** — essays and results persist in `localStorage`, no account needed

The rubric data is a plain object keyed by exam type, so adding an eighth exam means adding one entry.

## The desktop app

Qt 6, three-column layout: navigation, editor, feedback panel.

Three things in it are worth pointing at:

**State persistence through `QSettings`** — window geometry, last selected exam type, panel widths survive restart. Small detail, large difference in daily use.

**Network requests through `QNetworkAccessManager`** — asynchronous, with a progress state and a cancellation path. Synchronous requests would freeze the window.

**Column widths are user-adjustable and remembered** — `QSplitter` with sizes written back on close.

## Layout

```
index.html                     the web page
qt-desktop/
├── mainwindow.cpp             ~740 lines after cleanup
├── mainwindow.h
├── mainwindow.ui
└── RongyanWisdomPen.pro
融研智笔·英语写作智能体.docx       design document
融研智笔·英语写作智能体_决赛答辩汇报.pptx   final-round deck
```

## Building the desktop app

```bash
cd qt-desktop
qmake RongyanWisdomPen.pro
make            # or mingw32-make / nmake
```

Requires Qt 6 with Widgets and Network.

## Notes

The desktop source had a hard-coded API endpoint and credential from development; both were replaced with placeholders before publishing. Set your own in `mainwindow.cpp`.

About 500 lines of commented-out dead code were removed during cleanup; all twenty functions were retained and verified.

## License

MIT — see [LICENSE](LICENSE).
