#ifndef VIDEOSIZEGROUP_H
#define VIDEOSIZEGROUP_H

#include "gui/action/actiongroup.h"
#include <QMenu>
#include "core.h"

class TPlayerWindow;

namespace Settings {
class TMediaSettings;
}


namespace Gui {

class TSeekingButton;
class TFavorites;
class TTVList;
class TPlaylist;
class TBase;

// Evade mouse before popping up
void execPopup(QWidget* w, QMenu* popup, QPoint p);


class TMenu : public QMenu {
	Q_OBJECT
public:
	explicit TMenu(QWidget* parent,
				   QObject* aTranslator,
				   const QString& name,
				   const QString& text,
				   const QString& icon = QString());
	virtual ~TMenu();

	void addActionsTo(QWidget* w);

protected:
	QObject* translator;

	virtual void changeEvent(QEvent* event);
	virtual void onAboutToShow();
	virtual void setVisible(bool visible);

protected slots:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	QString text_en;
	void retranslateStrings();
};


class TABMenu : public TMenu {
public:
	explicit TABMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	QActionGroup* group;
	TAction* repeatAct;
};


class TAspectMenu : public TMenu {
public:
	explicit TAspectMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TAudioChannelMenu : public TMenu {
public:
	explicit TAudioChannelMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TCCMenu : public TMenu {
public:
	explicit TCCMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TDeinterlaceMenu : public TMenu {
public:
	explicit TDeinterlaceMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TDiscMenu : public TMenu {
public:
	explicit TDiscMenu(QWidget* parent);
};


class TOpenMenu : public TMenu {
	Q_OBJECT
public:
	explicit TOpenMenu(TBase* parent, TCore* core, QWidget* playlist);
	void updateRecents();
private:
	TBase* main_window;
	TMenu* recentfiles_menu;
	TAction* clearRecentsAct;
	TFavorites* favorites;
	TTVList* tvlist;
	TTVList* radiolist;
private slots:
	void clearRecentsList();
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


class TPlayMenu : public TMenu {
	Q_OBJECT
public:
	explicit TPlayMenu(QWidget* parent, TCore* c, Gui::TPlaylist* plist);
	void setJumpTexts();
	void retranslateStrings();
protected:
	virtual void enableActions(bool stopped, bool, bool);
private:
	TCore* core;
	Gui::TPlaylist* playlist;

	TAction* playAct;
	TAction* playOrPauseAct;
	QIcon pauseIcon;
	QIcon playIcon;
	TAction* pauseAct;
	TAction* stopAct;
	TAction* frameBackStepAct;
	TAction* frameStepAct;
	TAction* rewind1Act;
	TAction* forward1Act;
	TAction* rewind2Act;
	TAction* forward2Act;
	TAction* rewind3Act;
	TAction* forward3Act;
	TSeekingButton* rewindbutton_action;
	TSeekingButton* forwardbutton_action;
	TAction* playPrevAct;
	TAction* playNextAct;
	TAction* gotoAct;
private slots:
	void onStateChanged(TCore::State state);
};

class TPlaySpeedMenu : public TMenu {
public:
	explicit TPlaySpeedMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool);
private:
	TCore* core;
	QActionGroup* group;
};


class TRotateMenu : public TMenu {
public:
	explicit TRotateMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TStayOnTopMenu : public TMenu {
public:
	explicit TStayOnTopMenu(QWidget* parent);
	TActionGroup* group;
protected:
	virtual void onAboutToShow();
};


class TStereoMenu : public TMenu {
public:
	explicit TStereoMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
	TActionGroup* group;
};


class TSubFPSMenu : public TMenu {
public:
	explicit TSubFPSMenu(QWidget* parent, TCore* c);
	TActionGroup* group;
protected:
	virtual void enableActions(bool stopped, bool, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings* mset);
	virtual void onAboutToShow();
private:
	TCore* core;
};


class TVideoFilterMenu : public TMenu {
	Q_OBJECT
public:
	explicit TVideoFilterMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
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

	void updateFilters();

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
	Q_OBJECT
public:
	TVideoSizeMenu(TBase* parent, TPlayerWindow* pw);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
	virtual void onAboutToShow();
private:
	TVideoSizeGroup* group;
	TAction* doubleSizeAct;
private slots:
	virtual void fullscreenChanged();
};


class TVideoZoomAndPanMenu : public TMenu {
public:
	explicit TVideoZoomAndPanMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
private:
	TCore* core;
	QActionGroup* group;
};

} // namespace Gui
#endif // VIDEOSIZEGROUP_H
