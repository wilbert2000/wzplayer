#include "gui/action/menu/menubrowse.h"
#include <QWidget>
#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "player/player.h"
#include "gui/mainwindow.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

TMenuBrowse::TMenuBrowse(TMainWindow* mw)
    : TMenu(mw, mw, "browse_menu", tr("&Browse"), "noicon") {

    // Titles
    titlesMenu = new TMenu(main_window, main_window, "titles_menu", tr("&Title"), "title");
    addMenu(titlesMenu);
    titleGroup = new TActionGroup(this, "title");
    connect(titleGroup, SIGNAL(activated(int)), player, SLOT(changeTitle(int)));
    connect(player, SIGNAL(titleTrackChanged(int)), titleGroup, SLOT(setChecked(int)));
    connect(player, SIGNAL(titleTracksChanged()), this, SLOT(updateTitles()));

    // Chapters
    nextChapterAct = new TAction(this, "next_chapter", tr("Next chapter"), "", Qt::Key_C, false);
    main_window->addAction(nextChapterAct);
    connect(nextChapterAct, SIGNAL(triggered()), player, SLOT(nextChapter()));
    prevChapterAct = new TAction(this, "prev_chapter", tr("Previous chapter"), "", Qt::SHIFT | Qt::Key_C, false);
    main_window->addAction(prevChapterAct);
    connect(prevChapterAct, SIGNAL(triggered()), player, SLOT(prevChapter()));

    chaptersMenu = new TMenu(main_window, main_window, "chapters_menu", tr("&Chapter"), "chapter");
    chaptersMenu->addAction(nextChapterAct);
    chaptersMenu->addAction(prevChapterAct);
    chaptersMenu->addSeparator();
    addMenu(chaptersMenu);

    chapterGroup = new TActionGroup(this, "chapter");
    connect(chapterGroup, SIGNAL(activated(int)), player, SLOT(changeChapter(int)));
    connect(player, SIGNAL(chapterChanged(int)), chapterGroup, SLOT(setChecked(int)));
    // Update normally done by updateTitles. For DVDNAV only:
    connect(player, SIGNAL(chaptersChanged()), this, SLOT(updateChapters()));

    // Angles submenu
    nextAngleAct = new TAction(this, "next_angle", tr("Next angle"), "", Qt::SHIFT | Qt::Key_A, false);
    connect(nextAngleAct, SIGNAL(triggered()), player, SLOT(nextAngle()));
    anglesMenu = new TMenu(main_window, main_window, "angles_menu", tr("&Angle"), "angle");
    anglesMenu->addAction(nextAngleAct);
    anglesMenu->addSeparator();
    addMenu(anglesMenu);
    angleGroup = new TActionGroup(this, "angle");
    connect(angleGroup, SIGNAL(activated(int)), player, SLOT(setAngle(int)));
    // Update normally done by updateTitles. For DVDNAV only:
    connect(player, SIGNAL(anglesChanged()), this, SLOT(updateAngles()));

#if PROGRAM_SWITCH
    programMenu = new TMenu(parent, this, "program_menu", tr("P&rogram"), "program");
    addMenu(programMenu);
    programGroup = new TActionGroup(this, "program");
    connect(programGroup, SIGNAL(activated(int)), player, SLOT(changeProgram(int)));
    // TODO: update
#endif

    addSeparator();

    // DVDNAV
    dvdnavUpAct = new TAction(this, "dvdnav_up", tr("DVD, move up"),
                              "", Qt::META | Qt::Key_Up);
    connect(dvdnavUpAct, SIGNAL(triggered()), player, SLOT(dvdnavUp()));
    dvdnavDownAct = new TAction(this, "dvdnav_down", tr("DVD, move down"),
                                "", Qt::META | Qt::Key_Down);
    connect(dvdnavDownAct, SIGNAL(triggered()), player, SLOT(dvdnavDown()));
    dvdnavLeftAct = new TAction(this, "dvdnav_left", tr("DVD, move left"),
                                "", Qt::META | Qt::Key_Left);
    connect(dvdnavLeftAct, SIGNAL(triggered()), player, SLOT(dvdnavLeft()));
    dvdnavRightAct = new TAction(this, "dvdnav_right", tr("DVD, move right"),
                                 "", Qt::META | Qt::Key_Right);
    connect(dvdnavRightAct, SIGNAL(triggered()), player, SLOT(dvdnavRight()));

    addSeparator();

    dvdnavSelectAct = new TAction(this, "dvdnav_select", tr("DVD, select"),
                                  "", Qt::META | Qt::Key_Return);
    connect(dvdnavSelectAct, SIGNAL(triggered()), player, SLOT(dvdnavSelect()));

    // Not in menu, so add to parent only
    dvdnavMouseAct = new TAction(main_window, "dvdnav_mouse", tr("DVD, mouse click"));
    connect(dvdnavMouseAct, SIGNAL(triggered()), player, SLOT(dvdnavMouse()));

    dvdnavMenuAct = new TAction(this, "dvdnav_menu", tr("DVD &menu"),
                                "", Qt::CTRL | Qt::Key_Return);
    connect(dvdnavMenuAct, SIGNAL(triggered()), player, SLOT(dvdnavMenu()));

    dvdnavPrevAct = new TAction(this, "dvdnav_prev", tr("DVD &previous menu"),
                                "", Qt::META | Qt::Key_Escape);
    connect(dvdnavPrevAct, SIGNAL(triggered()), player, SLOT(dvdnavPrev()));

    addActionsTo(main_window);
}

void TMenuBrowse::enableActions() {

    bool pop = player->statePOP();
    bool enableChapters = pop && player->mdat.chapters.count() > 0;
    prevChapterAct->setEnabled(enableChapters);
    nextChapterAct->setEnabled(enableChapters);

    nextAngleAct->setEnabled(pop && player->mdat.angles > 1);

    bool enableDVDNav = pop && player->mdat.detected_type == TMediaData::TYPE_DVDNAV;
    dvdnavUpAct->setEnabled(enableDVDNav);
    dvdnavDownAct->setEnabled(enableDVDNav);
    dvdnavLeftAct->setEnabled(enableDVDNav);
    dvdnavRightAct->setEnabled(enableDVDNav);

    dvdnavMenuAct->setEnabled(enableDVDNav);
    dvdnavPrevAct->setEnabled(enableDVDNav);
    dvdnavSelectAct->setEnabled(enableDVDNav);
    dvdnavMouseAct->setEnabled(enableDVDNav);
}

void TMenuBrowse::updateTitles() {
    WZDEBUG("");

    titleGroup->clear();
    if (player->mdat.titles.count() == 0) {
        QAction* a = titleGroup->addAction(tr("<empty>"));
        a->setEnabled(false);
    } else {
        int selected_ID = player->mdat.titles.getSelectedID();
        foreach(const Maps::TTitleData title, player->mdat.titles) {
            QAction* action = new QAction(titleGroup);
            action->setCheckable(true);
            action->setText(title.getDisplayName());
            action->setData(title.getID());
            if (title.getID() == selected_ID) {
                action->setChecked(true);
            }
        }
    }

    titlesMenu->addActions(titleGroup->actions());

    updateChapters();
    updateAngles();
}

void TMenuBrowse::updateChapters() {
    WZDEBUG("");

    chapterGroup->clear();
    if (player->mdat.chapters.count() > 0) {
        int selected_id = player->mdat.chapters.getSelectedID();
        foreach(const Maps::TChapterData chapter, player->mdat.chapters) {
            QAction *a = new QAction(chapterGroup);
            a->setCheckable(true);
            a->setText(chapter.getDisplayName());
            a->setData(chapter.getID());
            if (chapter.getID() == selected_id) {
                a->setChecked(true);
            }
        }
    } else {
        QAction* a = chapterGroup->addAction(tr("<empty>"));
        a->setEnabled(false);
    }

    chaptersMenu->addActions(chapterGroup->actions());
}

void TMenuBrowse::updateAngles() {
    WZDEBUG("");

    angleGroup->clear();

    // Add angles to menu
    int angles = player->mdat.angles;
    if (angles > 1) {
        nextAngleAct->setEnabled(true);
        int angle = player->mdat.angle;
        for (int n = 1; n <= angles; n++) {
            QAction *a = new QAction(angleGroup);
            a->setCheckable(true);
            a->setText(QString::number(n));
            a->setData(n);
            if (n == angle) {
                a->setChecked(true);
            }
        }
    } else {
        // No angles
        nextAngleAct->setEnabled(false);
        QAction* a = new QAction(angleGroup);
        a->setCheckable(true);
        a->setText("1");
        a->setData(1);
        a->setChecked(true);
        a->setEnabled(false);
    }
    anglesMenu->addActions(angleGroup->actions());
}

} // namespace Menu
} // namespace Action
} // namespace Gui
