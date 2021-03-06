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

    Note: The TShortcutGetter class is taken from the source code of Edyuk
    (http://www.edyuk.org/), from file 3rdparty/qcumber/qshortcutdialog.cpp

    Copyright (C) 2006 FullMetalCoder
    License: GPL

    I modified it to support multiple shortcuts and some other few changes.
*/


/****************************************************************************
**
** Copyright (C) 2006 FullMetalCoder
**
** This file is part of the Edyuk project (beta version)
** 
** This file may be used under the terms of the GNU General Public License
** version 2 as published by the Free Software Foundation and appearing in the
** file GPL.txt included in the packaging of this file.
**
** Notes : Parts of the project are derivative work of Trolltech's QSA library
** or Trolltech's Qt4 framework but, unless notified, every single line of code
** is the work of the Edyuk team or a contributor. 
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#include "gui/action/shortcutgetter.h"
#include "gui/action/actionseditor.h"
#include "images.h"
#include "config.h"

#include <QLayout>
#include <QHash>
#include <QLabel>
#include <QString>
#include <QShortcut>
#include <QLineEdit>
#include <QKeyEvent>
#include <QPushButton>
#include <QDialogButtonBox>


namespace Gui {
namespace Action {


TShortcutGetter::TShortcutGetter(TActionsEditor* parent,
                                 const QString& actName,
                                 const QString& actOwner) :
    QDialog(parent, TConfig::DIALOG_FLAGS),
    actionName(actName),
    actionOwner(actOwner),
    editor(parent) {

    setWindowTitle(tr("Modify shortcut for action %1").arg(actionName));

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setSpacing(11);

    list = new QListWidget(this);
    connect(list, &QListWidget::currentRowChanged,
            this, &TShortcutGetter::rowChanged);
    vbox->addWidget(list);

    addItem = new QPushButton(Images::icon("plus"), "", this);
    addItem->setText("Add new shortcut");
    connect(addItem, &QPushButton::clicked,
            this, &TShortcutGetter::addItemClicked);

    removeItem = new QPushButton(Images::icon("minus"), "", this);
    removeItem->setText("Remove shortcut");
    connect(removeItem, &QPushButton::clicked,
            this, &TShortcutGetter::removeItemClicked);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->addSpacerItem(new QSpacerItem(20, 20, QSizePolicy::Expanding));
    hbox->addWidget(addItem);
    hbox->addWidget(removeItem);
    vbox->addLayout(hbox);

    QLabel *l = new QLabel(this);
    l->setWordWrap(true);
    l->setText(tr("Press the key combination to assign to action %1 for %2:")
               .arg(actionName).arg(actionOwner));
    vbox->addWidget(l);

    leKey = new QLineEdit(this);
    vbox->addWidget(leKey);

    assignedToLabel = new QLabel(this);
    assignedToLabel->setText("\n");
    vbox->addWidget(assignedToLabel);

    leKey->installEventFilter(this);
    // Needs QueuedConnection otherwise during the call to onTextChanged()
    // list.count() will not yet be updated
    connect(leKey, &QLineEdit::textChanged,
            this, &TShortcutGetter::onKeyTextChanged,
            Qt::QueuedConnection);
    setCaptureKeyboard(true);

    QDialogButtonBox* buttonbox = new QDialogButtonBox(
                QDialogButtonBox::Ok
                | QDialogButtonBox::Cancel
                | QDialogButtonBox::Reset,
                this);

    okButton = buttonbox->button(QDialogButtonBox::Ok);
    okButtonText = okButton->text();

    QPushButton* clearbutton = buttonbox->button(QDialogButtonBox::Reset);
    clearbutton->setText(tr("Clear"));

    QPushButton* captureButton = new QPushButton(tr("Capture"), this);
    captureButton->setToolTip(tr("Capture keystrokes"));
    captureButton->setCheckable(capture);
    captureButton->setChecked(capture);
    connect(captureButton, &QPushButton::toggled,
            this, &TShortcutGetter::setCaptureKeyboard);
    buttonbox->addButton(captureButton, QDialogButtonBox::ActionRole);

    connect(buttonbox, &QDialogButtonBox::accepted,
            this, &TShortcutGetter::accept);
    connect(buttonbox, &QDialogButtonBox::rejected,
            this, &TShortcutGetter::reject);
    connect(clearbutton, &QPushButton::clicked, leKey, &QLineEdit::clear);
    vbox->addWidget(buttonbox);
}

void TShortcutGetter::setCaptureKeyboard(bool b) { 

    capture = b;
    leKey->setReadOnly(b);
    leKey->setFocus();
}

void TShortcutGetter::rowChanged(int row) {

    QString s = list->item(row)->text();
    leKey->setText(s);
    leKey->setFocus();
}

void TShortcutGetter::onKeyTextChanged(const QString& text) {
    WZDEBUG(QString("'%1' %2").arg(text).arg(list->count()));

    list->item(list->currentRow())->setText(text);
    QString label, action;
    editor->findShortcutActionAndLabel(text, actionOwner, action, label);
    assignedToLabel->setText(label);

    QString buttonText;
    if (list->count() > 1) {
        buttonText = tr("Take shortcuts");
    } else if (action.isEmpty() || action == actionName) {
        buttonText = okButtonText;
    } else {
        buttonText = tr("Take shortcut from %1").arg(action);
    }
    okButton->setText(buttonText);
}

void TShortcutGetter::addItemClicked() {

    list->addItem("");
    list->setCurrentRow(list->count() - 1); // Select last item
}

void TShortcutGetter::removeItemClicked() {

    if (list->count() > 1) {
        delete list->takeItem(list->currentRow());
    } else {
        list->setCurrentRow(0);
        leKey->setText("");
    }
}

QString TShortcutGetter::exec(const QString& s) {

    QStringList shortcuts = TActionsEditor::shortcutsToStringList(s);
    if (shortcuts.count() == 0) {
        list->addItem("");
    } else {
        for(int i = 0; i < shortcuts.count(); i++) {
            list->addItem(shortcuts.at(i));
        }
    }
    list->setCurrentRow(0);

    bStop = false;

    if (QDialog::exec() == QDialog::Accepted) {
        QStringList l;
        for (int i = 0; i < list->count(); i++) {
            QString shortcut = list->item(i)->text();
            if (!shortcut.isEmpty()) {
                l << shortcut;
            }
        }
        return l.join(", ");
    }

    return QString();
}

static QString keyToString(int k) {

    if (k == Qt::Key_Shift
            || k == Qt::Key_Control
            || k == Qt::Key_Meta
            || k == Qt::Key_Alt
            || k == Qt::Key_AltGr) {
        return "";
    }

    return QKeySequence(k).toString();
}

static Qt::KeyboardModifiers getModifiers(Qt::KeyboardModifiers modifiers,
                                          const QString& text) {

    Qt::KeyboardModifiers result = 0;

    // Fix shift
    if ((modifiers & Qt::ShiftModifier)
            && (text.isEmpty()
                || !text.at(0).isPrint()
                || text.at(0).isLetterOrNumber()
                || text.at(0).isSpace()))
        result |= Qt::ShiftModifier;
    if (modifiers & Qt::ControlModifier)
        result |= Qt::ControlModifier;
    if (modifiers & Qt::AltModifier)
        result |= Qt::AltModifier;
    if (modifiers & Qt::MetaModifier)
        result |= Qt::MetaModifier;
    /*
    QKeySequence does not support Num
    if (modifiers & Qt::KeypadModifier) {
        result |= Qt::KeypadModifier;
    }
    */

    return result;
}

void TShortcutGetter::captureEvent(QEvent* e) {

    e->accept();

    if (e->type() == QEvent::KeyRelease) {
        bStop = true;
        return;
    }

    if (bStop) {
        bStop = false;
        modifiers = 0;
        lKeys.clear();
    }

    QKeyEvent *k = static_cast<QKeyEvent*>(e);
    QString key = keyToString(k->key());
    modifiers = getModifiers(k->modifiers(), k->text());

    if (!key.isEmpty()) {
        if (!lKeys.contains(key)) {
            lKeys << key;
        }
    } else if (modifiers == 0) {
        key = k->text();
        if (!lKeys.contains(key)) {
            lKeys << key;
        }
    }

    setText();
}

bool TShortcutGetter::event(QEvent* e) {

    if (capture && (e->type() == QEvent::KeyPress
                    || e->type() == QEvent::KeyRelease)) {
        captureEvent(e);
        return true;
    }

    return QDialog::event(e);
}

bool TShortcutGetter::eventFilter(QObject* o, QEvent* e) {

    if (capture
        && (e->type() == QEvent::KeyPress || e->type() ==QEvent::KeyRelease)) {
        captureEvent(e);
        // Filter event
        return true;
    }
    return QDialog::eventFilter(o, e);
}

void TShortcutGetter::setText() {

    QStringList seq;
    if (modifiers & Qt::ShiftModifier) {
        seq << "Shift";
    }
    if (modifiers & Qt::ControlModifier) {
        seq << "Ctrl";
    }
    if (modifiers & Qt::AltModifier) {
        seq << "Alt";
    }
    if (modifiers & Qt::MetaModifier) {
        seq << "Meta";
    }
    /*
    if (modifiers & Qt::KeypadModifier) {
        seq << "Num";
    }
    */

    foreach (const QString& s, lKeys) {
        seq << s;
    }

    leKey->setText(seq.join("+"));
}

} // namespace Action
} // namespace Gui

#include "moc_shortcutgetter.cpp"
