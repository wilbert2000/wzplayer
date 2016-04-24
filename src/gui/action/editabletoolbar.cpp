/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "gui/action/editabletoolbar.h"

#include <QDebug>
#include <QMenu>
#include <QResizeEvent>
#include <QTimer>

#include "desktop.h"
#include "settings/preferences.h"
#include "gui/base.h"
#include "gui/action/actionlist.h"
#include "gui/action/actionseditor.h"
#include "gui/action/toolbareditor.h"
#include "gui/action/menu.h"
#include "gui/action/sizegrip.h"
#include "gui/action/timeslider.h"


namespace Gui {
namespace Action {


TEditableToolbar::TEditableToolbar(TBase* mainwindow) :
    QToolBar(mainwindow),
    main_window(mainwindow),
    size_grip(0),
    space_eater(0),
    fixing_size(false) {

	fix_size = height() - iconSize().height();

	// Context menu
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
			this, SLOT(showContextMenu(const QPoint&)));

	// Update size grip when top level changes
	connect(this, SIGNAL(topLevelChanged(bool)),
			this, SLOT(onTopLevelChanged(bool)));

	// Reload toolbars when entering and exiting fullscreen
	connect(main_window, SIGNAL(didEnterFullscreenSignal()), this, SLOT(reload()));
	connect(main_window, SIGNAL(didExitFullscreenSignal()), this, SLOT(reload()));
}

TEditableToolbar::~TEditableToolbar() {
}

void TEditableToolbar::addMenu(QAction* action) {

    // Create button with menu
    QToolButton* button = new QToolButton();
    button->setObjectName(action->objectName() + "_toolbutton");
    QMenu* menu = action->menu();
    button->setMenu(menu);

    // Set popupmode and default action
    if (action->objectName() == "stay_on_top_menu") {
        button->setPopupMode(QToolButton::MenuButtonPopup);
        button->setDefaultAction(menu->defaultAction());
    } else if (action->objectName() == "forward_menu"
        || action->objectName() == "rewind_menu") {
        button->setPopupMode(QToolButton::MenuButtonPopup);
        button->setDefaultAction(menu->defaultAction());
        // Set triggered action as default action
        connect(menu, SIGNAL(triggered(QAction*)),
                button, SLOT(setDefaultAction(QAction*)));
        // Show menu when action disabled
        connect(action, SIGNAL(triggered()),
                button, SLOT(showMenu()),
                Qt::QueuedConnection);
    } else {
        // Default, use instant popup
        button->setPopupMode(QToolButton::InstantPopup);
        button->setDefaultAction(action);
    }

    addWidget(button);
}

void TEditableToolbar::setActionsFromStringList(const QStringList& acts, const TActionList& all_actions) {
	qDebug() << "Gui::Action::TEditableToolbar::setActionsFromStringList: loading toolbar" << objectName();

	clear();
	space_eater = 0;
	removeSizeGrip();
	// Copy actions
	actions = acts;

	int i = 0;
	while (i < actions.count()) {

		QString action_name;
		bool ns, fs;
		TToolbarEditor::stringToAction(actions[i], action_name, ns, fs);
		if (action_name.isEmpty()) {
			qWarning() << "Gui::Action::TEditableToolbar::setActionsFromStringList: malformed action"
					   << actions[i] << "at pos" << i;
			actions.removeAt(i);
		} else {
			if (Settings::pref->fullscreen ? fs : ns) {
				if (action_name == "separator") {
					addAction(TToolbarEditor::newSeparator(this));
				} else {
					QAction* action = TToolbarEditor::findAction(action_name, all_actions);
					if (action) {
                        if (action_name.endsWith("_menu")) {
                            addMenu(action);
                        } else {
                            addAction(action);
                            if (action_name == "timeslider_action") {
                                space_eater = qobject_cast<TTimeSlider*>(widgetForAction(action));
                            }
                        }
					} else {
						qWarning() << "Gui::Action::TEditableToolbar::setActionsFromStringList: action"
								   << action_name << "not found";
						actions.removeAt(i);
						i--;
					}
				}
            } // if (visible)

			i++;
		}
	} // while

	addSizeGrip();
} // TEditableToolbar::setActionsFromStringList()

QStringList TEditableToolbar::actionsToStringList(bool remove_size_grip) {

	if (remove_size_grip)
		removeSizeGrip();
	return actions;
}

void TEditableToolbar::edit() {
	qDebug("Gui::Action::TEditableToolbar::edit");

	// Create toolbar editor dialog
	TActionList all_actions = main_window->getAllNamedActions();
	TToolbarEditor editor(main_window);
	editor.setAllActions(all_actions);
	editor.setActiveActions(actions);
	editor.setDefaultActions(default_actions);
	editor.setIconSize(iconSize().width());

    // Execute
	if (editor.exec() == QDialog::Accepted) {
		// Get action names and update actions in all_actions
		QStringList new_actions = editor.saveActions();
		// Load new actions
		setActionsFromStringList(new_actions, all_actions);
        // Update icon size
        setIconSize(QSize(editor.iconSize(), editor.iconSize()));
        // Save modified icon texts to pref
        TActionsEditor::saveToConfig(Settings::pref, main_window);
        Settings::pref->sync();
	}
}

void TEditableToolbar::reload() {

	TActionList all_actions = main_window->getAllNamedActions();
	setActionsFromStringList(actions, all_actions);
}

void TEditableToolbar::showContextMenu(const QPoint& pos) {
	//qDebug("Gui::Action::TEditableToolbar::showContextMenu: x: %d y: %d", pos.x(), pos.y());

	QMenu* popup = main_window->getToolbarMenu();
	if (popup) {
		execPopup(this, popup, mapToGlobal(pos));
	}
}

void TEditableToolbar::mouseReleaseEvent(QMouseEvent* event) {

    QToolBar::mouseReleaseEvent(event);
    if (size_grip) {
        QTimer::singleShot(1000, size_grip, SLOT(delayedShow()));
    }
}

void TEditableToolbar::moveEvent(QMoveEvent* event) {
	//qDebug("Gui::Action::TEditableToolbar::moveEvent");

	QToolBar::moveEvent(event);
    if (size_grip) {
        if (QApplication::mouseButtons()) {
            size_grip->hide();
        }
        size_grip->follow();
    }
}

void TEditableToolbar::resizeEvent(QResizeEvent* event) {
    //qDebug() << "Gui::Action::TEditableToolbar::resizeEvent:" << objectName()
    //         << "from" << event->oldSize() << "to" << size()
    //         << "min" << minimumSizeHint();

	QToolBar::resizeEvent(event);

	// Fix the dark and uncontrollable ways of Qt's layout engine.
	// It looks like that with an orientation change the resize is done first,
	// than the orientation changed signal is sent and received by TTImeslider
    // changing TTImesliders minimum size and then another resize arrives,
    // based on the old minimum size hint from before the orientation change.
    // sizeHint() is never called, so can't fix it that way.
    if (isFloating() && space_eater && !fixing_size) {
		if (orientation() == Qt::Horizontal) {
            if (height() > iconSize().height() + fix_size) {
				qDebug() << "Gui::Action::TEditableToolbar::resizeEvent: fixing height";
				fixing_size = true;
				resize(width(), iconSize().height() + fix_size);
				fixing_size = false;
			}
        } else if (width() > iconSize().width() + fix_size) {
			qDebug() << "Gui::Action::TEditableToolbar::resizeEvent: fixing width";
			fixing_size = true;
			resize(iconSize().width() + fix_size, height());
			fixing_size = false;
		}
	}

    if (size_grip) {
        size_grip->follow();
    }
}

void TEditableToolbar::setVisible(bool visible) {
	//qDebug("TEditableToolbar::setVisible: %d", visible);

	QToolBar::setVisible(visible);
	if (size_grip && (!visible || underMouse())) {
		size_grip->setVisible(visible);
	}
}

void TEditableToolbar::removeSizeGrip() {

	if (size_grip) {
		//qDebug("Gui::Action::TEditableToolbar::removeSizeGrip: removing size grip");
		size_grip->close();
		delete size_grip;
		size_grip = 0;
	}
}

void TEditableToolbar::addSizeGrip() {

	if (space_eater && isFloating()) {
        setMaximumSize(0.9 * TDesktop::availableSize(this));
        if (!size_grip) {
			//qDebug() << "Gui::Action::TEditableToolbar::addSizeGrip: adding size grip";
			size_grip = new TSizeGrip(main_window, this);
			connect(size_grip, SIGNAL(saveSizeHint()),
					space_eater, SLOT(saveSizeHint()));
		}
	} else {
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
		removeSizeGrip();
	}
}

void TEditableToolbar::onTopLevelChanged(bool) {

    addSizeGrip();
}

void TEditableToolbar::enterEvent(QEvent* event) {
	//qDebug() << "TEditableToolbar::enterEvent";

	QToolBar::enterEvent(event);
	if (size_grip)
		size_grip->show();
}

void TEditableToolbar::leaveEvent(QEvent* event) {
	//qDebug() << "TEditableToolbar::leaveEvent";

	QToolBar::leaveEvent(event);
	if (size_grip)
		QTimer::singleShot(0, size_grip, SLOT(delayedHide()));
}

} // namespace Action
} // namespace Gui

#include "moc_editabletoolbar.cpp"

