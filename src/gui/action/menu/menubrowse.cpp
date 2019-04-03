#include "gui/action/menu/menubrowse.h"
#include "gui/action/actiongroup.h"
#include "player/player.h"
#include "gui/mainwindow.h"


using namespace Settings;

namespace Gui {
namespace Action {
namespace Menu {

TTitleGroup::TTitleGroup(TMainWindow *mw) :
    TActionGroup(mw, "titlegroup") {

    connect(this, &TTitleGroup::activated,
            player, &Player::TPlayer::setTitle);
    connect(player, &Player::TPlayer::titleTrackChanged,
            this, &TTitleGroup::setChecked);
    connect(player, &Player::TPlayer::titleTracksChanged,
            this, &TTitleGroup::updateTitles);
}

void TTitleGroup::updateTitles() {
    WZTRACE(QString("Have %1 titles").arg(player->mdat.titles.count()));

    clear();
    if (player->mdat.titles.count() == 0) {
        QAction* a = addAction(tr("<empty>"));
        a->setEnabled(false);
    } else {
        int selected_ID = player->mdat.titles.getSelectedID();
        foreach(const Maps::TTitleData& title, player->mdat.titles) {
            QAction* action = new QAction(this);
            action->setCheckable(true);
            action->setText(title.getDisplayName());
            action->setData(title.getID());
            if (title.getID() == selected_ID) {
                action->setChecked(true);
            }
        }
    }

    emit titleTracksChanged(this);
}


TChapterGroup::TChapterGroup(TMainWindow *mw,
                             TAction* prvChapterAct,
                             TAction* nxtChapterAct) :
    TActionGroup(mw, "chaptergroup"),
    prevChapterAct(prvChapterAct),
    nextChapterAct(nxtChapterAct) {

    connect(this, &TChapterGroup::activated,
            player, &Player::TPlayer::setChapter);
    connect(player, &Player::TPlayer::chapterChanged,
            this, &TChapterGroup::setChecked);
    connect(player, &Player::TPlayer::titleTracksChanged,
            this, &TChapterGroup::updateChapters);
    // updateChapters() normally done by above titleTracksChanged signal.
    // For DVDNAV only:
    connect(player, &Player::TPlayer::chaptersChanged,
            this, &TChapterGroup::updateChapters);
}

void TChapterGroup::updateChapters() {
    WZTRACE(QString("Have %1 chapters").arg(player->mdat.chapters.count()));

    clear();
    if (player->mdat.chapters.count() > 0) {
        int selected_id = player->mdat.chapters.getSelectedID();
        foreach(const Maps::TChapterData& chapter, player->mdat.chapters) {
            QAction *a = new QAction(this);
            a->setCheckable(true);
            a->setText(chapter.getDisplayName());
            a->setData(chapter.getID());
            if (chapter.getID() == selected_id) {
                a->setChecked(true);
            }
        }
    } else {
        QAction* a = addAction(tr("<empty>"));
        a->setEnabled(false);
    }

    bool enable = player->statePOP() && player->mdat.chapters.count() > 0;
    prevChapterAct->setEnabled(enable);
    nextChapterAct->setEnabled(enable);

    emit chaptersChanged(this);
}

TAngleGroup::TAngleGroup(TMainWindow *mw, TAction* nxtAngleAct) :
    TActionGroup(mw, "anglegroup"),
    nextAngleAct(nxtAngleAct) {

    connect(this, &TAngleGroup::activated,
            player, &Player::TPlayer::setAngle);
    connect(player, &Player::TPlayer::titleTracksChanged,
            this, &TAngleGroup::updateAngles);
    // updateAngles() normally done by above titleTracksChanged signal.
    // For DVDNAV only:
    connect(player, &Player::TPlayer::anglesChanged,
            this, &TAngleGroup::updateAngles);
}

void TAngleGroup::updateAngles() {
    WZTRACE(QString("Have %1 angles").arg(player->mdat.angles));

    clear();

    int angles = player->mdat.angles;
    if (angles > 1) {
        nextAngleAct->setEnabled(player->statePOP());
        int angle = player->mdat.angle;
        for (int n = 1; n <= angles; n++) {
            QAction *a = new QAction(this);
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
        QAction* a = new QAction(this);
        a->setCheckable(true);
        a->setText("1");
        a->setData(1);
        a->setChecked(true);
        a->setEnabled(false);
    }

    emit anglesChanged(this);
}

TMenuBrowse::TMenuBrowse(QWidget* parent, TMainWindow* mw)
    : TMenu(parent, mw, "browse_menu", tr("Browse"), "noicon") {

    // Titles
    titlesMenu = new TMenu(this, mw, "titles_menu", tr("Title"), "title");
    addMenu(titlesMenu);

    TTitleGroup* titleGroup = mw->findChild<TTitleGroup*>();
    connect(titleGroup, &TTitleGroup::titleTracksChanged,
            this, &TMenuBrowse::updateTitles);

    // Chapters
    chaptersMenu = new TMenu(this, mw, "chapters_menu", tr("Chapter"),
                             "chapter");
    chaptersMenu->addAction(mw->findAction("next_chapter"));
    chaptersMenu->addAction(mw->findAction("prev_chapter"));
    chaptersMenu->addSeparator();
    addMenu(chaptersMenu);

    TChapterGroup* chapterGroup = mw->findChild<TChapterGroup*>();
    connect(chapterGroup, &TChapterGroup::chaptersChanged,
            this, &TMenuBrowse::updateChapters);

    // Angles submenu
    anglesMenu = new TMenu(this, mw, "angles_menu", tr("Angle"), "angle");
    anglesMenu->addAction(mw->findAction("next_angle"));
    anglesMenu->addSeparator();
    addMenu(anglesMenu);

    TAngleGroup* angleGroup = mw->findChild<TAngleGroup*>();
    connect(angleGroup, &TAngleGroup::anglesChanged,
            this, &TMenuBrowse::updateAngles);

    addSeparator();
    // DVDNAV
    addAction(mw->findAction("dvdnav_up"));
    addAction(mw->findAction("dvdnav_down"));
    addAction(mw->findAction("dvdnav_left"));
    addAction(mw->findAction("dvdnav_right"));

    addSeparator();
    addAction(mw->findAction("dvdnav_select"));
    // dvdnav_mouse Not in menu
    addAction(mw->findAction("dvdnav_menu"));
    addAction(mw->findAction("dvdnav_prev"));
}

void TMenuBrowse::updateTitles(TTitleGroup* titleGroup) {
    titlesMenu->addActions(titleGroup->actions());
}

void TMenuBrowse::updateChapters(TChapterGroup* chapterGroup) {
    chaptersMenu->addActions(chapterGroup->actions());
}

void TMenuBrowse::updateAngles(TAngleGroup* angleGroup) {
    anglesMenu->addActions(angleGroup->actions());
}

} // namespace Menu
} // namespace Action
} // namespace Gui
