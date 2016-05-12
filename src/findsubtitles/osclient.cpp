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

#include "osclient.h"
#include "version.h"

OSClient::OSClient(QObject* parent) : 
    QObject(parent),
    debug(logger()),
    logged_in(false),
    search_size(0)
#ifdef OS_SEARCH_WORKAROUND
	, best_search_count(0)
	, search_retries(8)
#endif
{
	rpc = new MaiaXmlRpcClient(QUrl("http://api.opensubtitles.org/xml-rpc"), this);
}

void OSClient::setServer(const QString & server) {
	rpc->setUrl(QUrl(server));
}

#ifdef FS_USE_PROXY
void OSClient::setProxy(const QNetworkProxy & proxy) {
	rpc->setProxy(proxy);
}
#endif

void OSClient::login() {
    logger()->debug("login");

	// api.opensubtitles.org checks on user agent...
	// QString user_agent = "WZPlayer " + TVersion::version;
	QString user_agent = "SMPlayer v16.1.0.0";
    logger()->debug("OSClient::login: user agent: '" + user_agent + "'");

	QVariantList args;

	args << "" << "" << "" << user_agent;

	rpc->call("LogIn", args,
			  this, SLOT(responseLogin(QVariant &)),
			  this, SLOT(gotFault(int, const QString &)));
}

void OSClient::search(const QString & hash, qint64 file_size) {
    debug << "search: hash: " << hash << "file_size: " << file_size << debug;

	search_hash = hash;
	search_size = file_size;

	disconnect(this, SIGNAL(loggedIn()), this, SLOT(doSearch()));

	#if 0
	if (logged_in) {
		doSearch();
	} else {
		connect(this, SIGNAL(loggedIn()), this, SLOT(doSearch()));
		login();
	}
	#else
	connect(this, SIGNAL(loggedIn()), this, SLOT(doSearch()));
	login();
	#endif
}

#ifdef OS_SEARCH_WORKAROUND
void OSClient::doSearch() {
	best_search_count = -1;
	for (int n = 1; n <= search_retries; n++) doSearch(n);
}

void OSClient::doSearch(int nqueries) {
#else
void OSClient::doSearch() {
#endif
    logger()->debug("doSearch");

	QVariantMap m;
	m["sublanguageid"] = "all";
	m["moviehash"] = search_hash;
	m["moviebytesize"] = QString::number(search_size);

	QVariantList list;
#ifdef OS_SEARCH_WORKAROUND
	// Sometimes opensubtitles return 0 subtitles
	// A workaround seems to add the query several times
    logger()->debug("doSearch: nqueries %1", nqueries);
	for (int count = 0; count < nqueries; count++) list.append(m);
#else
	list.append(m);
#endif

	QVariantList args;
	args << token << QVariant(list);

	rpc->call("SearchSubtitles", args,
			  this, SLOT(responseSearch(QVariant &)),
			  this, SLOT(gotFault(int, const QString &)));
}

void OSClient::responseLogin(QVariant &arg) {
    logger()->debug("responseLogin");

	QVariantMap m = arg.toMap();
	QString status = m["status"].toString();
	QString t = m["token"].toString();

    logger()->debug("responseLogin: status: '" + status + "'");
    logger()->debug("responseLogin: token: '" + t + "'");

	if (status == "200 OK") {
		token = t;
		logged_in = true;
		emit loggedIn();
	} else {
		emit loginFailed();
	}
}

void OSClient::responseSearch(QVariant &arg) {
    logger()->debug("responseSearch");

	QVariantMap m = arg.toMap();
	QString status = m["status"].toString();

    logger()->debug("responseSearch: status: '" + status + "'");

	if (status != "200 OK") {
		emit searchFailed();
		return;
	}

	s_list.clear();

	QVariantList data = m["data"].toList();
    logger()->debug("responseSearch: data count %1", data.count());

#ifdef OS_SEARCH_WORKAROUND
	if (best_search_count >= data.count()) {
        logger()->debug("responseSearch: we already have a better search (%1)."
                        " Ignoring this one.", best_search_count);
		return;
	}
	best_search_count = data.count();
#endif

	for (int n = 0; n < data.count(); n++) {
		OSSubtitle sub;

		QVariantMap m = data[n].toMap();

		sub.releasename = m["MovieReleaseName"].toString();
		sub.movie = m["MovieName"].toString();
#ifdef USE_QUAZIP
		sub.link = m["ZipDownloadLink"].toString();
#else
		sub.link = m["SubDownloadLink"].toString();
#endif
		sub.date = m["SubAddDate"].toString();
		sub.iso639 = m["ISO639"].toString();
		sub.rating = m["SubRating"].toString();
		sub.comments = m["SubAuthorComment"].toString();
		sub.format = m["SubFormat"].toString();
		sub.language = m["LanguageName"].toString();
		sub.user = m["UserNickName"].toString();
		sub.files = "1";

		s_list.append(sub);
	}

	emit searchFinished();
}

void OSClient::gotFault(int error, const QString &message) {
    logger()->debug("gotFault: error: " + QString::number(error)
                    + ", message: '" + message + "'");
	emit errorFound(error, message);
}

#include "moc_osclient.cpp"
