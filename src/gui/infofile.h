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

#ifndef GUI_INFOFILE_H
#define GUI_INFOFILE_H

#include <QObject>
#include <QString>

#include "mediadata.h"
#include "wzdebug.h"


namespace Gui {

class TInfoFile : public QObject {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TInfoFile();

    QString getInfo(const TMediaData& md);

private:
    QString openPar(QString text);
    QString closePar();

    QString addItem(QString tag, QString value);

    QString formatSize(qint64 size);
    void addTracks(QString& s,
                   const Maps::TTracks& tracks,
                   const QString& name);

    void setMetaData(const QString& filename);
};

} // namespace Gui

#endif // GUI_INFOFILE_H
