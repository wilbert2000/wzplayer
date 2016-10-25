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

#ifndef PLAYER_PROCESS_MPVPROCESS_H
#define PLAYER_PROCESS_MPVPROCESS_H

#include <QObject>
#include <QPoint>
#include <QString>
#include <QTime>

#include "player/process/playerprocess.h"

class QStringList;

namespace Player {
namespace Process {

class TMPVProcess : public TPlayerProcess {
    Q_OBJECT
    DECLARE_QCLASS_LOGGER

public:
    TMPVProcess(QObject* parent, TMediaData* mdata);
    ~TMPVProcess();

    virtual bool startPlayer();

    // Command line options
    void setMedia(const QString& media);
    void disableInput();
    void setFixedOptions();
    void setOption(const QString& name, const QVariant& value = QVariant());
    void addUserOption(const QString& option);
    void addVF(const QString& filter_name, const QVariant& value = QVariant());
    void addAF(const QString& filter_name, const QVariant& value = QVariant());
    void addStereo3DFilter(const QString& in, const QString& out);
    void setSubStyles(const Settings::TAssStyles& styles, const QString& assStylesFile = QString::null);

    // Slave commands
    void setVolume(int v);
    void setOSDLevel(int level);
    void setAudio(int ID);
    void setVideo(int ID);
    void setSubtitle(SubData::Type type, int ID);
    void disableSubtitles();
    void setSecondarySubtitle(SubData::Type type, int ID);
    void disableSecondarySubtitles();
    void setSubtitlesVisibility(bool b);
    void seekPlayerTime(double secs, int mode, bool precise, bool currently_paused);
    void mute(bool b);
    void setPause(bool b);
    void frameStep();
    void frameBackStep();
    void showOSDText(const QString& text, int duration, int level);
    void showFilenameOnOSD();
    void showTimeOnOSD();
    void setContrast(int value);
    void setBrightness(int value);
    void setHue(int value);
    void setSaturation(int value);
    void setGamma(int value);
    void setChapter(int ID);
    void nextChapter(int delta);
    void setAngle(int ID);
    void nextAngle();
    void setExternalSubtitleFile(const QString& filename);
    void setSubPos(int pos);
    void setSubScale(double value);
    void setSubStep(int value);
    void seekSub(int value);
    void setSubForcedOnly(bool b);
    void setSpeed(double value);
    void enableKaraoke(bool);
    void enableExtrastereo(bool);
    void enableVolnorm(bool b, const QString& option);
    void setAudioEqualizer(const QString& values);
    void setAudioDelay(double delay);
    void setSubDelay(double delay);
    void setLoop(int v);
    void takeScreenshot(ScreenshotType t, bool include_subtitles = false);
    void switchCapturing();
    void setTitle(int ID);
    void changeVF(const QString& filter,
                  bool enable,
                  const QVariant& option = QVariant());
    void changeStereo3DFilter(bool enable,
                              const QString& in,
                              const QString& out);

    void discSetMousePos(int, int);
    void discButtonPressed(const QString& button_name);

    void setAspect(double aspect);

#if PROGRAM_SWITCH
    void setTSProgram(int ID);
#endif
    void toggleDeinterlace();
    void setOSDScale(double value);
    void setChannelsFile(const QString &);

    virtual void save();

protected:
    virtual void playingStarted();
    virtual void checkTime(double sec);

    virtual bool parseLine(QString& line);
    virtual bool parseProperty(const QString& name, const QString& value);
    bool isOptionAvailable(const QString& option);
    void addVFIfAvailable(const QString& vf, const QString& value = QString::null);

protected slots:
    void requestChapterInfo();

private:
    bool received_buffering;

    int selected_title;
    bool received_title_not_found;
    bool title_swictched;
    double title_switch_time;
    bool quit_at_end_of_title;
    int quit_at_end_of_title_ms;
    QTime quit_at_end_of_title_time;

    bool request_bit_rate_info;

    QString sub_file;

    QString previous_eq;

    bool capturing;

    void convertChaptersToTitles();
    void fixTitle();
    bool parseStatusLine(const QRegExp& rx);
    bool parseChapter(int id, double start, QString title);
    bool parseTitleSwitched(QString disc_type, int title);
    bool parseTitleNotFound(const QString& disc_type);
    bool parseVideoTrack(int id, QString name, bool selected);
    bool parseAudioTrack(int id, const QString& lang, QString name, bool selected);
    bool parseSubtitleTrack(int id, const QString& lang, QString name, QString type, bool selected);
    bool parseMetaDataList(QString list);
    void requestBitrateInfo();
};

} // namespace Process
} // namespace Player

#endif // PLAYER_PROCESS_MPVPROCESS_H
