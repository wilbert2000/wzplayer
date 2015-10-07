/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gui/autohidewidget.h"
#include <QTimer>
#include <QEvent>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>

#if QT_VERSION >= 0x040600
#include <QPropertyAnimation>
#endif

namespace Gui {

TAutohideWidget::TAutohideWidget(QWidget* parent, QWidget* playerwindow)
	: QWidget(parent)
	, turned_on(false)
	, auto_hide(false)
	, use_animation(false)
	, spacing(0)
	, perc_width(100)
	, activation_area(Anywhere)
	, internal_widget(0)
	, timer(0)
#if QT_VERSION >= 0x040600
	, animation(0)
#endif
{
	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);
	setLayoutDirection(Qt::LeftToRight);

	playerwindow->installEventFilter(this);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(checkUnderMouse()));
	timer->setInterval(3000);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->setSpacing(0);
	layout->setMargin(0);
	setLayout(layout);
}

TAutohideWidget::~TAutohideWidget() {
#if QT_VERSION >= 0x040600
	if (animation) delete animation;
#endif
}

void TAutohideWidget::setInternalWidget(QWidget* w) {
	layout()->addWidget(w);
	internal_widget = w;
}

void TAutohideWidget::setHideDelay(int ms) {
	timer->setInterval(ms);
}

int TAutohideWidget::hideDelay() {
	return timer->interval();
}

void TAutohideWidget::activate() {
	turned_on = true;
	timer->start();
}

void TAutohideWidget::deactivate() {
	turned_on = false;
	timer->stop();
	hide();
}

void TAutohideWidget::show() {
	qDebug() << "Gui::TAutohideWidget::show";
	resizeAndMove();

	if (use_animation) {
		showAnimated();
	} else {
		QWidget::show();
	}

	// Restart timer
	if (timer->isActive()) timer->start();
}

void TAutohideWidget::setAutoHide(bool b) {
	auto_hide = b;
}

void TAutohideWidget::checkUnderMouse() {
	if (auto_hide && isVisible() && !underMouse()) {
		hide();
	}
}

void TAutohideWidget::resizeAndMove() {
	QWidget* widget = parentWidget();
	int w = widget->width() * perc_width / 100;
	int h = height();
	resize(w, h);

	int x = (widget->width() - width()) / 2;
	int y = widget->height() - height() - spacing;
	move(x, y);
}

bool TAutohideWidget::eventFilter(QObject* obj, QEvent* event) {
	if (turned_on) {
		if (event->type() == QEvent::MouseMove) {
			if (!isVisible()) {
				if (activation_area == Anywhere) {
					show();
				} else {
					QMouseEvent* mouse_event = dynamic_cast<QMouseEvent*>(event);
					QWidget* parent = parentWidget();
					QPoint p = parent->mapFromGlobal(mouse_event->globalPos());
					if (p.y() > (parent->height() - height() - spacing)) {
						show();
					}
				}
			}
		}
	}

	return QWidget::eventFilter(obj, event);
}

void TAutohideWidget::showAnimated() {
#if QT_VERSION >= 0x040600
	if (!animation) {
		animation = new QPropertyAnimation(this, "pos");
	}

	QPoint initial_position = QPoint(pos().x(), parentWidget()->size().height());
	QPoint final_position = pos();
	move(initial_position);

	QWidget::show();

	animation->setDuration(300);
	animation->setEasingCurve(QEasingCurve::OutBounce);
	animation->setEndValue(final_position);
	animation->setStartValue(initial_position);
	animation->start();
#else
	QWidget::show();
#endif
}

} // namespace Gui

#include "moc_autohidewidget.cpp"

