/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>
    umplayer, Copyright (C) 2010 Ori Rejwan

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

#include "gui/skin/mediapanel.h"

#include <Qt>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QTimerEvent>
#include <QGridLayout>
#include <QLabel>
#include <QHelpEvent>
#include <QToolTip>
#include <QDebug>

#include "config.h"
#include "gui/action/widgetactions.h"
#include "gui/skin/actiontools.h"
#include "gui/skin/iconsetter.h"
#include "core.h"

namespace Gui {
namespace Skin {

TMediaPanel::TMediaPanel(QWidget* parent, int pos_max) :
	QWidget(parent),
	duration(0) {

	setupUi(this);
	setAttribute(Qt::WA_StyledBackground, true);
	setMinimumWidth(270);

	if (fontInfo().pixelSize() > 12) {
		QFont f = font();
		f.setPixelSize(12);
		setFont(f);
	}

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mediaLabel = new TScrollingLabel(this);
	resolutionLabel = new QLabel(this);
	resolutionLabel->setObjectName("panel-resolution");
	resolutionLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	repeatButton = new TButton(this);
	shuffleButton = new TButton(this);
	seeker = new TPanelTimeSeeker;
	seeker->setObjectName("panel-seeker");
	seeker->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
	seeker->setRange(0, pos_max);
	seeker->installEventFilter(this);
	mediaLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	mediaLabel->setObjectName("panel-main-label");
	layout = new QGridLayout;
	elapsedLabel = new QLabel(this);
	elapsedLabel->setObjectName("panel-elapsed-label");
	elapsedLabel->setMargin(0);
	elapsedLabel->setAlignment((Qt::Alignment) Qt::AlignHCenter | Qt::AlignTop);
	elapsedLabel->setIndent(3);
	elapsedLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	totalLabel = new QLabel(this);
	totalLabel->setObjectName("panel-total-label");
	totalLabel->setMargin(0);
	totalLabel->setAlignment((Qt::Alignment) Qt::AlignHCenter | Qt::AlignTop);
	totalLabel->setIndent(3);
	/*
	layout->addWidget(mediaLabel, 0, 0, 1, 2);
	layout->addWidget(resolutionLabel, 0, 2, 1, 1);
	layout->addWidget(repeatButton, 0, 3 );
	layout->addWidget(shuffleButton, 0, 4 );
	layout->addWidget(elapsedLabel, 1, 0, 1, 1);
	layout->addWidget(seeker, 1, 1, 1, 2);
	layout->addWidget(totalLabel, 1, 3, 1, 2);
	*/
	rearrangeWidgets(false);
	layout->setSpacing(0);
	layout->setContentsMargins(8,3,8, 3);
	elapsedLabel->setText("00:00:00");
	totalLabel->setText("00:00:00");
	//resolutionLabel->setText("1920x1024");
	//resolutionLabel->hide();
	setLayout(layout);
	timer = new QTimer(this);
	timer->setSingleShot(true);
	timer->setInterval(2000);
	connect(timer, SIGNAL(timeout()), this, SLOT(reverseStatus()));
	connect(seeker, SIGNAL(valueChanged(int)), this, SIGNAL(seekerChanged(int)));
	connect(seeker, SIGNAL(wheelUp()), this, SIGNAL(seekerWheelUp()));
	connect(seeker, SIGNAL(wheelDown()), this, SIGNAL(seekerWheelDown()));
}

TMediaPanel::~TMediaPanel() {
}

void TMediaPanel::rearrangeWidgets(bool resolution_visible) {
	if (resolution_visible) {
		layout->addWidget(mediaLabel, 0, 0, 1, 2);
		layout->addWidget(resolutionLabel, 0, 2, 1, 1);
		layout->addWidget(repeatButton, 0, 3 );
		layout->addWidget(shuffleButton, 0, 4 );
		layout->addWidget(elapsedLabel, 1, 0, 1, 1);
		layout->addWidget(seeker, 1, 1, 1, 2);
		layout->addWidget(totalLabel, 1, 3, 1, 2);
		resolutionLabel->setVisible(true);
	} else {
		layout->addWidget(mediaLabel, 0, 0, 1, 2);
		layout->addWidget(repeatButton, 0, 2 );
		layout->addWidget(shuffleButton, 0, 3 );
		layout->addWidget(elapsedLabel, 1, 0, 1, 1);
		layout->addWidget(seeker, 1, 1, 1, 1);
		layout->addWidget(totalLabel, 1, 2, 1, 2);
		resolutionLabel->setVisible(false);
	}
}

void TMediaPanel::setResolutionVisible(bool b) {
	rearrangeWidgets(b);
}

void TMediaPanel::setScrollingEnabled(bool b) {
	mediaLabel->setScrollingEnabled(b);
}

void TMediaPanel::paintEvent(QPaintEvent *) {
	QPainter p(this);
	p.drawPixmap(0,0,leftBackground.width(), 53, leftBackground);
	p.drawPixmap(width() - rightBackground.width(), 0, rightBackground.width(), 53, rightBackground);
	p.drawTiledPixmap(leftBackground.width(), 0, width() - leftBackground.width() - rightBackground.width(), 53, centerBackground );    
}

void TMediaPanel::setShuffleIcon(TIcon icon) {
	shuffleButton->setIcon(icon);
	shuffleButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off));
}

void TMediaPanel::setRepeatIcon(TIcon icon) {
	repeatButton->setIcon(icon);
	repeatButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off));
}

void TMediaPanel::setActionCollection(QList<QAction *>actions) {
	//TActionTools::findAction("aaa", actions);
	SETACTIONTOBUTTON(shuffleButton, "pl_shuffle");
	SETACTIONTOBUTTON(repeatButton, "pl_repeat");

	retranslateStrings();
}

void TMediaPanel::setPlayerState(TCore::State state) {

	if (state == TCore::Stopped) {
		seeker->setEnabled(false);
	} else if (state == TCore::Paused || state == TCore::Playing) {
		seeker->setEnabled(true);
	}
}

void TMediaPanel::setDuration(int duration) {

	this->duration = duration;
	if (duration == 0) {
		seeker->setState(TPanelSeeker::Stopped, true);
	}
	else {
		seeker->setState(TPanelSeeker::Stopped, false);
	}
	setTotalText(Helper::formatTime(duration));
	setElapsedText(Helper::formatTime(0));
}

void TMediaPanel::setMediaLabelText(QString text) {
	mediaLabel->setText(text);
	mediaLabel->update();
	originalTitle = text;
}

void TMediaPanel::setResolutionLabelText(QString text) {
	resolutionLabel->setText(text);
}

void TMediaPanel::setStatusText(QString text, int time) {
	mediaLabel->setText(text);
	mediaLabel->update();
	if (time > 0)
		timer->start(time);
	else
		timer->stop();
}

void TMediaPanel::reverseStatus() {
	setMediaLabelText(originalTitle);
}

void TMediaPanel::setBuffering(bool enable) {
	if (enable) {
		seeker->setState(TPanelSeeker::Buffering, true);
	}
	else
	{
		seeker->setState(TPanelSeeker::Buffering, false);
	}
}

void TMediaPanel::setSeeker(int v) {
	seeker->setSliderValue(v);
}

bool TMediaPanel::eventFilter(QObject *o, QEvent *e) {
	if (o == seeker && e->type() == QEvent::ToolTip) {
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
		qreal value = seeker->valueForPos(helpEvent->pos().x())* duration/seeker->maximum();
		if (value >=0 && value <= duration) {
			QToolTip::showText(helpEvent->globalPos(),Helper::formatTime(value), seeker);
		} else {
			QToolTip::hideText();
		}
	}
	return false;
}

// Language change stuff
void TMediaPanel::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

void TMediaPanel::retranslateStrings() {
	if (shuffleButton) shuffleButton->setToolTip(tr("Shuffle playlist"));
	if (repeatButton) repeatButton->setToolTip(tr("Repeat playlist"));
}

void TScrollingLabel::paintEvent(QPaintEvent *) {
	QPainter p(this);
	p.setFont(font());
	p.setPen(palette().color(foregroundRole()));
	p.setRenderHint(QPainter::TextAntialiasing, true);
	QRect widgetRect = rect();
	if (textRect.width() <= width()) {
		p.drawText(widgetRect, Qt::AlignVCenter | Qt::AlignLeading, mText );
	} else {
		p.translate(-scrollPos, 0);
		p.drawText(widgetRect.adjusted(0,0,scrollPos, 0), Qt::AlignVCenter | Qt::AlignLeading, mText);
		p.translate(textRect.width() + gap, 0);
		p.drawText(widgetRect.adjusted(0, 0, scrollPos - gap - textRect.width(), 0) , Qt::AlignVCenter | Qt::AlignLeading, mText);
	}
	p.end();
}

void TScrollingLabel::setText(QString text) {
	mText = text;
	updateLabel();
	repaint();
}

void TScrollingLabel::changeEvent(QEvent* e) {
	if (e->type() == QEvent::FontChange) {
		updateLabel();
	}
}

void TScrollingLabel::updateLabel() {
	QFontMetrics fm(font());
	QRect rect = fm.boundingRect(mText);
	textRect = rect;

	if (timerId > 0) {
		killTimer(timerId);
		timerId = -1;
		scrollPos = 0;
	}

	if (scrolling_enabled) {
		if (rect.width() > width()) {
			timerId = startTimer(20);
		}
	}
}

void TScrollingLabel::timerEvent(QTimerEvent *) {
	scrollPos += 1;
	scrollPos = scrollPos % (textRect.width() + gap);
	update();
}

TScrollingLabel::TScrollingLabel(QWidget* parent) {
	Q_UNUSED(parent)

	scrollPos =0;
	timerId = -1;
	scrolling_enabled = false;
	textRect = QRect();
	setAttribute(Qt::WA_StyledBackground, true);
	setText("SMPlayer");
}

void TScrollingLabel::setScrollingEnabled(bool b) {
	scrolling_enabled = b;
	updateLabel();
	repaint();
}

void TScrollingLabel::resizeEvent(QResizeEvent *) {
	updateLabel();
}

QSize TScrollingLabel::sizeHint() const {
	QFontMetrics fm(font());
	return QSize(0, fm.height());
}

} // namesapce Skin
} // namespace Gui

#include "moc_mediapanel.cpp"
