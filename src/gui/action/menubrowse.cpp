#include "gui/action/menubrowse.h"
#include <QWidget>
#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "core.h"


using namespace Settings;

namespace Gui {
namespace Action {


TMenuBrowse::TMenuBrowse(QWidget* parent, TCore* c)
    : TMenu(parent, "browse_menu", tr("&Browse"), "noicon")
	, core(c) {

	// Titles
    titlesMenu = new TMenu(parent, "titles_menu", tr("&Title"), "title");
	addMenu(titlesMenu);
	titleGroup = new TActionGroup(this, "title");
	connect(titleGroup, SIGNAL(activated(int)), core, SLOT(changeTitle(int)));
	connect(core, SIGNAL(titleTrackChanged(int)), titleGroup, SLOT(setChecked(int)));
	connect(core, SIGNAL(titleTracksChanged()), this, SLOT(updateTitles()));

	// Chapters
	prevChapterAct = new TAction(this, "prev_chapter", tr("Previous chapter"), "", Qt::Key_AsciiTilde, false);
	parent->addAction(prevChapterAct);
	connect(prevChapterAct, SIGNAL(triggered()), core, SLOT(prevChapter()));
	nextChapterAct = new TAction(this, "next_chapter", tr("Next chapter"), "", Qt::Key_Exclam, false);
	parent->addAction(nextChapterAct);
	connect(nextChapterAct, SIGNAL(triggered()), core, SLOT(nextChapter()));

    chaptersMenu = new TMenu(parent, "chapters_menu", tr("&Chapter"), "chapter");
	chaptersMenu->addAction(prevChapterAct);
	chaptersMenu->addAction(nextChapterAct);
	chaptersMenu->addSeparator();
	addMenu(chaptersMenu);

	chapterGroup = new TActionGroup(this, "chapter");
	connect(chapterGroup, SIGNAL(activated(int)), core, SLOT(changeChapter(int)));
	connect(core, SIGNAL(chapterChanged(int)), chapterGroup, SLOT(setChecked(int)));
	// Update normally done by updateTitles. For DVDNAV only:
	connect(core, SIGNAL(chaptersChanged()), this, SLOT(updateChapters()));

	// Angles submenu
	nextAngleAct = new TAction(this, "next_angle", tr("Next angle"), "", Qt::Key_At, false);
	connect(nextAngleAct, SIGNAL(triggered()), core, SLOT(nextAngle()));
    anglesMenu = new TMenu(parent, "angles_menu", tr("&Angle"), "angle");
	anglesMenu->addAction(nextAngleAct);
	anglesMenu->addSeparator();
	addMenu(anglesMenu);
	angleGroup = new TActionGroup(this, "angle");
	connect(angleGroup, SIGNAL(activated(int)), core, SLOT(setAngle(int)));
	// Update normally done by updateTitles. For DVDNAV only:
	connect(core, SIGNAL(anglesChanged()), this, SLOT(updateAngles()));

#if PROGRAM_SWITCH
	programMenu = new TMenu(parent, this, "program_menu", tr("P&rogram"), "program");
	addMenu(programMenu);
	programGroup = new TActionGroup(this, "program");
	connect(programGroup, SIGNAL(activated(int)), core, SLOT(changeProgram(int)));
	// TODO: update
#endif

	addSeparator();

	// DVDNAV
	dvdnavUpAct = new TAction(this, "dvdnav_up", tr("DVD, move up"),
							  "", Qt::SHIFT | Qt::Key_Up);
	connect(dvdnavUpAct, SIGNAL(triggered()), core, SLOT(dvdnavUp()));
	dvdnavDownAct = new TAction(this, "dvdnav_down", tr("DVD, move down"),
								"", Qt::SHIFT | Qt::Key_Down);
	connect(dvdnavDownAct, SIGNAL(triggered()), core, SLOT(dvdnavDown()));
	dvdnavLeftAct = new TAction(this, "dvdnav_left", tr("DVD, move left"),
								"", Qt::SHIFT | Qt::Key_Left);
	connect(dvdnavLeftAct, SIGNAL(triggered()), core, SLOT(dvdnavLeft()));
	dvdnavRightAct = new TAction(this, "dvdnav_right", tr("DVD, move right"),
								 "", Qt::SHIFT | Qt::Key_Right);
	connect(dvdnavRightAct, SIGNAL(triggered()), core, SLOT(dvdnavRight()));

	addSeparator();

	dvdnavSelectAct = new TAction(this, "dvdnav_select", tr("DVD, select"),
								  "", Qt::Key_Return);
	connect(dvdnavSelectAct, SIGNAL(triggered()), core, SLOT(dvdnavSelect()));

	dvdnavMouseAct = new TAction(this, "dvdnav_mouse", tr("DVD, mouse click"),
								 "", Qt::CTRL | Qt::Key_Return);
	connect(dvdnavMouseAct, SIGNAL(triggered()), core, SLOT(dvdnavMouse()));

	addSeparator();

	dvdnavMenuAct = new TAction(this, "dvdnav_menu", tr("DVD &menu"),
								"", Qt::SHIFT | Qt::Key_Return);
	connect(dvdnavMenuAct, SIGNAL(triggered()), core, SLOT(dvdnavMenu()));

	dvdnavPrevAct = new TAction(this, "dvdnav_prev", tr("DVD &previous menu"),
								"", Qt::SHIFT | Qt::Key_Escape);
	connect(dvdnavPrevAct, SIGNAL(triggered()), core, SLOT(dvdnavPrev()));

	addActionsTo(parent);
}

void TMenuBrowse::enableActions(bool stopped, bool, bool) {

	bool enableChapters = !stopped && core->mdat.chapters.count() > 0;
	prevChapterAct->setEnabled(enableChapters);
	nextChapterAct->setEnabled(enableChapters);

	nextAngleAct->setEnabled(!stopped && core->mdat.angles > 1);

	bool enableDVDNav = !stopped && core->mdat.detected_type == TMediaData::TYPE_DVDNAV;
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
	qDebug("Gui::Action::TMenuBrowse::updateTitles");

	titleGroup->clear();
	if (core->mdat.titles.count() == 0) {
		QAction* a = titleGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	} else {
		int selected_ID = core->mdat.titles.getSelectedID();
		Maps::TTitleTracks::TTitleTrackIterator i = core->mdat.titles.getIterator();
		while (i.hasNext()) {
			i.next();
			Maps::TTitleData title = i.value();
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
	qDebug("Gui::Action::TMenuBrowse::updateChapters");

	chapterGroup->clear();
	if (core->mdat.chapters.count() > 0) {
		int selected_id = core->mdat.chapters.getSelectedID();
		Maps::TChapters::TChapterIterator i = core->mdat.chapters.getIterator();
		do {
			i.next();
			const Maps::TChapterData chapter = i.value();
			QAction *a = new QAction(chapterGroup);
			a->setCheckable(true);
			a->setText(chapter.getDisplayName());
			a->setData(chapter.getID());
			if (chapter.getID() == selected_id) {
				a->setChecked(true);
			}
		} while (i.hasNext());
	} else {
		QAction* a = chapterGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	}

	chaptersMenu->addActions(chapterGroup->actions());
}

void TMenuBrowse::updateAngles() {
	qDebug("Gui::Action::TMenuBrowse::updateAngels");

	angleGroup->clear();

	// Add angles to menu
	int angles = core->mdat.angles;
	if (angles > 1) {
		nextAngleAct->setEnabled(true);
		int angle = core->mdat.angle;
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

} // namespace Action
} // namespace Gui
