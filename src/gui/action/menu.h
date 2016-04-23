#ifndef GUI_MENU_H
#define GUI_MENU_H

#include <QMenu>


namespace Settings {
class TMediaSettings;
}

namespace Gui {

class TBase;

namespace Action {


// Evade mouse before popping up
void execPopup(QWidget* w, QMenu* popup, QPoint p);


class TMenu : public QMenu {
	Q_OBJECT
public:
	explicit TMenu(QWidget* parent,
                   TBase* w,
				   const QString& name,
				   const QString& text,
				   const QString& icon = QString());
	virtual ~TMenu();

	void addActionsTo(QWidget* w);

protected:
    TBase* main_window;
	virtual void onAboutToShow();
	virtual void setVisible(bool visible);

protected slots:
    virtual void enableActions();
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
};

} // namespace Action
} // namespace Gui

#endif // GUI_MENU_H
