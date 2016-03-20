/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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


#include "gui/pref/advanced.h"
#include <QColorDialog>
#include "images.h"
#include "settings/preferences.h"


using namespace Settings;

namespace Gui { namespace Pref {

TAdvanced::TAdvanced(QWidget* parent, Qt::WindowFlags f)
	: TWidget(parent, f) {

	setupUi(this);

	// TODO:
	colorkey_label->hide();
	colorkey_view->hide();
	changeButton->hide();

	retranslateStrings();
}

TAdvanced::~TAdvanced() {
}

QString TAdvanced::sectionName() {
	return tr("Advanced");
}

QPixmap TAdvanced::sectionIcon() {
    return Images::icon("pref_advanced", 22);
}


void TAdvanced::retranslateStrings() {

	retranslateUi(this);
	createHelp();
}

void TAdvanced::setData(TPreferences* pref) {

	setColorKey(pref->color_key);

	setActionsToRun(pref->actions_to_run);

	setMplayerAdditionalArguments(pref->mplayer_additional_options);
	setMplayerAdditionalVideoFilters(pref->mplayer_additional_video_filters);
	setMplayerAdditionalAudioFilters(pref->mplayer_additional_audio_filters);
}

void TAdvanced::getData(TPreferences* pref) {

	requires_restart = false;
	colorkey_changed = false;
	lavf_demuxer_changed = false;

	if (pref->color_key != colorKey()) {
		pref->color_key = colorKey();
		colorkey_changed = true;
		requires_restart = true;
	}

	pref->actions_to_run = actionsToRun();

	restartIfStringChanged(pref->mplayer_additional_options, mplayerAdditionalArguments());
	restartIfStringChanged(pref->mplayer_additional_video_filters, mplayerAdditionalVideoFilters());
	restartIfStringChanged(pref->mplayer_additional_audio_filters, mplayerAdditionalAudioFilters());
}

void TAdvanced::setMplayerAdditionalArguments(QString args) {
	mplayer_args_edit->setText(args);
}

QString TAdvanced::mplayerAdditionalArguments() {
	return mplayer_args_edit->text();
}

void TAdvanced::setMplayerAdditionalVideoFilters(QString s) {
	mplayer_vfilters_edit->setText(s);
}

QString TAdvanced::mplayerAdditionalVideoFilters() {
	return mplayer_vfilters_edit->text();
}

void TAdvanced::setMplayerAdditionalAudioFilters(QString s) {
	mplayer_afilters_edit->setText(s);
}

QString TAdvanced::mplayerAdditionalAudioFilters() {
	return mplayer_afilters_edit->text();
}

void TAdvanced::setColorKey(unsigned int c) {

	QString color = QString::number(c, 16);
	while (color.length() < 6)
		color = "0" + color;
	colorkey_view->setText("#" + color);
}

unsigned int TAdvanced::colorKey() {

	QString c = colorkey_view->text();
	if (c.startsWith("#")) c = c.mid(1);

	bool ok;
	unsigned int color = c.toUInt(&ok, 16);

	if (!ok) 
		qWarning("Gui::Pref::TAdvanced::colorKey: cannot convert color to uint");

	qDebug("Gui::Pref::TAdvanced::colorKey: color: %s", QString::number(color, 16).toUtf8().data());
	return color;
}

void TAdvanced::setActionsToRun(QString actions) {
	actions_to_run_edit->setText(actions);
}

QString TAdvanced::actionsToRun() {
	return actions_to_run_edit->text();
}

void TAdvanced::on_changeButton_clicked() {
	//bool ok;
	//int color = colorkey_view->text().toUInt(&ok, 16);
	QColor color(colorkey_view->text());
	QColor c = QColorDialog::getColor (color, this);
	if (c.isValid()) {
		//colorkey_view->setText(QString::number(c.rgb(), 16));
		colorkey_view->setText(c.name());
	}
}

void TAdvanced::createHelp() {
	clearHelp();

	addSectionTitle(tr("Advanced"));

	setWhatsThis(actions_to_run_edit, tr("Actions list"),
		tr("Here you can specify a list of <i>actions</i> which will be "
           "run every time a file is opened. You'll find all available "
           "actions in the key shortcut editor in the <b>Keyboard and mouse</b> "
           "section. The actions must be separated by spaces. Checkable "
           "actions can be followed by <i>true</i> or <i>false</i> to "
           "enable or disable the action.") +"<br>"+
		tr("Example:") +" <i>auto_zoom fullscreen true</i><br>" +
		tr("Limitation: the actions are run only when a file is opened and "
           "not when the mplayer process is restarted (e.g. you select an "
           "audio or video filter)."));

	setWhatsThis(colorkey_view, tr("Colorkey"),
        tr("If you see parts of the video over any other window, you can "
           "change the colorkey to fix it. Try to select a color close to "
           "black."));

	addSectionTitle(tr("Options for %1").arg(pref->playerName()));

	setWhatsThis(mplayer_args_edit, tr("Options"),
        tr("Here you can type options for %1.").arg(pref->playerName()) +" "+
        tr("Write them separated by spaces."));

	setWhatsThis(mplayer_vfilters_edit, tr("Video filters"),
        tr("Here you can add video filters for %1.").arg(pref->playerName()) +" "+
        tr("Write them separated by commas. Don't use spaces!"));

	setWhatsThis(mplayer_afilters_edit, tr("Audio filters"),
        tr("Here you can add audio filters for %1.").arg(pref->playerName()) +" "+
        tr("Write them separated by commas. Don't use spaces!"));
}

}} // namespace Gui::Pref

#include "moc_advanced.cpp"
