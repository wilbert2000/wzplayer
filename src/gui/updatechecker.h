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


#ifndef GUI_UPDATE_CHECKER_H
#define GUI_UPDATE_CHECKER_H

#include <QWidget>
#include <QUrl>
#include "wzdebug.h"
#include "settings/updatecheckerdata.h"


class QNetworkAccessManager;

namespace Gui {

class TUpdateChecker : public QObject {
    Q_OBJECT
    LOG4QT_DECLARE_QCLASS_LOGGER
public:
    explicit TUpdateChecker(QWidget* parent, Settings::TUpdateCheckerData* data);

    void check();

protected slots:
    void gotReply();
    void gotReplyFromUserRequest();
    void reportNewVersionAvailable(const QString &);
    void reportNoNewVersionFound(const QString &);
    void reportError(int, QString);

signals:
    void newVersionFound(const QString & new_version);
    void noNewVersionFound(const QString & version);
    void errorOcurred(int error_number, QString error_str);

protected:
    void saveVersion(QString v);
    QString parseVersion(const QByteArray& data, const QString& name);

    QNetworkAccessManager* net_manager;
    Settings::TUpdateCheckerData* d;

    QUrl check_url;
    QByteArray user_agent;
};

} // namespace Gui

#endif // GUI_UPDATE_CHECKER_H

