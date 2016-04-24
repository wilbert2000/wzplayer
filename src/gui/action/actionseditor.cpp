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

/* This is based on qq14-actioneditor-code.zip from Qt */


#include "gui/action/actionseditor.h"

#include <QDebug>
#include <QTableWidget>
#include <QHeaderView>

#include <QString>
#include <QAction>
#include <QLayout>
#include <QScrollBar>
#include <QPushButton>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileInfo>
#include <QRegExp>
#include <QApplication>
#include <QTimer>
#include <QResizeEvent>

#include "images.h"
#include "filedialog.h"
#include "settings/paths.h"

#include "gui/action/shortcutgetter.h"


/*
#include <QLineEdit>
#include <QItemDelegate>

class MyDelegate : public QItemDelegate 
{
public:
	MyDelegate(QObject *parent = 0);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;
		virtual void setModelData(QWidget* editor, QAbstractItemModel* model, 
                              const QModelIndex & index) const;
};

MyDelegate::MyDelegate(QObject *parent) : QItemDelegate(parent)
{
}

static QString old_accel_text;

QWidget* MyDelegate::createEditor(QWidget *parent, 
								   const QStyleOptionViewItem & option,
	                               const QModelIndex & index) const
{
	qDebug("MyDelegate::createEditor");

	old_accel_text = index.model()->data(index, Qt::DisplayRole).toString();
	//qDebug("text: %s", old_accel_text.toUtf8().data());
	
	return QItemDelegate::createEditor(parent, option, index);
}

void MyDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                              const QModelIndex &index) const
{
	QLineEdit *line_edit = static_cast<QLineEdit*>(editor);

	QString accelText = QKeySequence(line_edit->text()).toString();
	if (accelText.isEmpty() && !line_edit->text().isEmpty()) {
		model->setData(index, old_accel_text);
	}
	else {
		model->setData(index, accelText);
	}
}
*/

namespace Gui {
namespace Action {

const int WIDTH_CONFLICT_ICON = 16;
const int MARGINS = 2;

TActionsEditor::TActionsEditor(QWidget* parent, Qt::WindowFlags f) :
	QWidget(parent, f) {

	latest_dir = Settings::TPaths::shortcutsPath();

	actionsTable = new QTableWidget(0, COL_COUNT, this);
	actionsTable->verticalHeader()->hide();
    actionsTable->setColumnWidth(COL_CONFLICTS, WIDTH_CONFLICT_ICON + MARGINS);

#if QT_VERSION_MAJOR >= 5
	actionsTable->horizontalHeader()->setSectionResizeMode(COL_CONFLICTS, QHeaderView::Fixed);
#else
	actionsTable->horizontalHeader()->setResizeMode(COL_CONFLICTS, QHeaderView::Fixed);
#endif

	actionsTable->horizontalHeader()->setStretchLastSection(true);

    actionsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    actionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	actionsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(actionsTable, SIGNAL(itemActivated(QTableWidgetItem*)),
			this, SLOT(editShortcut()));

	saveButton = new QPushButton(this);
	loadButton = new QPushButton(this);

	connect(saveButton, SIGNAL(clicked()), this, SLOT(saveActionsTable()));
	connect(loadButton, SIGNAL(clicked()), this, SLOT(loadActionsTable()));

	editButton = new QPushButton(this);
	connect(editButton, SIGNAL(clicked()), this, SLOT(editShortcut()));

	QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(16, 8, 16, 0);
	buttonLayout->setSpacing(6);
	buttonLayout->addWidget(editButton);
	buttonLayout->addStretch(1);
	buttonLayout->addWidget(loadButton);
	buttonLayout->addWidget(saveButton);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setMargin(0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(actionsTable);
	mainLayout->addLayout(buttonLayout);

	retranslateStrings();
}

TActionsEditor::~TActionsEditor() {
}

void TActionsEditor::retranslateStrings() {

	actionsTable->setHorizontalHeaderLabels(QStringList() << "" <<
		tr("Action") << tr("Description") << tr("Shortcuts"));

	saveButton->setText(tr("&Save"));
	saveButton->setIcon(Images::icon("save"));

	loadButton->setText(tr("&Load"));
	loadButton->setIcon(Images::icon("open"));

	editButton->setText(tr("&Change shortcut..."));

	//updateView(); // The actions are translated later, so it's useless
}

void TActionsEditor::clear() {
	actionsList.clear();
}

void TActionsEditor::addActions(QWidget* widget) {

	TActionList actions = widget->findChildren<QAction *>();
	for (int n = 0; n < actions.count(); n++) {
		QAction* action = actions[n];
		if (!action->isSeparator()
			&& !action->objectName().isEmpty()
			&& !action->inherits("QWidgetAction")
			&& (action->menu() == 0))
			actionsList.append(action);
	}

	updateView();
}

void TActionsEditor::resizeColumns() {

    int w = (width() - WIDTH_CONFLICT_ICON - MARGINS * actionsTable->columnCount())
			/ (actionsTable->columnCount() - 1);

	for (int col = 1; col < actionsTable->columnCount() - 1; col++) {
		actionsTable->setColumnWidth(col, w);
	}

	actionsTable->resizeRowsToContents();
}

void TActionsEditor::resizeEvent(QResizeEvent* event) {
	qDebug("Gui::Action::TToolbarEditor::resizeEvent %d", event->spontaneous());

	QWidget::resizeEvent(event);
	resizeColumns();
}

void TActionsEditor::updateView() {

	actionsTable->setRowCount(actionsList.count());

	for (int n = 0; n < actionsList.count(); n++) {
		QAction* action = actionsList[n];

		// Conflict column
        QTableWidgetItem* i = new QTableWidgetItem(
            action->property("modified").toBool() ? "m" : "");
        i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        actionsTable->setItem(n, COL_CONFLICTS, i);

        // Action column
        i = new QTableWidgetItem(action->objectName());
        i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        actionsTable->setItem(n, COL_ACTION, i);

        // Desc column
        i = new QTableWidgetItem(actionTextToDescription(
            action->text(), action->objectName()));
        i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        i->setIcon(action->icon());
        actionsTable->setIconSize(QSize(32, 32));

        actionsTable->setItem(n, COL_DESC, i);

        // Shortcut column
        i = new QTableWidgetItem(shortcutsToString(action->shortcuts()));
        i->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        actionsTable->setItem(n, COL_SHORTCUT, i);
	}

	hasConflicts(); // Check for conflicts

	actionsTable->setCurrentCell(0, COL_SHORTCUT);
	resizeColumns();
}

void TActionsEditor::applyChanges() {
	qDebug("Gui::Action::TActionsEditor::applyChanges");

	for (int row = 0; row < (int)actionsList.size(); ++row) {
		QAction* action = actionsList[row];
		QTableWidgetItem* i = actionsTable->item(row, COL_SHORTCUT);
        if (i) {
            QString shortcuts = i->text();
            QString old_shortcuts = shortcutsToString(action->shortcuts());
            if (shortcuts != old_shortcuts) {
                action->setShortcuts(stringToShortcuts(shortcuts));
                action->setProperty("modified", true);
            }
        }
	}
}

void TActionsEditor::editShortcut() {

	QTableWidgetItem* i = actionsTable->item(actionsTable->currentRow(), COL_SHORTCUT);
	if (i) {
		TShortcutGetter d(this);
		QString result = d.exec(i->text());
        if (d.result() == QDialog::Accepted) {
            // Just to make sure shortcutgetter and actionseditor
            // speak the same language...
            result = shortcutsToString(stringToShortcuts(result));
            i->setText(result);
			if (hasConflicts())
				qApp->beep();
		}
	}
}

int TActionsEditor::findActionName(const QString& name) {

	for (int row = 0; row < actionsTable->rowCount(); row++) {
		if (actionsTable->item(row, COL_ACTION)->text() == name)
			return row;
	}
	return -1;
}

bool TActionsEditor::containsShortcut(const QString& accel, const QString& shortcut) {

	QStringList shortcut_list = accel.split(", ");
	QString s;
	foreach(s, shortcut_list) {
		s = s.trimmed();
		if (s == shortcut)
			return true;
	}
	return false;
}

int TActionsEditor::findActionAccel(const QString& accel, int ignoreRow) {

	QStringList shortcuts = accel.split(", ");
	QString shortcut;

	for (int row = 0; row < actionsTable->rowCount(); row++) {
		QTableWidgetItem* i = actionsTable->item(row, COL_SHORTCUT);
		if (i && row != ignoreRow) {
			if (!i->text().isEmpty()) {
				foreach(shortcut, shortcuts) {
					if (containsShortcut(i->text(), shortcut.trimmed())) {
						return row;
					}
				}
			}
		}
	}
	return -1;
}

bool TActionsEditor::hasConflicts() {

	int found;
	bool conflict = false;

	QString accelText;
	QTableWidgetItem *i;

	for (int n = 0; n < actionsTable->rowCount(); n++) {
		i = actionsTable->item(n, COL_CONFLICTS);
        if (i) {
			i->setIcon(QPixmap());
        }
		i = actionsTable->item(n, COL_SHORTCUT);
		if (i) {
			accelText = i->text();
			if (!accelText.isEmpty()) {
				found = findActionAccel(accelText, n);
                if (found >= 0) {
					conflict = true;
					actionsTable->item(n, COL_CONFLICTS)->setIcon(Images::icon("conflict"));
				}
			}
		}
	}

	return conflict;
}

void TActionsEditor::saveActionsTable() {
	QString s = MyFileDialog::getSaveFileName(
					this, tr("Choose a filename"),
					latest_dir,
					tr("Key files") +" (*.keys)");

	if (!s.isEmpty()) {
		// If filename has no extension, add it
		if (QFileInfo(s).suffix().isEmpty()) {
			s = s + ".keys";
		}
		if (QFileInfo(s).exists()) {
			int res = QMessageBox::question(this,
					tr("Confirm overwrite?"),
					tr("The file %1 already exists.\n"
					   "Do you want to overwrite?").arg(s),
					QMessageBox::Yes,
					QMessageBox::No,
					Qt::NoButton);
			if (res == QMessageBox::No) {
				return;
			}
		}

		latest_dir = QFileInfo(s).absolutePath();
		bool r = saveActionsTable(s);
		if (!r) {
			QMessageBox::warning(this, tr("Error"), 
				tr("The file couldn't be saved"),
				QMessageBox::Ok, Qt::NoButton);
		}
	}
}

bool TActionsEditor::saveActionsTable(const QString & filename) {
	qDebug() << "Gui::Action::TActionsEditor::saveActions:" << filename;

	QFile f(filename);
	if (f.open(QIODevice::WriteOnly)) {
		QTextStream stream(&f);
		stream.setCodec("UTF-8");

		for (int row = 0; row < actionsTable->rowCount(); row++) {
			stream << actionsTable->item(row, COL_ACTION)->text() << "\t"
				   << actionsTable->item(row, COL_SHORTCUT)->text() << "\n";
		}
		f.close();
		return true;
	}
	return false;
}

void TActionsEditor::loadActionsTable() {
	QString s = MyFileDialog::getOpenFileName(
					this, tr("Choose a file"),
					latest_dir, tr("Key files") +" (*.keys)");

	if (!s.isEmpty()) {
		latest_dir = QFileInfo(s).absolutePath();
		bool r = loadActionsTable(s);
		if (!r) {
			QMessageBox::warning(this, tr("Error"), 
				tr("The file couldn't be loaded"),
				QMessageBox::Ok, Qt::NoButton);
		}
	}
}

bool TActionsEditor::loadActionsTable(const QString& filename) {
	qDebug() << "Gui::Action::TActionsEditor::loadActions:" << filename;

    QRegExp rx("^([^\\t]*)\\t(.*)");

	QFile f(filename);
	if (f.open(QIODevice::ReadOnly)) {
		QTextStream stream(&f);
		stream.setCodec("UTF-8");
		while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (rx.indexIn(line) >= 0) {
                QString name = rx.cap(1).trimmed();
                int row = findActionName(name);
                if (row >= 0) {
                    QString shortcuts = shortcutsToString(stringToShortcuts(rx.cap(2)));
                    actionsTable->item(row, COL_SHORTCUT)->setText(shortcuts);
                } else {
                    qWarning() << "Gui::Action::TActionsEditor::loadActions: action"
                               << name << "not found";
                }
			} else {
                qDebug() << "Gui::Action::TActionsEditor::loadActions: skipped line"
                         << line;
			}
		}
		f.close();
		hasConflicts(); // Check for conflicts
		return true;
	} else {
		return false;
	}
}


// Static functions

QString TActionsEditor::keyToString(QKeySequence key) {
    return key.toString(QKeySequence::PortableText);
}

QKeySequence TActionsEditor::stringToKey(QString s) {

    s = s.simplified();

    // Qt 4.3 and 4.4 (at least on linux) seems to have a problem when
    // using Simplified-Chinese. QKeysequence deletes the arrow key names
    // from the shortcut, so this is a work-around.
    s.replace(QString::fromUtf8("左"), "Left");
    s.replace(QString::fromUtf8("下"), "Down");
    s.replace(QString::fromUtf8("右"), "Right");
    s.replace(QString::fromUtf8("上"), "Up");

    return QKeySequence(s, QKeySequence::PortableText);
}

QString TActionsEditor::shortcutsToString(const TShortCutList& shortcuts) {

	QString s = "";
	for (int n = 0; n < shortcuts.count(); n++) {
        s += keyToString(shortcuts[n]);
		if (n < shortcuts.count() - 1)
			s += ", ";
	}

	return s;
}

TShortCutList TActionsEditor::stringToShortcuts(const QString& shortcuts) {

	TShortCutList shortcut_list;
    QStringList l = shortcuts.split(", ", QString::SkipEmptyParts);

	for (int n = 0; n < l.count(); n++) {
        shortcut_list.append(stringToKey(l[n]));
	}

	return shortcut_list;
}

QString TActionsEditor::actionTextToDescription(const QString& text,
                                                const QString& action_name) {

    // Actions modifying text property themselves
    if (action_name == "display_time") {
		return tr("Display time");
	}
    if (action_name == "pl_play_or_pause") {
        return tr("Play or pause");
    }
    if (action_name == "aspect_detect") {
        return tr("Auto");
    }
    if (action_name == "aspect_none") {
        return tr("Disabled");
    }
    if (action_name == "video_size") {
        return tr("Optimize size");
    }

	QString s = text;
	s = s.replace("&", "");
    s = s.replace(".", ""); // To remove ...
    s = s.replace("\t", " "); // Aspect menu used tabs
	return s;
}

QString TActionsEditor::actionToString(QAction *action) {

    QString s = shortcutsToString(action->shortcuts());
    QString action_text = actionTextToDescription(action->text(),
                                                  action->objectName());
    QString action_icon_text = actionTextToDescription(action->iconText(),
                                                       action->objectName());
	if (action_text != action_icon_text) {
        s += "\t" + action->iconText();
	}

	return s;
}

void TActionsEditor::saveToConfig(QSettings* set, QObject* o) {
	qDebug("Gui::Action::TActionsEditor::saveToConfig");

	set->beginGroup("actions");

    // Clear group to remove actions no longer modified
	set->remove("");

	TActionList actions = o->findChildren<QAction*>();
	for (int n = 0; n < actions.count(); n++) {
		QAction* action = actions[n];
        if (action->property("modified").toBool()) {
            set->setValue(action->objectName(), actionToString(action));
		}
	}

	set->endGroup();
}

void TActionsEditor::removeShortcuts(const TActionList& actions, const TShortCutList& shortcuts, QAction* skip_action) {

	if (shortcuts.isEmpty()) {
		TShortCutList remove = skip_action->shortcuts();
		if (!remove.isEmpty()) {
			qDebug() << "Gui::Action::TActionsEditor::removeShortcuts: removing shortcuts"
					 << remove << "from" << skip_action->objectName();
		}
		return;
	}

	for(int i = 0; i < actions.size(); i++) {
		QAction* action = actions[i];
		if (action != skip_action) {
			TShortCutList l = action->shortcuts();
			bool removed = false;
			for(int n = 0; n < shortcuts.size(); n++) {
				if (l.removeOne(shortcuts[n])) {
					qDebug() << "Gui::Action::TActionsEditor::removeShortcuts: removing shortcut"
							 << shortcuts[n] << "from" << action->objectName()
							 << "to assign it to" << skip_action->objectName();
					removed = true;
				}
			}
			if (removed) {
				action->setShortcuts(l);
			}
		}
	}
}

void TActionsEditor::setActionFromString(QAction* action, const QString& s, const TActionList& actions) {
    //qDebug() << "TActionsEditor::setActionFromString:" << action.objectName() << s;

    static QRegExp rx("^([^\\t]*)(\\t(.*))?");

    if (rx.indexIn(s) >= 0) {
        QString shortcuts = rx.cap(1);
        QString old_shortcuts = shortcutsToString(action->shortcuts());
        if (shortcuts != old_shortcuts) {
            TShortCutList l = stringToShortcuts(shortcuts);
            removeShortcuts(actions, l, action);
            action->setShortcuts(l);
            action->setProperty("modified", true);
        }

        QString icon_text = rx.cap(3).trimmed();
        if (!icon_text.isEmpty()) {
            action->setIconText(icon_text);
            action->setProperty("modified", true);
        }
    }
}

QAction* TActionsEditor::findAction(const TActionList& actions, const QString& name) {

    for (int n = 0; n < actions.count(); n++) {
        QAction* action = actions[n];
        if (name == action->objectName())
            return action;
    }

    return 0;
}

void TActionsEditor::loadFromConfig(QSettings* set, const TActionList& all_actions) {
	qDebug("Gui::Action::TActionsEditor::loadFromConfig");

	set->beginGroup("actions");

    QStringList actions = set->childKeys();
    for (int n = 0; n < actions.count(); n++) {
        QString name = actions[n];
        QAction* action = findAction(all_actions, name);
        if (action) {
            setActionFromString(action, set->value(name, "").toString(),
                                all_actions);
        } else {
            qWarning() << "TActionsEditor::loadFromConfig: action" << name
                       << "not found";
        }
    }

	set->endGroup();
}

} // namespace Action
} // namespace Gui

#include "moc_actionseditor.cpp"
