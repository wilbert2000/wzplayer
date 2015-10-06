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

#ifndef GUI_SKIN_MEDIAPANEL_H
#define GUI_SKIN_MEDIAPANEL_H

#include <QWidget>
#include <QPixmap>
#include <QLabel>

#include "gui/skin/button.h"
#include "gui/skin/panelseeker.h"
#include "core.h"

#include "ui_mediapanel.h"

class QGridLayout;

namespace Gui {
namespace Skin {

class TScrollingLabel : public QWidget
{
    Q_OBJECT

public:
	TScrollingLabel(QWidget* parent=0);
	~TScrollingLabel(){}
    QString text() { return mText; }
    void setText( QString text);

    void setScrollingEnabled(bool b);
	bool scrollingEnabled() { return scrolling_enabled; }

private:
    QString mText;
    void updateLabel();
    int scrollPos;
    int timerId;
    QRect textRect;
    static const int gap = 10;
    bool scrolling_enabled;

protected:
    void paintEvent(QPaintEvent *);
    void changeEvent(QEvent *);
    void resizeEvent(QResizeEvent *);
    QSize sizeHint() const;

private slots:
    void timerEvent(QTimerEvent *);
};

class TMediaPanel : public QWidget, public Ui::TMediaPanel
{
    Q_OBJECT
    Q_PROPERTY(QPixmap bgLeft READ bgLeftPix WRITE setBgLeftPix)
    Q_PROPERTY(QPixmap bgRight READ bgRightPix WRITE setBgRightPix)
    Q_PROPERTY(QPixmap bgCenter READ bgCenterPix WRITE setBgCenterPix)

public:
	TMediaPanel(QWidget* parent, int pos_max);
	virtual ~TMediaPanel();
    QPixmap bgLeftPix() { return leftBackground ;}
    void setBgLeftPix( QPixmap pix){ leftBackground = pix; }
    QPixmap bgRightPix() { return rightBackground ;}
    void setBgRightPix( QPixmap pix){ rightBackground = pix; }
    QPixmap bgCenterPix() { return centerBackground ;}
    void setBgCenterPix( QPixmap pix){ centerBackground = pix; }
	void setShuffleIcon( TIcon icon );
	void setRepeatIcon(TIcon icon);
    void setElapsedText(QString text) {
        elapsedLabel->setText(text);
		if(seeker->states().testFlag(TPanelSeeker::Buffering))
            setBuffering(false);
        }
    void setTotalText( QString text) { totalLabel->setText(text); }
    void setActionCollection(QList<QAction*> actions);
    void setMediaLabelText(QString text);
    void setResolutionLabelText(QString text);
    void setStatusText(QString text, int time = 2000);
    void setBuffering(bool enable);
    bool eventFilter(QObject *object, QEvent *event);

public slots:
	void setDuration(int duration);
	void setSeeker(int v);
	void setPlayerState(TCore::State state);
	void setResolutionVisible(bool b);
	void setScrollingEnabled(bool b);

private:
    QGridLayout * layout;
    QPixmap leftBackground;
    QPixmap centerBackground;
    QPixmap rightBackground;
	TScrollingLabel* mediaLabel;
    QLabel *resolutionLabel;
	TPanelTimeSeeker* seeker;
	TButton* repeatButton;
	TButton* shuffleButton;
    QLabel* elapsedLabel;
    QLabel* totalLabel;
    QString originalTitle;
    QTimer* timer;    
    int duration;

private slots:
    void reverseStatus();
    void rearrangeWidgets(bool resolution_visible);

protected:
    void paintEvent(QPaintEvent *);
    virtual void changeEvent (QEvent * event);
    virtual void retranslateStrings();

signals:
	void seekerChanged(int);
	void seekerWheelUp();
	void seekerWheelDown();

public:
    friend class TIconSetter;
};

} // namesapce Skin
} // namespace Gui

#endif // GUI_SKIN_MEDIAPANEL_H
