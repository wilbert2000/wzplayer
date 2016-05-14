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

#include "images.h"
#include <QString>
#include <QFile>
#include <QPixmap>
#include <QResource>

#include "log4qt/logger.h"
#include "settings/preferences.h"
#include "settings/paths.h"


using namespace Settings;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Images)

QString Images::current_theme;
QString Images::themes_path;

QString Images::last_resource_loaded;
bool Images::has_rcc = false;

QString Images::resourceFilename() {

	QString filename = QString::null;

	if ((!themes_path.isEmpty()) && (!current_theme.isEmpty())) {
		filename = themes_path +"/"+ current_theme +"/"+ current_theme +".rcc";
	}

    logger()->debug("resourceFilename: resource file name '" + filename + "'");
    return filename;
}

void Images::setTheme(const QString& name) {

	current_theme = name;

	QString dir = TPaths::configPath() + "/themes/" + name;
	if (QFile::exists(dir)) {
		setThemesPath(TPaths::configPath() + "/themes/");
	} else {
		setThemesPath(TPaths::themesPath());
	}

	if (!last_resource_loaded.isEmpty()) {
        logger()->debug("setTheme: unloading '" + last_resource_loaded + "'");
		QResource::unregisterResource(last_resource_loaded);
		last_resource_loaded = QString::null;
	}

	QString rs_file = resourceFilename();
	if (!rs_file.isEmpty() && QFile::exists(rs_file)) {
        logger()->debug("setTheme: loading '" + rs_file + "'");
		QResource::registerResource(rs_file);
		last_resource_loaded = rs_file;
		has_rcc = true;
	} else {
		has_rcc = false;
	}
    logger()->debug("setTheme: has_rcc: %1", has_rcc);
}

void Images::setThemesPath(const QString& folder) {
    logger()->debug("setThemesPath: '" + folder + "'");
	themes_path = folder;
}

QString Images::file(const QString& name) {

	if (name.isEmpty())
		return "";

	if (current_theme != pref->iconset) {
		setTheme(pref->iconset);
	}

	QString icon_name;
	if (!current_theme.isEmpty()) {
		if (has_rcc)
			icon_name = ":/" + current_theme + "/"+ name + ".png";
		else
			icon_name = themes_path +"/"+ current_theme + "/"+ name + ".png";
	}

	if (icon_name.isEmpty() || !QFile::exists(icon_name)) {
		icon_name = ":/default-theme/" + name + ".png";
	}

	return icon_name;
}

QPixmap Images::icon(const QString& name, int size) {

	if (name.isEmpty())
        return QPixmap();

	QString icon_name = file(name);
	QPixmap p(icon_name);
	if (p.isNull()) {
        logger()->trace("icon: '" + name + "' not found");
	} else if (size > 0) {
		p = resize(&p, size);
	}

	return p;
}

QPixmap Images::resize(QPixmap* p, int size) {
	return QPixmap::fromImage((*p).toImage().scaled(size,size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
}

QPixmap Images::flip(QPixmap* p) {
	return QPixmap::fromImage((*p).toImage().mirrored(true, false));
}

QPixmap Images::flippedIcon(const QString& name, int size) {

	QPixmap p = icon(name, size);
	p = flip(&p);
	return p;
}

QString Images::styleSheet() {

	QString css;
	QString filename = themesDirectory() + "/main.css";
	QFile file(filename);
	if (file.exists()) {
		file.open(QFile::ReadOnly | QFile::Text);
		css = QString::fromUtf8(file.readAll().constData());
	}
	return css;
}

QString Images::themesDirectory() {

	QString skin = pref->iconset;
	QString dirname;
	if (!skin.isEmpty()) {
		dirname = TPaths::configPath() + "/themes/" + skin;
		if (!QFile::exists(dirname)) {
			dirname = TPaths::themesPath() + "/" + skin ;
		}
	}
	return dirname;
}

