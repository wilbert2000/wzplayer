#include "gui/action/browsemenu.h"
#include <QWidget>
#include "gui/action/actiongroup.h"
#include "settings/preferences.h"
#include "core.h"


using namespace Settings;

namespace Gui {

TBrowseMenu::TBrowseMenu(QWidget* parent, TCore* c)
	: TMenu(parent, this, "browse_menu", QT_TR_NOOP("&Browse"), "noicon")
	, core(c) {

	// Titles submenu
	titlesMenu = new TMenu(parent, this, "titles_menu", QT_TR_NOOP("&Title"), "title");
	addMenu(titlesMenu);
	titleGroup = new TActionGroup(this, "title");
	connect(titleGroup, SIGNAL(activated(int)), core, SLOT(changeTitle(int)));
	connect(core, SIGNAL(titleTrackChanged(int)), titleGroup, SLOT(setChecked(int)));
	connect(core, SIGNAL(titleTrackInfoChanged()), this, SLOT(updateTitles()));

	// Chapters submenu
	chaptersMenu = new TMenu(parent, this, "chapters_menu", QT_TR_NOOP("&Chapter"), "chapter");
	addMenu(chaptersMenu);
	chapterGroup = new TActionGroup(this, "chapter");
	connect(chapterGroup, SIGNAL(activated(int)), core, SLOT(changeChapter(int)));
	connect(core, SIGNAL(chapterChanged(int)), chapterGroup, SLOT(setChecked(int)));
	// Update done by updateTitles. For DVDNAV only:
	connect(core, SIGNAL(chapterInfoChanged()), this, SLOT(updateChapters()));

	// Angles submenu
	anglesMenu = new TMenu(parent, this, "angles_menu", QT_TR_NOOP("&Angle"), "angle");
	addMenu(anglesMenu);
	angleGroup = new TActionGroup(this, "angle");
	connect(angleGroup, SIGNAL(activated(int)), core, SLOT(changeAngle(int)));
	// Update done by updateTitles

#if PROGRAM_SWITCH
	programMenu = new TMenu(parent, this, "program_menu", QT_TR_NOOP("P&rogram"), "program");
	addMenu(programMenu);
	programGroup = new TActionGroup(this, "program");
	connect(programGroup, SIGNAL(activated(int)), core, SLOT(changeProgram(int)));
	// TODO: update
#endif

	// DVDNAV
	if (pref->isMPlayer())
		addSeparator();
	dvdnavUpAct = new TAction(this, "dvdnav_up", QT_TR_NOOP("DVD menu, move up"),
							  "", Qt::SHIFT | Qt::Key_Up, false);
	parent->addAction(dvdnavUpAct);
	connect(dvdnavUpAct, SIGNAL(triggered()), core, SLOT(dvdnavUp()));

	dvdnavDownAct = new TAction(this, "dvdnav_down", QT_TR_NOOP("DVD menu, move down"),
								"", Qt::SHIFT | Qt::Key_Down, false);
	parent->addAction(dvdnavDownAct);
	connect(dvdnavDownAct, SIGNAL(triggered()), core, SLOT(dvdnavDown()));

	dvdnavLeftAct = new TAction(this, "dvdnav_left", QT_TR_NOOP("DVD menu, move left"),
								"", Qt::SHIFT | Qt::Key_Left, false);
	parent->addAction(dvdnavLeftAct);
	connect(dvdnavLeftAct, SIGNAL(triggered()), core, SLOT(dvdnavLeft()));

	dvdnavRightAct = new TAction(this, "dvdnav_right", QT_TR_NOOP("DVD menu, move right"),
								 "", Qt::SHIFT | Qt::Key_Right, false);
	parent->addAction(dvdnavRightAct);
	connect(dvdnavRightAct, SIGNAL(triggered()), core, SLOT(dvdnavRight()));


	dvdnavMenuAct = new TAction(this, "dvdnav_menu", QT_TR_NOOP("DVD &menu"),
								"", Qt::SHIFT | Qt::Key_Return, pref->isMPlayer());
	connect(dvdnavMenuAct, SIGNAL(triggered()), core, SLOT(dvdnavMenu()));

	dvdnavPrevAct = new TAction(this, "dvdnav_prev", QT_TR_NOOP("DVD &previous menu"),
								"", Qt::SHIFT | Qt::Key_Escape, pref->isMPlayer());
	connect(dvdnavPrevAct, SIGNAL(triggered()), core, SLOT(dvdnavPrev()));

	dvdnavSelectAct = new TAction(this, "dvdnav_select", QT_TR_NOOP("DVD menu, select option"),
								  "", Qt::Key_Return, pref->isMPlayer());
	connect(dvdnavSelectAct, SIGNAL(triggered()), core, SLOT(dvdnavSelect()));

	dvdnavMouseAct = new TAction(this, "dvdnav_mouse", QT_TR_NOOP("DVD menu, mouse click"),
								 "", false);
	parent->addAction(dvdnavMouseAct);
	connect(dvdnavMouseAct, SIGNAL(triggered()), core, SLOT(dvdnavMouse()));

	addActionsTo(parent);
}

void TBrowseMenu::enableActions(bool stopped, bool, bool) {

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

void TBrowseMenu::updateTitles() {
	qDebug("Gui::TBrowseMenu::updateTitles");

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

void TBrowseMenu::updateChapters() {
	qDebug("Gui::TBrowseMenu::updateChapters");

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

void TBrowseMenu::updateAngles() {
	qDebug("Gui::TBrowseMenu::updateAngels");

	angleGroup->clear();
	int n_angles = 0;
	int sel_title_id = core->mdat.titles.getSelectedID();
	if (sel_title_id >= 0) {
		Maps::TTitleData title = core->mdat.titles.value(sel_title_id);
		n_angles = title.getAngles();
	}
	if (n_angles > 0) {
		for (int n = 1; n <= n_angles; n++) {
			QAction *a = new QAction(angleGroup);
			a->setCheckable(true);
			a->setText(QString::number(n));
			a->setData(n);
		}
	} else {
		QAction* a = angleGroup->addAction(tr("<empty>"));
		a->setEnabled(false);
	}
	anglesMenu->addActions(angleGroup->actions());
}

} // namespace Gui
