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

#ifndef GUI_SKIN_PLAYCONTROL_H
#define GUI_SKIN_PLAYCONTROL_H

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPixmap>
#include "gui/skin/button.h"

namespace Gui {
namespace Skin {

class TPlayControl : public QWidget
{
Q_OBJECT

public:
	explicit TPlayControl(QWidget *parent = 0);

private:
    TButton* backwardButton;
    TButton* previousButton;
    TButton* playPauseButton;
    TButton* stopButton;
    TButton* nextButton;
    TButton* forwardButton;
    TButton* recordButton;
    QHBoxLayout* layout;    
    bool playOrPause;
    void updateSize();    
    void updateWidths();

public:
    QPixmap backwardIcon();
    void setRecordEnabled(bool enable) { recordButton->setEnabled(enable); updateWidths();}
    void setPreviousTrackEnabled(bool enable) { previousButton->setEnabled(enable); updateWidths();}
    void setNextTrackEnabled(bool enable) { nextButton->setEnabled(enable); updateWidths();}
    void setPlay(bool on) { playOrPause = on; playPauseButton->setState(on); }

	void setBackwardIcon(TIcon icon ) { backwardButton->setIcon(icon); backwardButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off)); updateWidths(); }
	void setForwardIcon(TIcon icon) { forwardButton->setIcon(icon); forwardButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off)); updateWidths(); }
	void setPreviousIcon(TIcon icon) { previousButton->setIcon(icon); previousButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off)); updateWidths();}
	void setNextIcon(TIcon icon) { nextButton->setIcon(icon); nextButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off)); updateWidths();}
	void setPlayPauseIcon (TIcon icon) { playPauseButton->setIcon(icon); playPauseButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off));updateWidths();}
	void setStopIcon (TIcon icon) { stopButton->setIcon(icon); stopButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off)); updateWidths();}
	void setRecordIcon(TIcon icon) { recordButton->setIcon(icon); recordButton->setFixedSize(icon.size(TIcon::Normal, TIcon::Off)); updateWidths();}

    void setActionCollection(QList<QAction*> actions);
    bool eventFilter(QObject *watched, QEvent *event);


protected:
	void resizeEvent(QResizeEvent *);
    virtual void changeEvent (QEvent * event);
    virtual void retranslateStrings();

friend class TIconSetter;
};

} // namesapce Skin
} // namespace Gui

#endif // GUI_SKIN_PLAYCONTROL_H
