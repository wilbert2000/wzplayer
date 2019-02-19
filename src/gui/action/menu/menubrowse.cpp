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
    : TMenu(mw, mw, "browse_menu", tr("Browse"), "noicon") {

    // Titles
    titlesMenu = new TMenu(mw, mw, "titles_menu", tr("Title"), "title");
    addMenu(titlesMenu);
    titleGroup = new TActionGroup(mw, "titlegroup");
    connect(titleGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setTitle);
    connect(player, &Player::TPlayer::titleTrackChanged,
            titleGroup, &TActionGroup::setChecked);
    connect(player, &Player::TPlayer::titleTracksChanged,
            this, &TMenuBrowse::updateTitles);

    // Chapters
    nextChapterAct = new TAction(mw, "next_chapter", tr("Next chapter"), "",
                                 Qt::Key_C);
    connect(nextChapterAct, &TAction::triggered,
            player, &Player::TPlayer::nextChapter);
    prevChapterAct = new TAction(mw, "prev_chapter", tr("Previous chapter"), "",
                                 Qt::SHIFT | Qt::Key_C);
    connect(prevChapterAct, &TAction::triggered,
            player, &Player::TPlayer::prevChapter);

    chaptersMenu = new TMenu(mw, mw, "chapters_menu", tr("Chapter"), "chapter");
    chaptersMenu->addAction(nextChapterAct);
    chaptersMenu->addAction(prevChapterAct);
    chaptersMenu->addSeparator();
    addMenu(chaptersMenu);

    chapterGroup = new TActionGroup(mw, "chaptergroup");
    connect(chapterGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setChapter);
    connect(player, &Player::TPlayer::chapterChanged,
            chapterGroup, &TActionGroup::setChecked);
    // Update normally done by updateTitles. For DVDNAV only:
    connect(player, &Player::TPlayer::chaptersChanged,
            this, &TMenuBrowse::updateChapters);

    // Angles submenu
    nextAngleAct = new TAction(mw, "next_angle", tr("Next angle"), "",
                               Qt::SHIFT | Qt::Key_A);
    connect(nextAngleAct, &TAction::triggered,
            player, &Player::TPlayer::nextAngle);
    anglesMenu = new TMenu(mw, mw, "angles_menu", tr("Angle"), "angle");
    anglesMenu->addAction(nextAngleAct);
    anglesMenu->addSeparator();
    addMenu(anglesMenu);
    angleGroup = new TActionGroup(mw, "anglegroup");
    connect(angleGroup, &TActionGroup::activated,
            player, &Player::TPlayer::setAngle);
    // Update normally done by updateTitles. For DVDNAV only:
    connect(player, &Player::TPlayer::anglesChanged,
            this, &TMenuBrowse::updateAngles);

    addSeparator();

    // DVDNAV
    dvdnavUpAct = new TAction(mw, "dvdnav_up", tr("DVD, move up"), "",
                              Qt::META | Qt::Key_Up);
    connect(dvdnavUpAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavUp);
    addAction(dvdnavUpAct);
    dvdnavDownAct = new TAction(mw, "dvdnav_down", tr("DVD, move down"), "",
                                Qt::META | Qt::Key_Down);
    connect(dvdnavDownAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavDown);
    addAction(dvdnavDownAct);
    dvdnavLeftAct = new TAction(mw, "dvdnav_left", tr("DVD, move left"), "",
                                Qt::META | Qt::Key_Left);
    connect(dvdnavLeftAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavLeft);
    addAction(dvdnavLeftAct);
    dvdnavRightAct = new TAction(mw, "dvdnav_right", tr("DVD, move right"), "",
                                 Qt::META | Qt::Key_Right);
    connect(dvdnavRightAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavRight);
    addAction(dvdnavRightAct);

    addSeparator();

    dvdnavSelectAct = new TAction(mw, "dvdnav_select", tr("DVD, select"), "",
                                  Qt::META | Qt::Key_Return);
    connect(dvdnavSelectAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavSelect);
    addAction(dvdnavSelectAct);

    // Not in menu
    dvdnavMouseAct = new TAction(mw, "dvdnav_mouse", tr("DVD, mouse click"));
    connect(dvdnavMouseAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavMouse);

    dvdnavMenuAct = new TAction(mw, "dvdnav_menu", tr("DVD menu"), "",
                                Qt::CTRL | Qt::Key_Return);
    connect(dvdnavMenuAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavMenu);
    addAction(dvdnavMenuAct);

    dvdnavPrevAct = new TAction(mw, "dvdnav_prev", tr("DVD previous menu"), "",
                                Qt::META | Qt::Key_Escape);
    connect(dvdnavPrevAct, &TAction::triggered,
            player, &Player::TPlayer::dvdnavPrev);
    addAction(dvdnavPrevAct);
}

void TMenuBrowse::enableActions() {

    bool pop = player->statePOP();
    bool enableChapters = pop && player->mdat.chapters.count() > 0;
    prevChapterAct->setEnabled(enableChapters);
    nextChapterAct->setEnabled(enableChapters);

    nextAngleAct->setEnabled(pop && player->mdat.angles > 1);

    bool enableDVDNav = pop
            && player->mdat.detected_type == TMediaData::TYPE_DVDNAV;
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
        foreach(const Maps::TTitleData& title, player->mdat.titles) {
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
        foreach(const Maps::TChapterData& chapter, player->mdat.chapters) {
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
