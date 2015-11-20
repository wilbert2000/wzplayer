#ifndef GUI_MENUS_H
#define GUI_MENUS_H

#include <QMenu>
#include "gui/action/actiongroup.h"
#include "core.h"

class QActionGroup;

namespace Settings {
class TMediaSettings;
}


namespace Gui {

class TAction;

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


class TOSDMenu : public TMenu {
public:
	explicit TOSDMenu(QWidget* parent, TCore* c);
protected:
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

} // namespace Gui

#endif // GUI_MENUS_H
