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

/***********************************************************************
 * Copyright 2012  Eike Hein <hein@kde.org>
 * Copyright 2012  Bernd Buschinski <b.buschinski@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#include <QApplication>

#include "player/mpris2/mediaplayer2.h"
#include "player/mpris2/mpris2.h"
#include "config.h"
#include "gui/mainwindow.h"


namespace Player {
namespace Mpris2 {

TMediaPlayer2::TMediaPlayer2(Gui::TMainWindow* gui, QObject* parent)
    : QDBusAbstractAdaptor(parent),
      m_gui(gui) {
//     connect(m_gui, SIGNAL(fullScreen(bool)),
//             this, SLOT(emitFullscreenChange(bool)));
}

TMediaPlayer2::~TMediaPlayer2() {
}

bool TMediaPlayer2::CanQuit() const {
    return true;
}

void TMediaPlayer2::Quit() const {
	m_gui->runActions("close");
}

bool TMediaPlayer2::CanRaise() const {
    return true;
}

void TMediaPlayer2::Raise() const {
    m_gui->raise();
}

bool TMediaPlayer2::Fullscreen() const {
    return m_gui->isFullScreen();
}

void TMediaPlayer2::setFullscreen(bool fullscreen) const {
    m_gui->toggleFullscreen(fullscreen);
}

void TMediaPlayer2::emitFullscreenChange(bool fullscreen) const {
    QVariantMap properties;
    properties["Fullscreen"] = fullscreen;
    TMpris2::signalPropertiesChange(this, properties);
}

bool TMediaPlayer2::CanSetFullscreen() const {
    return true;
}

bool TMediaPlayer2::HasTrackList() const {
    return false;
}

QString TMediaPlayer2::Identity() const {
	return TConfig::PROGRAM_NAME;
}

QString TMediaPlayer2::DesktopEntry() const {
	return TConfig::PROGRAM_ID;
}

QStringList TMediaPlayer2::SupportedUriSchemes() const {
    //TODO: Implement me
    return QStringList();
}

QStringList TMediaPlayer2::SupportedMimeTypes() const {
    //TODO: Implement me
    return QStringList();
}

} // namespace Mpris2
} // namespace Player

#include "moc_mediaplayer2.cpp"