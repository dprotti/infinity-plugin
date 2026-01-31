#include "ui.h"
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

namespace {

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
		display_notify_resize(event->size().width(), event->size().height());
		QWidget::resizeEvent(event);
	}

	void showEvent(QShowEvent *event) override {
		display_notify_visibility(TRUE);
		QWidget::showEvent(event);
	}

	void hideEvent(QHideEvent *event) override {
		display_notify_visibility(FALSE);
		QWidget::hideEvent(event);
	}

	void closeEvent(QCloseEvent *event) override {
		display_notify_close();
		QWidget::closeEvent(event);
	}

	void keyPressEvent(QKeyEvent *event) override {
		switch (event->key()) {
		case Qt::Key_Right:
			infinity_queue_key(INFINITY_KEY_RIGHT);
			break;
		case Qt::Key_Left:
			infinity_queue_key(INFINITY_KEY_LEFT);
			break;
		case Qt::Key_Up:
			infinity_queue_key(INFINITY_KEY_UP);
			break;
		case Qt::Key_Down:
			infinity_queue_key(INFINITY_KEY_DOWN);
			break;
		case Qt::Key_Z:
			infinity_queue_key(INFINITY_KEY_PREV);
			break;
		case Qt::Key_X:
			infinity_queue_key(INFINITY_KEY_PLAY);
			break;
		case Qt::Key_C:
			infinity_queue_key(INFINITY_KEY_PAUSE);
			break;
		case Qt::Key_V:
			infinity_queue_key(INFINITY_KEY_STOP);
			break;
		case Qt::Key_B:
			infinity_queue_key(INFINITY_KEY_NEXT);
			break;
		case Qt::Key_F11:
			infinity_queue_key(INFINITY_KEY_FULLSCREEN);
			break;
		case Qt::Key_Escape:
			infinity_queue_key(INFINITY_KEY_EXIT_FULLSCREEN);
			break;
		case Qt::Key_F12:
			infinity_queue_key(INFINITY_KEY_NEXT_PALETTE);
			break;
		case Qt::Key_Space:
			infinity_queue_key(INFINITY_KEY_NEXT_EFFECT);
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			infinity_queue_key(INFINITY_KEY_TOGGLE_INTERACTIVE);
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

} // namespace

gboolean ui_init(gint32 width, gint32 height)
{
	if (QApplication::instance() == nullptr) {
		return FALSE;
	}

	if (window_instance != nullptr) {
		return TRUE;
	}

	window_instance = new InfinityWindow();
	window_instance->resize(width, height);
	window_instance->show();
	window_instance->raise();
	return TRUE;
}

void ui_quit(void)
{
	if (window_instance == nullptr) {
		return;
	}
	window_instance->close();
	delete window_instance;
	window_instance = nullptr;
}

void ui_present(const guint16 *pixels, gint32 width, gint32 height)
{
	if (window_instance == nullptr) {
		return;
	}
	window_instance->update_frame(pixels, width, height);
}

void ui_resize(gint32 width, gint32 height)
{
	if (window_instance == nullptr) {
		return;
	}
	window_instance->resize(width, height);
}

void ui_toggle_fullscreen(void)
{
	if (window_instance == nullptr) {
		return;
	}
	window_instance->set_fullscreen(!window_instance->is_fullscreen());
}

void ui_exit_fullscreen_if_needed(void)
{
	if (window_instance == nullptr) {
		return;
	}
	if (window_instance->is_fullscreen()) {
		window_instance->set_fullscreen(false);
	}
}
