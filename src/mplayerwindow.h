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


#ifndef MPLAYERWINDOW_H
#define MPLAYERWINDOW_H

#include <QWidget>
#include <QSize>
#include <QPoint>

#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QTime>

#include "config.h"
#include "myactiongroup.h"


class QWidget;
class QLabel;
class QKeyEvent;
class QTimer;

// Zooming
const double ZOOM_MIN = 0.05;
const double ZOOM_MAX = 8.0; // High max can blow up surface
const double ZOOM_STEP = 0.05;

const int PAN_STEP = 8;

// Distance the mouse must travel before it is shown if not clicked
#define SHOW_MOUSE_TRESHOLD 4


//! MplayerLayer can be instructed to not delete the background.

class MplayerLayer : public QWidget {
	Q_OBJECT

public:
	MplayerLayer(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~MplayerLayer();

#if REPAINT_BACKGROUND_OPTION
	//! If b is true, the background of the widget will be repainted as usual.
	/*! Otherwise the background will not repainted when a video is playing. */
	void setRepaintBackground(bool b);

	//! Return true if repainting the background is allowed.
	bool repaintBackground() { return repaint_background; };
#endif

	//! Should be called when a file has started. 
    /*! It's needed to know if the background has to be cleared or not. */
	void setFastBackground();
	//! Should be called when a file has stopped.
	void restoreNormalBackground();

#if REPAINT_BACKGROUND_OPTION
protected:
	virtual void paintEvent ( QPaintEvent * e );
#endif

private:
#if REPAINT_BACKGROUND_OPTION
	bool repaint_background;
#endif
	bool normal_background;
};


class MplayerWindow : public QWidget
{
	Q_OBJECT

public:
	MplayerWindow(QWidget* parent = 0, Qt::WindowFlags f = 0);
	~MplayerWindow();

	MplayerLayer* videoLayer() { return mplayerlayer; }

	bool main_window_moved;

	void set(double aspect,
			 double zoom_factor,
			 double zoom_factor_fullscreen,
			 QPoint pan,
			 QPoint pan_fullscreen);

	void setAspect(double aspect, bool updateVideoWindow = true);
	void setMonitorAspect(double asp);
	void setResolution(int width, int height);

	// Zoom
	// Sets current zoom to factor if factor_fullscreen == 0.0
	// else sets both zoom for normal and full screen.
	// Kept between ZOOM_MIN and ZOOM_MAX
	void setZoom(double factor,
				 double factor_fullscreen = 0.0,
				 bool updateVideoWindow = true);
	// Zoom current screen
	double zoom();
	// Zoom normal screen
	double zoomNormalScreen() { return zoom_factor; }
	// Zoom full screen
	double zoomFullScreen() { return zoom_factor_fullscreen; }

	// Pan
	void setPan(QPoint pan, QPoint pan_fullscreen, bool updateVideoWindow = true);
	// Pan current screen
	QPoint pan();
	// Pan normal screen
	QPoint panNormalScreen() { return pan_offset; }
	// Pan full screen
	QPoint panFullScreen() { return pan_offset_fullscreen; }

	// Reset zoom and pan for current screen
	void resetZoomAndPan();

	void setDelayLeftClick(bool b) { delay_left_click = b; };

	// Get size adjusted for monitor aspect and desired zoom
	QSize getAdjustedSize(int w, int h, double desired_zoom) const;

	// Keep track off full screen state
	void aboutToEnterFullscreen();
	void aboutToExitFullscreen();

	void updateVideoWindow();
	void moveVideo(int dx, int dy);

	void setSizeGroup(MyActionGroup* group);

#if USE_COLORKEY
	void setColorKey(QColor c);
#endif

#if LOGO_ANIMATION
	bool animatedLogo() { return animated_logo; }
#endif

#ifdef SHAREWIDGET
	void setCornerWidget(QWidget * w);
	QWidget * cornerWidget() { return corner_widget; };
#endif

public slots:
	void aboutToStartPlaying();
	void playingStopped();

	void setLogoVisible(bool b);
	void showLogo() { setLogoVisible(true); };
	void hideLogo() { setLogoVisible(false); };

#if LOGO_ANIMATION
	void setAnimatedLogo(bool b) { animated_logo = b; };
#endif

protected slots:
	void checkHideMouse();
	void enableMessages();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;
	virtual void resizeEvent(QResizeEvent *);

	virtual void mousePressEvent ( QMouseEvent * e );
	virtual void mouseMoveEvent ( QMouseEvent * e );
	virtual void mouseReleaseEvent( QMouseEvent * e);
	virtual void mouseDoubleClickEvent( QMouseEvent * e );
	virtual void wheelEvent( QWheelEvent * e );

signals:
	void doubleClicked();
	void leftClicked();
	void rightClicked();
	void middleClicked();
	void xbutton1Clicked(); // first X button
	void xbutton2Clicked(); // second X button
	void keyPressed(QKeyEvent * e);
	void wheelUp();
	void wheelDown();
	void mouseMoved(QPoint);
	void showMessage(QString text, int duration, int osd_level);
	void moveOSD(QPoint pos);

protected:
	double aspect;
	double monitoraspect;

	QLabel * logo;

	// Zoom and pan
	double zoom_factor;
	double zoom_factor_fullscreen;
	QPoint pan_offset;
	QPoint pan_offset_fullscreen;

	// Delay left click event
	bool delay_left_click;
	QTimer * left_click_timer;
	bool double_clicked;

#if LOGO_ANIMATION
	bool animated_logo;
#endif

	QWidget * corner_widget;

private:
	MplayerLayer * mplayerlayer;

	int video_width;
	int video_height;
	MyActionGroup* size_group;

	QSize last_video_size;

	QTime* left_button_pressed_time;
	QPoint drag_pos;
	bool dragging;
	bool kill_fake_event;

	bool fullscreen;
	bool enable_messages;

	bool autohide_cursor;
	QTimer * check_hide_mouse_timer;
	QPoint check_hide_mouse_last_position;
	int autohide_interval;

	void autoHideCursorStartTimer();
	void showHiddenCursor(bool startTimer);
	void setAutoHideCursor(bool enable);
	void setMouseTrackingInclChildren(QWidget *w);

	void moveVideo(QPoint delta);

	void startDragging();
	void stopDragging();
	bool checkDragging(QMouseEvent * event);

	void uncheckSizeGroup();
	void enableSizeGroup();
	void updateSizeGroup();

	void pauseMessages(int msec);
};

#endif

