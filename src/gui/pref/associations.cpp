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


    prefassociations.cpp
    Handles file associations in Windows
    Author: Florin Braghis (florin@libertv.ro)
*/


#include "gui/pref/associations.h"
#include "images.h"
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include "winfileassoc.h"
#include "extensions.h"

namespace Gui { namespace Pref {

static Qt::CheckState CurItemCheckState = Qt::Unchecked; 


TAssociations::TAssociations(QWidget* parent, Qt::WindowFlags f)
: TSection(parent, f)
{
    setupUi(this);

    connect(selectAll, &QPushButton::clicked,
            this, &TAssociations::selectAllClicked);
    connect(selectNone, &QPushButton::clicked,
            this, &TAssociations::selectNoneClicked);
    connect(listWidget, &QListWidget::itemClicked,
            this, &TAssociations::listItemClicked);
    connect(listWidget, &QListWidget::itemPressed,
            this, &TAssociations::listItemPressed);

    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
    {
        //Hide Select None - One cannot restore an association in Vista. Go figure.
        selectNone->hide();
        //QPushButton* lpbButton = new QPushButton("Launch Program Defaults", this);
        //hboxLayout->addWidget(lpbButton);
        //connect(lpbButton, SIGNAL(clicked(bool)), this, SLOT(launchAppDefaults()));
    }

    for (int n = 0; n < extensions.allPlayable().count(); n++) {
        addItem(extensions.allPlayable()[n]);
    }

    retranslateStrings();

    something_changed = false;
}

void TAssociations::selectAllClicked(bool)
{
    for (int k = 0; k < listWidget->count(); k++)
        listWidget->item(k)->setCheckState(Qt::Checked);
    listWidget->setFocus();

    something_changed = true;
}

void TAssociations::selectNoneClicked(bool)
{

    for (int k = 0; k < listWidget->count(); k++)
        listWidget->item(k)->setCheckState(Qt::Unchecked);
    listWidget->setFocus();

    something_changed = true;
}

void TAssociations::listItemClicked(QListWidgetItem* item)
{
    logger()->debug("Gui::Pref::TAssociations::listItemClicked");

    if (!(item->flags() & Qt::ItemIsEnabled))
        return;

    if (item->checkState() == CurItemCheckState)
    {
        //Clicked on the list item (not checkbox)
        if (item->checkState() == Qt::Checked)
        {
            item->setCheckState(Qt::Unchecked);
        }
        else
            item->setCheckState(Qt::Checked);
    }

    //else - clicked on the checkbox itself, do nothing

    something_changed = true;
}

void TAssociations::listItemPressed(QListWidgetItem* item)
{
    CurItemCheckState = item->checkState();
}

void TAssociations::addItem(QString label)
{
    QListWidgetItem* item = new QListWidgetItem(listWidget);
    item->setText(label);
}

void TAssociations::refreshList()
{
    m_regExtensions.clear();
    WinFileAssoc ().GetRegisteredExtensions(extensions.allPlayable(), m_regExtensions);

    for (int k = 0; k < listWidget->count(); k++)
    {
        QListWidgetItem* pItem = listWidget->item(k);
        if (pItem)
        {
            pItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

            if (m_regExtensions.contains(pItem->text()))
            {
                pItem->setCheckState(Qt::Checked);
                //Don't allow de-selection in windows VISTA if extension is registered.
                //VISTA doesn't seem to support extension 'restoration' in the API.
                if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA) {
                    pItem->setFlags(0);
                }
            }
            else
            {
                pItem->setCheckState(Qt::Unchecked);
            }

        }
    }
}

void TAssociations::setData(Settings::TPreferences*)
{
    refreshList();
}

int TAssociations::ProcessAssociations(QStringList& current, QStringList& old)
{
    WinFileAssoc RegAssoc;

    QStringList toRestore;

    //Restore unselected associations
    foreach(const QString& ext, old)
    {
        if (!current.contains(ext))
            toRestore.append(ext);
    }

    RegAssoc.RestoreFileAssociations(toRestore);
    return RegAssoc.CreateFileAssociations(current);
}

void TAssociations::getData(Settings::TPreferences*) {
    logger()->debug("something_changed: %1", something_changed);

    TSection::getData(pref);

    if (!something_changed)
        return;

    QStringList extensions;

    for (int k = 0; k < listWidget->count(); k++)
    {
        QListWidgetItem* pItem = listWidget->item(k);
        if (pItem && pItem->checkState() == Qt::Checked)
            extensions.append(pItem->text());
    }

    int processed = ProcessAssociations(extensions, m_regExtensions);

    if (processed != extensions.count())
    {
        QMessageBox::warning(this, tr("Warning"),
            tr("Not all files could be associated. Please check your "
               "security permissions and retry."), QMessageBox::Ok);
    }

    refreshList(); //Useless when OK is pressed... How to detect if apply or ok is pressed ?

    something_changed = false;
}

QString TAssociations::sectionName() {
    return tr("File Types");
}

QPixmap TAssociations::sectionIcon() {
    return Images::icon("pref_associations", iconSize);
}

void TAssociations::retranslateStrings() {

    retranslateUi(this);
    createHelp();
}

void TAssociations::createHelp() {

    clearHelp();

    setWhatsThis(selectAll, tr("Select all"),
        tr("Check all file types in the list"));

    setWhatsThis(selectNone, tr("Select none"),
        tr("Uncheck all file types in the list"));

    setWhatsThis(listWidget, tr("List of file types"),
        tr("Check the media file extensions you would like WZPlayer to handle. "
           "When you click Apply, the checked files will be associated with "
           "WZPlayer. If you uncheck a media type, the file association will "
           "be restored.") + "<br><b>" + tr("Note:") + "</b> " +
        tr("Restoration doesn't work on Windows Vista."));
}

}} // namespace Gui::Pref

#include "moc_associations.cpp"

