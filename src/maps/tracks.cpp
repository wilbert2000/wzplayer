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

#include "tracks.h"

#include <QString>
#include <QRegExp>
#include "log4qt/logger.h"


namespace Maps {

QString TTrackData::getDisplayName() const {

    QString dname = QString::number(ID);

    if (!name.isEmpty()) {
        dname += " " + name;
    }

    if (!lang.isEmpty()) {
        dname += " ["+ lang + "]";
    }

    return dname;
}

TTracks::TTracks() :
    logger(Log4Qt::Logger::logger("Maps::TTracks")) {
}

void TTracks::addLang(int id, const QString &lang) {

    TTrackData& track = (*this)[id];
    track.setID(id);
    track.setLang(lang);
}

void TTracks::addName(int id, const QString &name) {

    TTrackData& track = (*this)[id];
    track.setID(id);
    track.setName(name);
}

void TTracks::addTrack(int id, const QString &lang, const QString &name) {

    TTrackData& track = (*this)[id];
    track.setID(id);
    track.setLang(lang);
    track.setName(name);
}

// For mplayer
bool TTracks::updateTrack(int id, const QString &field, const QString &value) {

    bool changed = false;

    iterator i = find(id);
    if (i == end()) {
        changed = true;
        if (field == "NAME") {
            addName(id, value);
        } else {
            addLang(id, value);
        }
    } else {
        TTrackData& track = i.value();
        if (field == "NAME") {
            if (track.getName() != value) {
                changed = true;
                track.setName(value);
            }
        } else {
            // LANG
            if (track.getLang() != value) {
                changed = true;
                track.setLang(value);
            }
        }
    }

    if (changed) {
        logger->debug("updateTrack updated track id " + QString::number(id)
                        + " field '" + field +"' to value '" + value + "'");
    } else {
        logger->debug("updateTrack track id %1 was up to date", id);
    }

    return changed;
}

bool TTracks::updateTrack(int ID, const QString &lang, const QString &name, bool selected) {

    bool changed = false;
    iterator i = find(ID);
    if (i == end()) {
        // New track
        changed = true;
        addTrack(ID, lang, name);
        if (selected)
            selectedID = ID;
        logger->debug("updateTrack added new track id " + QString::number(ID)
                      + " lang '" + lang + "' name '" + name + "'"
                      + " selected " + QString::number(selected));
    } else {
        // Existing track
        TTrackData& track = i.value();
        if (track.getLang() != lang) {
            logger->debug("updateTrack updating lang track id "
                          + QString::number(ID) + " from " + track.getLang()
                          + " to " + lang);
            track.setLang(lang);
            changed = true;
        }
        if (track.getName() != name) {
            logger->debug("updateTrack updating name track id "
                          + QString::number(ID) + " from "
                          + track.getName() + " to " + name);
            track.setName(name);
            changed = true;
        }

        if (selected) {
            if (selectedID != ID) {
                logger->debug("updateTrack changed selected id from %1 to %2",
                              selectedID, ID);
                selectedID = ID;
                changed = true;
            }
        } else if (selectedID == ID) {
            logger->debug("updateTrack changed selected id from %1 to -1", ID);
            selectedID = -1;
            changed = true;
        }
    }

    if (!changed)
        logger->debug("updateTrack track id %1 was up to date", ID);

    return changed;
}

// Select a track matching expr if only one track matches
int TTracks::findLangID(QString expr) const {
    logger->debug("findLangID '" + expr + "'");

    int id = -1;
    QRegExp rx(expr);
    TMapIterator i(*this);
    while (i.hasNext()) {
        i.next();
        TTrackData track = i.value();
        if (rx.indexIn(track.getLang()) >= 0) {
            if (id != -1) {
                // For complex formats it is not save to select just a track,
                // it can disable audio
                logger->info("findLang found multiple matching tracks,"
                             " canceling selection");
                return -1;
            }
            logger->debug("findLangID found preferred lang "
                          + track.getLang() + " matching " + expr);
            id = track.getID();
        }
    }

    return id;
}

void TTracks::list() const {
    logger->debug("list selected track %1", selectedID);

    TMapIterator i(*this);
    while (i.hasNext()) {
        i.next();
        TTrackData track = i.value();
        logger->debug("list ID: " + QString::number(track.getID())
                      + " lang: '" + track.getLang()
                      + "' name: '" + track.getName() + "'");
    }
}

} // namespace Maps
