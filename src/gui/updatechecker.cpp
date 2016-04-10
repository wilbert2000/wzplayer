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

#include "gui/updatechecker.h"
#include "config.h"
#include "updatecheckerdata.h"
#include "version.h"
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegExp>
#include <QDate>
#include <QDateTime>
#include <QStringList>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QSettings>
#include <QDebug>

namespace Gui {

TUpdateChecker::TUpdateChecker(QWidget* parent, TUpdateCheckerData* data) : QObject(parent)
	, net_manager(0)
	, d(0)
{
	d = data;

	check_url = TConfig::URL_VERSION_INFO;
	user_agent = TConfig::PROGRAM_NAME.toLatin1();

	connect(this, SIGNAL(newVersionFound(const QString&)),
			this, SLOT(reportNewVersionAvailable(const QString&)));

	connect(this, SIGNAL(noNewVersionFound(const QString&)),
			this, SLOT(reportNoNewVersionFound(const QString&)));

	connect(this, SIGNAL(errorOcurred(int, QString)), this, SLOT(reportError(int, QString)));

	net_manager = new QNetworkAccessManager(this);

	QDate now = QDate::currentDate();
	//now = now.addDays(27);
	int days = QDateTime(d->last_checked).daysTo(QDateTime(now));

	qDebug("TUpdateChecker::TUpdateChecker: enabled: %d, days_to_check: %d, days since last check: %d",
		   d->enabled, d->days_to_check, days);
	if (!d->enabled || days < d->days_to_check)
		return;

	QNetworkRequest req(check_url);
	req.setRawHeader("User-Agent", user_agent);
	QNetworkReply *reply = net_manager->get(req);
	connect(reply, SIGNAL(finished()), this, SLOT(gotReply()), Qt::QueuedConnection);
}

TUpdateChecker::~TUpdateChecker() {
}

// Force a check, requested by the user
void TUpdateChecker::check() {
	qDebug("TUpdateChecker::check");

	QNetworkRequest req(check_url);
	req.setRawHeader("User-Agent", user_agent);
	QNetworkReply *reply = net_manager->get(req);
	connect(reply, SIGNAL(finished()), this, SLOT(gotReplyFromUserRequest()));
}

QString TUpdateChecker::parseVersion(const QByteArray& data, const QString& name) {

	qDebug() << "TUpdateChecker::parseVersion: data:\n" << data;
	QTemporaryFile tf;
	tf.open();
	tf.write(data);
	tf.close();
	QString tfile = tf.fileName();
	qDebug() << "TUpdateChecker::parseVersion: tfile:" << tfile;

#ifdef Q_OS_WIN
	QString grname = "windows";
#else
	QString grname = "linux";
#endif

	QSettings set(tfile, QSettings::IniFormat);
	set.beginGroup(grname);
	QString version = set.value(name, "").toString();
	set.endGroup();
	return version;
}

void TUpdateChecker::gotReply() {
	qDebug("TUpdateChecker::gotReply");

	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply) {
		if (reply->error() == QNetworkReply::NoError) {
			QString version = parseVersion(reply->readAll(), "stable");
			if (!version.isEmpty()) {
				d->last_checked = QDate::currentDate();
				qDebug() << "TUpdateChecker::gotReply: last known version:" << d->last_known_version << "received version:" << version;
				qDebug() << "TUpdateChecker::gotReply: installed version:" << TVersion::version;
				if (d->last_known_version != version && version > TVersion::version) {
					qDebug() << "TUpdateChecker::gotReply: new version found:" << version;
					emit newVersionFound(version);
				}
			}
		} else {
			//get http status code
			int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			qDebug("TUpdateChecker::gotReply: status: %d", status);
		}
		reply->deleteLater();
	}
}

void TUpdateChecker::gotReplyFromUserRequest() {
	qDebug("TUpdateChecker::gotReplyFromUserRequest");

	QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

	if (reply) {
		if (reply->error() == QNetworkReply::NoError) {
			QString version = parseVersion(reply->readAll(), "unstable");
			if (!version.isEmpty()) {
				if (version > TVersion::version) {
					qDebug("TUpdateChecker::gotReplyFromUserRequest: new version found: %s", version.toUtf8().constData());
					emit newVersionFound(version);
				} else {
					emit noNewVersionFound(version);
				}
			} else {
				emit errorOcurred(1, tr("Failed to get the latest version number"));
			}
		} else {
			int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
			qDebug("TUpdateChecker::gotReplyFromUserRequest: status: %d", status);
			emit errorOcurred((int)reply->error(), reply->errorString());
		}
		reply->deleteLater();
	}
}

void TUpdateChecker::saveVersion(QString v) {
	d->last_known_version = v;
}

void TUpdateChecker::reportNewVersionAvailable(const QString& new_version) {

	QWidget* p = qobject_cast<QWidget*>(parent());

	QMessageBox::StandardButton button = QMessageBox::information(p, tr("New version available"),
		tr("A new version of WZPlayer is available.") + "<br><br>" +
		tr("Installed version: %1").arg(TVersion::version) + "<br>" +
		tr("Available version: %1").arg(new_version) + "<br><br>" +
		tr("Would you like to know more about this new version?"),
		QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

	if (button == QMessageBox::Yes) {
		QDesktopServices::openUrl(QUrl(TConfig::URL_CHANGES));
	}

	saveVersion(new_version);
}

void TUpdateChecker::reportNoNewVersionFound(const QString & version) {
	QWidget* p = qobject_cast<QWidget*>(parent());

	QMessageBox::information(p, tr("Checking for updates"),
		tr("Congratulations, WZPlayer is up to date.") + "<br><br>" +
		tr("Installed version: %1").arg(TVersion::version) + "<br>" +
		tr("Available version: %1").arg(version));
}

void TUpdateChecker::reportError(int error_number, QString error_str) {
	QWidget* p = qobject_cast<QWidget*>(parent());
	QMessageBox::warning(p, tr("Error"), 
		tr("An error happened while trying to retrieve information about the latest version available.") +
		"<br>" + tr("Error code: %1").arg(error_number) + "<br>" + error_str);
}

} // namespace Gui

#include "moc_updatechecker.cpp"

