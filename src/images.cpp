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

#include "wzdebug.h"
#include "settings/preferences.h"
#include "settings/paths.h"


using namespace Settings;

LOG4QT_DECLARE_STATIC_LOGGER(logger, Images)

QString Images::current_theme;
QString Images::themes_path;
QString Images::last_resource_loaded;
bool Images::has_rcc = false;


void Images::setTheme(const QString& name) {

    if (!last_resource_loaded.isEmpty()) {
        WZDEBUG("Unloading '" + last_resource_loaded + "'");
        QResource::unregisterResource(last_resource_loaded);
        last_resource_loaded = "";
        has_rcc = false;
    }

    current_theme = name;

    QString dir = TPaths::configPath() + "/themes/" + name;
    if (QFile::exists(dir)) {
        themes_path = TPaths::configPath() + "/themes/";
    } else {
        themes_path = TPaths::themesPath();
    }

    QString rs_file;
    if (!themes_path.isEmpty() && !current_theme.isEmpty()) {
        rs_file = themes_path + "/" + current_theme + "/"
                + current_theme + ".rcc";
    }

    if (!rs_file.isEmpty() && QFile::exists(rs_file)) {
        WZDEBUG("Loading resource file '" + rs_file + "'");
        if (QResource::registerResource(rs_file)) {
            last_resource_loaded = rs_file;
            has_rcc = true;
        }
    } else {
        has_rcc = false;
    }
    WZDEBUG("has_rcc " + QString::number(has_rcc));
}

QString Images::iconFilename(const QString& name) {

    if (name.isEmpty())
        return "";

    if (current_theme != pref->iconset) {
        setTheme(pref->iconset);
    }

    QString filename;
    if (!current_theme.isEmpty()) {
        if (has_rcc) {
            filename = ":/" + current_theme + "/"+ name + ".png";
        } else {
            filename = themes_path +"/"+ current_theme + "/"+ name + ".png";
        }
    }

    if (filename.isEmpty() || !QFile::exists(filename)) {
        filename = ":/default-theme/" + name + ".png";
    }

    return filename;
}

QPixmap Images::icon(const QString& name, int size) {

    if (name.isEmpty()) {
        return QPixmap();
    }

    QPixmap pixmap(iconFilename(name));
    if (pixmap.isNull()) {
        WZTRACE("'" + name + "' not found");
    } else if (size > 0) {
        pixmap = resize(pixmap, size);
    }

    return pixmap;
}

QPixmap Images::resize(const QPixmap& pixmap, int size) {
    return QPixmap::fromImage(
                pixmap.toImage()
                .scaled(size, size,
                        Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

QPixmap Images::flip(const QPixmap& pixmap) {
    return QPixmap::fromImage(pixmap.toImage().mirrored(true, false));
}

QPixmap Images::flippedIcon(const QString& name, int size) {
    return flip(icon(name, size));
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

