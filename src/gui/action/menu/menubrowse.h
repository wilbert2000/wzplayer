#ifndef GUI_ACTION_MENU_MENUBROWSE_H
#define GUI_ACTION_MENU_MENUBROWSE_H

#include "gui/action/menu/menu.h"
#include "gui/action/actiongroup.h"
#include "log4qt/logger.h"


namespace Gui {
namespace Action {

class TAction;

namespace Menu {

class TTitleGroup : public TActionGroup {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    explicit TTitleGroup(TMainWindow* mw);
signals:
    void titleTracksChanged(TTitleGroup* titleGroup);
private slots:
    void updateTitles();
};

class TChapterGroup : public TActionGroup {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    explicit TChapterGroup(TMainWindow* mw,
                           TAction* prvChapterAct,
                           TAction* nxtChapterAct);
signals:
    void chaptersChanged(TChapterGroup* chapterGroup);
private:
    TAction* prevChapterAct;
    TAction* nextChapterAct;
private slots:
    void updateChapters();
};

class TAngleGroup : public TActionGroup {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    explicit TAngleGroup(TMainWindow* mw, TAction* nxtAngleAct);
signals:
    void anglesChanged(TAngleGroup* angleGroup);
private:
    TAction* nextAngleAct;
private slots:
    void updateAngles();
};

class TMenuBrowse : public TMenu {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    TMenuBrowse(QWidget* parent, TMainWindow* mw);

private:
    TMenu* titlesMenu;
    TMenu* chaptersMenu;
    TMenu* anglesMenu;

private slots:
    void updateTitles(TTitleGroup* titleGroup);
    void updateChapters(TChapterGroup* chapterGroup);
    void updateAngles(TAngleGroup* angleGroup);
}; // class TMenuBrowse

} // namespace Menu
} // namespace Action
} // namespace Gui

#endif // GUI_ACTION_MENU_MENUBROWSE_H
