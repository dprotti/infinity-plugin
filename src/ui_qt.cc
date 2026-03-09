
#include "ui.h"
#include "infinity.h"
#include "input.h"

#include <QApplication>
#include <QCloseEvent>
#include <QHideEvent>
#include <QImage>
#include <QKeyEvent>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QResizeEvent>
#include <QShowEvent>
#include <QWidget>
#include <QtGlobal>
#include <QEventLoop>

#include <cassert>
#include <memory>

namespace {

DisplayCallbacks g_display_callbacks = {};

class InfinityWindow final : public QWidget {
public:
    InfinityWindow() {
        setWindowTitle(QStringLiteral("Infinity"));
        setMinimumSize(200, 150);
        setFocusPolicy(Qt::StrongFocus);
    }

    void update_frame(const guint16 *pixels, gint32 width, gint32 height) {
        if (pixels == nullptr || width <= 0 || height <= 0) {
            return;
        }

        QImage frame(reinterpret_cast<const uchar *>(pixels), width, height, QImage::Format_RGB16);
        {
            QMutexLocker locker(&frame_mutex_);
            frame_ = frame.copy();
        }
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }

    void set_fullscreen(bool enabled) {
        if (enabled) {
            showFullScreen();
        } else {
            showNormal();
        }
    }

    bool is_fullscreen() const {
        return isFullScreen();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        QImage frame_copy;
        {
            QMutexLocker locker(&frame_mutex_);
            frame_copy = frame_;
        }
        if (!frame_copy.isNull()) {
            painter.drawImage(rect(), frame_copy);
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        const qreal ratio = devicePixelRatioF();
        const gint32 pixel_width = qRound(event->size().width() * ratio);
        const gint32 pixel_height = qRound(event->size().height() * ratio);
        if (g_display_callbacks.notify_resize != nullptr) {
            g_display_callbacks.notify_resize(pixel_width, pixel_height);
        }
        QWidget::resizeEvent(event);
    }

    void showEvent(QShowEvent *event) override {
        if (g_display_callbacks.notify_visibility != nullptr) {
            g_display_callbacks.notify_visibility(TRUE);
        }
        QWidget::showEvent(event);
    }

    void hideEvent(QHideEvent *event) override {
        if (g_display_callbacks.notify_visibility != nullptr) {
            g_display_callbacks.notify_visibility(FALSE);
        }
        QWidget::hideEvent(event);
    }

    void closeEvent(QCloseEvent *event) override {
        if (g_display_callbacks.notify_close != nullptr) {
            g_display_callbacks.notify_close();
        }
        QWidget::closeEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
        case Qt::Key_Right:
            Infinity::queue_key(INFINITY_KEY_RIGHT);
            break;
        case Qt::Key_Left:
            Infinity::queue_key(INFINITY_KEY_LEFT);
            break;
        case Qt::Key_Up:
            Infinity::queue_key(INFINITY_KEY_UP);
            break;
        case Qt::Key_Down:
            Infinity::queue_key(INFINITY_KEY_DOWN);
            break;
        case Qt::Key_Z:
            Infinity::queue_key(INFINITY_KEY_PREV);
            break;
        case Qt::Key_X:
            Infinity::queue_key(INFINITY_KEY_PLAY);
            break;
        case Qt::Key_C:
            Infinity::queue_key(INFINITY_KEY_PAUSE);
            break;
        case Qt::Key_V:
            Infinity::queue_key(INFINITY_KEY_STOP);
            break;
        case Qt::Key_B:
            Infinity::queue_key(INFINITY_KEY_NEXT);
            break;
        case Qt::Key_F11:
            Infinity::queue_key(INFINITY_KEY_FULLSCREEN);
            break;
        case Qt::Key_Escape:
            Infinity::queue_key(INFINITY_KEY_EXIT_FULLSCREEN);
            break;
        case Qt::Key_F12:
            Infinity::queue_key(INFINITY_KEY_NEXT_PALETTE);
            break;
        case Qt::Key_Space:
            Infinity::queue_key(INFINITY_KEY_NEXT_EFFECT);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            Infinity::queue_key(INFINITY_KEY_TOGGLE_INTERACTIVE);
            break;
        default:
            break;
        }
        QWidget::keyPressEvent(event);
    }

private:
    QMutex frame_mutex_;
    QImage frame_;
};

InfinityWindow *window_instance = nullptr;
std::unique_ptr<QApplication> app_instance;

void ensure_app_instance() {
    if (QApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char app_name[] = "infinity";
    static char *argv[] = {app_name, nullptr};
    app_instance = std::make_unique<QApplication>(argc, argv);
}

void process_events() {
    if (QApplication::instance() == nullptr) {
        return;
    }
    QApplication::processEvents(QEventLoop::AllEvents, 1);
}

} // namespace

gboolean ui_init(gint32 width, gint32 height, const DisplayCallbacks &callbacks) {
    ensure_app_instance();
    if (QApplication::instance() == nullptr) {
        return FALSE;
    }

    if (window_instance != nullptr) {
        return TRUE;
    }

    assert(callbacks.notify_close);
    assert(callbacks.notify_resize);
    assert(callbacks.notify_visibility);
    assert(callbacks.queue_key);
    g_display_callbacks = callbacks;

    window_instance = new InfinityWindow();
    window_instance->resize(width, height);
    window_instance->show();
    window_instance->raise();
    process_events();
    return TRUE;
}

void ui_quit(void) {
    if (window_instance == nullptr) {
        return;
    }
    window_instance->close();
    delete window_instance;
    window_instance = nullptr;
}

void ui_present(const guint16 *pixels, gint32 width, gint32 height) {
    if (window_instance == nullptr) {
        return;
    }
    window_instance->update_frame(pixels, width, height);
    process_events();
}

void ui_resize(gint32 width, gint32 height) {
    if (window_instance == nullptr) {
        return;
    }
    const qreal ratio = window_instance->devicePixelRatioF();
    const gint32 logical_width = qRound(width / ratio);
    const gint32 logical_height = qRound(height / ratio);
    window_instance->resize(logical_width, logical_height);
    process_events();
}

void ui_toggle_fullscreen(void) {
    if (window_instance == nullptr) {
        return;
    }
    window_instance->set_fullscreen(!window_instance->is_fullscreen());
    process_events();
    const qreal ratio = window_instance->devicePixelRatioF();
    if (g_display_callbacks.notify_resize != nullptr) {
        g_display_callbacks.notify_resize(
            qRound(window_instance->width() * ratio), qRound(window_instance->height() * ratio));
    }
}

void ui_exit_fullscreen_if_needed(void) {
    if (window_instance == nullptr) {
        return;
    }
    if (window_instance->is_fullscreen()) {
        window_instance->set_fullscreen(false);
        process_events();
        const qreal ratio = window_instance->devicePixelRatioF();
        if (g_display_callbacks.notify_resize != nullptr) {
            g_display_callbacks.notify_resize(
                qRound(window_instance->width() * ratio), qRound(window_instance->height() * ratio));
        }
    }
}
