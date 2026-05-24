#ifndef NATIVE_WEB_VIEW_H
#define NATIVE_WEB_VIEW_H

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWidget>
#include <mutex>
#include <string>

#include "wke.h"

class NativeWebView : public QWidget {
    Q_OBJECT
public:
    explicit NativeWebView(const QString& url, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_url(url)
        , m_view(nullptr)
        , m_initialized(false)
        , m_miniblinkReady(false)
        , m_updateTimer(new QTimer(this))
    {
        // 核心一：给 Qt 打预防针，强行霸占此区域的绘制权
        this->setAttribute(Qt::WA_NativeWindow, true);       // 确保拥有独立的原生 Win32 句柄
        this->setAttribute(Qt::WA_OpaquePaintEvent, true);   // 告诉 Qt 我自己画所有像素，你别帮我擦除
        this->setAttribute(Qt::WA_NoSystemBackground, true); // 禁用系统默认背景绘制，防止白屏覆盖
        this->setStyleSheet("background-color: #202124;");  // 诊断：确保容器可见
        
        // 让组件具备自适应拉伸能力
        this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        m_updateTimer->setInterval(16);
        connect(m_updateTimer, &QTimer::timeout, this, []() {
            wkeUpdate();
        });
    }

    ~NativeWebView() {
        if (m_updateTimer) {
            m_updateTimer->stop();
        }
        if (m_view) {
            wkeDestroyWebView(m_view);
        }
    }

protected:
    // 核心二：改在 showEvent 中进行延迟初始化
    // 此时 Widget 已经被完全放入了 Qt 的布局层次中，拿到的 winId() 是绝对稳定且最终的
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);
        
        if (!m_initialized) {
            m_miniblinkReady = initMiniblinkOnce();

            if (!m_miniblinkReady) {
                m_initialized = true;
                return;
            }

            HWND hwnd = reinterpret_cast<HWND>(this->winId());
            qDebug("Miniblink parent HWND: %p", hwnd);

            // 【关键修复 1】：强制告诉 Windows 系统，Qt 不要在子窗口（浏览器）的区域内乱画
            LONG style = GetWindowLongW(hwnd, GWL_STYLE);
            SetWindowLongW(hwnd, GWL_STYLE, style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);

            m_view = wkeCreateWebWindow(WKE_WINDOW_TYPE_CONTROL, hwnd, 0, 0, this->width(), this->height());
            wkeSetTransparent(m_view, false);
            wkeOnLoadingFinish(m_view, &NativeWebView::onLoadingFinish, this);
            wkeOnTitleChanged(m_view, &NativeWebView::onTitleChanged, this);
            wkeLoadURL(m_view, m_url.toUtf8().constData());
            
            wkeShowWindow(m_view, true); 

            HWND childHwnd = wkeGetWindowHandle(m_view);
            qDebug("Miniblink child HWND: %p", childHwnd);

            m_updateTimer->start();
            m_initialized = true;
        }
    }

    // 当 Qt 布局改变导致当前 Widget 大小变化时，同步通知 Miniblink 调整分辨率
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        // 只有初始化完成后才允许调整尺寸，防止在 showEvent 之前触发导致空指针崩溃
        if (m_initialized && m_view) {
            wkeResizeWindow(m_view, this->width(), this->height());
        }
    }

private:
    QString m_url;
    wkeWebView m_view;
    bool m_initialized;
    bool m_miniblinkReady;
    QTimer* m_updateTimer;

    static void onLoadingFinish(wkeWebView webView, void* param, const wkeString url,
                                wkeLoadingResult result, const wkeString failedReason) {
        Q_UNUSED(webView);
        Q_UNUSED(param);
        const char* urlStr = url ? wkeGetString(url) : "";
        const char* reasonStr = failedReason ? wkeGetString(failedReason) : "";
        qDebug("Miniblink load finished: url=%s result=%d reason=%s", urlStr, result, reasonStr);
    }

    static void onTitleChanged(wkeWebView webView, void* param, const wkeString title) {
        Q_UNUSED(webView);
        Q_UNUSED(param);
        const char* titleStr = title ? wkeGetString(title) : "";
        qDebug("Miniblink title changed: %s", titleStr);
    }

    static bool initMiniblinkOnce() {
        static std::once_flag once;
        static bool ready = false;
        static std::wstring dllPath;
        std::call_once(once, []() {
            QString path = QCoreApplication::applicationDirPath() + "/node.dll";
            if (!QFileInfo::exists(path)) {
                qWarning("Miniblink node.dll not found: %s", qPrintable(path));
                return;
            }

            dllPath = QDir::toNativeSeparators(path).toStdWString();
            HMODULE testHandle = LoadLibraryW(dllPath.c_str());
            if (!testHandle) {
                qWarning("Miniblink node.dll failed to load: %s", qPrintable(path));
                return;
            }
            FARPROC initFunc = GetProcAddress(testHandle, "wkeInitializeEx");
            FreeLibrary(testHandle);
            if (!initFunc) {
                qWarning("Miniblink node.dll missing wkeInitializeEx: %s", qPrintable(path));
                return;
            }

            wkeSetWkeDllPath(dllPath.c_str());
            wkeInit();
                qDebug("Miniblink initialized: %s", qPrintable(path));
                ready = true;
        });
        return ready;
    }
};

#endif // NATIVE_WEB_VIEW_H