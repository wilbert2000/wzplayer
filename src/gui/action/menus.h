#ifndef VIDEOSIZEGROUP_H
#define VIDEOSIZEGROUP_H

#include "gui/action/actiongroup.h"
#include <QMenu>

class TPlayerWindow;
class TCore;

namespace Gui {

void execPopup(QWidget* w, QMenu* popup, QPoint p);

class TMenu : public QMenu {
	Q_OBJECT
public:
	TMenu(QWidget* parent,
		  QObject* atranslator,
		  const QString& name,
		  const QString& text,
		  const QString& icon);
protected:
	virtual void changeEvent(QEvent* event);
protected slots:
	virtual void onAboutToShow();
private:
	QString text_en;
	QObject* translator;
	void retranslateStrings();
};


class TAspectMenu : public TMenu {
public:
	explicit TAspectMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TAudioChannelMenu : public TMenu {
public:
	explicit TAudioChannelMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TCCMenu : public TMenu {
public:
	explicit TCCMenu(QWidget* parent, TCore* c);
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TDeinterlaceMenu : public TMenu {
public:
	explicit TDeinterlaceMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TOnTopMenu : public TMenu {
public:
	explicit TOnTopMenu(QWidget* parent);
protected:
	virtual void onAboutToShow();
private:
	TActionGroup* group;
};


class TOSDMenu : public TMenu {
public:
	explicit TOSDMenu(QWidget* parent, TCore* c);
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TRotateMenu : public TMenu {
public:
	explicit TRotateMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TStereoMenu : public TMenu {
public:
	explicit TStereoMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TSubFPSMenu : public TMenu {
public:
	explicit TSubFPSMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TVideoFilterMenu : public TMenu {
	Q_OBJECT
public:
	explicit TVideoFilterMenu(QWidget* parent, TCore* c);
	void setEnabledX(bool enable);
protected:
	virtual void onAboutToShow();
private:
	TCore* core;

	QActionGroup* group;
	TAction* postProcessingAct;
	TAction* deblockAct;
	TAction* deringAct;
	TAction* gradfunAct;
	TAction* addNoiseAct;
	TAction* addLetterboxAct;
	TAction* upscaleAct;
	TAction* phaseAct;

	// Denoise Action Group
	TActionGroup* denoiseGroup;
	TAction* denoiseNoneAct;
	TAction* denoiseNormalAct;
	TAction* denoiseSoftAct;

	// Blur-sharpen group
	TActionGroup* unsharpGroup;
	TAction* unsharpNoneAct;
	TAction* blurAct;
	TAction* sharpenAct;

private slots:
	void onAboutToShowDenoise();
	void onAboutToShowUnSharp();
};


class TVideoSizeGroup : public TActionGroup {
	Q_OBJECT
public:
	explicit TVideoSizeGroup(QWidget* parent, TPlayerWindow* pw);
public slots:
	void enableVideoSizeGroup(bool on);
	void updateVideoSizeGroup();
private:
	TPlayerWindow* playerWindow;
	void uncheck();
};


class TVideoSizeMenu : public TMenu {
public:
	TVideoSizeMenu(QWidget* parent, TPlayerWindow* pw);
	void enableVideoSize(bool on);
protected:
	virtual void onAboutToShow();
private:
	TVideoSizeGroup* group;
	TAction* doubleSizeAct;
};


class TVideoZoomAndPanMenu : public TMenu {
public:
	explicit TVideoZoomAndPanMenu(QWidget* parent, TCore* c);
	QActionGroup* group;
private:
	TCore* core;
};

} // namespace Gui
#endif // VIDEOSIZEGROUP_H
