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


#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QWidget>
#include <QPoint>
#include <QSize>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QTime>

class QTimer;
class QLabel;


//! TPlayerLayer can be instructed to not delete the background.
class TPlayerLayer : public QWidget {
	Q_OBJECT

public:
	explicit TPlayerLayer(QWidget* parent);
	virtual ~TPlayerLayer();

	//! Should be called when a file has started playing.
    /*! It's needed to know if the background has to be cleared or not. */
	void setFastBackground();

	//! Should be called when a file has stopped playing.
	void restoreNormalBackground();

protected:
	virtual void paintEvent(QPaintEvent* e);

private:
	bool normal_background;
};


class TPlayerWindow : public QWidget {
	Q_OBJECT

public:
	TPlayerWindow(QWidget* parent);
	virtual ~TPlayerWindow();

	TPlayerLayer* videoLayer() { return playerlayer; }

	void set(double aspect,
			 double zoom_factor,
			 double zoom_factor_fullscreen,
			 QPoint pan,
			 QPoint pan_fullscreen);
	void setAspect(double aspect, bool updateVideoWindow = true);
	void setMonitorAspect(double asp);
	void setResolution(int width, int height);
	QSize resolution() const { return QSize(video_width, video_height); }

	// Zoom
	// Sets current zoom to factor if factor_fullscreen == 0.0
	// else sets both zoom for normal and full screen.
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

	// Reset zoom and pan full and normal screen
	void resetZoomAndPan();

	void setDelayLeftClick(bool b) { delay_left_click = b; }
	void setColorKey(QColor c);

	// Get size adjusted for aspect and desired zoom
	QSize getAdjustedSize(int w, int h, double zoom) const;

	// Calculate size factor for current view
	void getSizeFactors(double& factorX, double& factorY);
	double getSizeFactor();
	void updateSizeFactor();

	void updateVideoWindow();
	void moveVideo(int dx, int dy);
	void playingStopped(bool clear_background = true);
	void retranslateStrings();

public slots:
	void aboutToStartPlaying();

	void setLogoVisible(bool b);
	void showLogo() { setLogoVisible(true); }
	void hideLogo() { setLogoVisible(false); }

signals:
	void leftClicked();
	void doubleClicked();
	void rightClicked();
	void middleClicked();
	void xbutton1Clicked(); // first X button
	void xbutton2Clicked(); // second X button
	void keyPressed(QKeyEvent* e);
	void wheelUp();
	void wheelDown();
	void mouseMoved(QPoint);
	void draggingChanged(bool);
	void moveWindow(QPoint);

	void moveOSD(QPoint pos);
	void videoOutChanged(const QSize& size);

protected:
	virtual void resizeEvent(QResizeEvent*);
	virtual void mousePressEvent(QMouseEvent* e);
	virtual void mouseMoveEvent(QMouseEvent* e);
	virtual void mouseReleaseEvent(QMouseEvent* e);
	virtual void mouseDoubleClickEvent(QMouseEvent* e);
	virtual void wheelEvent(QWheelEvent* e);

private:
	TPlayerLayer* playerlayer;

	int video_width;
	int video_height;
	QSize last_video_size;

	double zoom_factor;
	double zoom_factor_fullscreen;
	QPoint pan_offset;
	QPoint pan_offset_fullscreen;

	double aspect;
	double monitoraspect;

	bool double_clicked;
	bool delay_left_click;
	QTimer* left_click_timer;
	QTime left_button_pressed_time;
	QPoint drag_pos;
	bool dragging;
	bool kill_fake_event;

	QLabel* logo;

	void moveVideo(QPoint delta);

	void startDragging();
	void stopDragging();
	bool checkDragging(QMouseEvent* event);
};

#endif // PLAYERWINDOW_H

