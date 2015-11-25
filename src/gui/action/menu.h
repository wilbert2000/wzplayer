#ifndef GUI_MENU_H
#define GUI_MENU_H

#include <QMenu>


namespace Settings {
class TMediaSettings;
}

namespace Gui {

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
	QString text_en;

	virtual void changeEvent(QEvent* event);
	virtual void onAboutToShow();
	virtual void setVisible(bool visible);

protected slots:
	virtual void enableActions(bool stopped, bool video, bool audio);
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);

private:
	void retranslateStrings();
};

} // namespace Gui

#endif // GUI_MENU_H
