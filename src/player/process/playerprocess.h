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

#ifndef PLAYER_PROCESS_PLAYERPROCESS_H
#define PLAYER_PROCESS_PLAYERPROCESS_H

#include "player/process/process.h"
#include "settings/assstyles.h"
#include "subtracks.h"

#include <QTemporaryFile>
#include <QTime>

class QRegExp;
class TMediaData;

namespace Player {
namespace Process {

class TPlayerProcess : public TProcess {
    Q_OBJECT
public:
    enum ScreenshotType { Single = 0, Multiple = 1 };

    static TPlayerProcess* createPlayerProcess(QObject* parent,
                                               const QString& name,
                                               TMediaData* md);

    explicit TPlayerProcess(QObject* parent,
                            const QString& name,
                            TMediaData* mdata);

    QTime startTime;

    bool isRunning() const { return state() == QProcess::Running; }
    bool isReady() const {
        return notified_player_is_running
               && !received_end_of_file
               && isRunning();
    }
    bool isBuffering() const { return buffering; }

    virtual bool startPlayer();

    void writeToPlayer(const QString& text, bool log = true);

    // Command line options
    virtual void setMedia(const QString& media) = 0;
    virtual void setFixedOptions() = 0;
    virtual void disableInput() = 0;
    virtual void setOption(const QString& option_name,
                           const QVariant& value = QVariant()) = 0;
    virtual void addUserOption(const QString& option) = 0;
    virtual void addVideoFilter(const QString& filter_name,
                                const QVariant& value = QVariant()) = 0;
    virtual void addAudioFilter(const QString& filter_name,
                       const QVariant& value = QVariant()) = 0;
    virtual void addStereo3DFilter(const QString& in, const QString& out) = 0;
    virtual void setSubStyles(const Settings::TAssStyles& styles,
                              const QString& assStylesFile = QString::null) = 0;
    virtual void setImageDuration(int durationSec);

    // Slave commands
    void quit(int exit_code);
    virtual void setVolume(int v) = 0;
    virtual void setOSDLevel(int level) = 0;
    virtual void setAudio(int ID) = 0;
    virtual void setVideo(int ID) = 0;
    virtual void setSubtitle(SubData::Type type, int ID) = 0;
    virtual void disableSubtitles() = 0;
    virtual void setSecondarySubtitle(SubData::Type type, int ID) = 0;
    virtual void disableSecondarySubtitles() = 0;
    virtual void setSubtitlesVisibility(bool b) = 0;
    virtual void seekPlayerTime(double secs, int mode, bool keyframes,
                                bool currently_paused) = 0;
    virtual void seek(double secs, int mode, bool keyframes,
                      bool currently_paused);
    virtual void mute(bool b) = 0;
    virtual void setPause(bool b) = 0;
    virtual void frameStep() = 0;
    virtual void frameBackStep() = 0;
    virtual void showOSDText(const QString& text, int duration, int level) = 0;
    virtual void showFilenameOnOSD() = 0;
    virtual void showTimeOnOSD() = 0;
    virtual void setContrast(int value) = 0;
    virtual void setBrightness(int value) = 0;
    virtual void setHue(int value) = 0;
    virtual void setSaturation(int value) = 0;
    virtual void setGamma(int value) = 0;
    virtual void setChapter(int ID) = 0;
    virtual void nextChapter(int delta) = 0;
    virtual void setAngle(int ID) = 0;
    virtual void nextAngle() = 0;
    virtual void setExternalSubtitleFile(const QString& filename) = 0;
    virtual void setSubPos(int pos) = 0;
    virtual void setSubScale(double value) = 0;
    virtual void setSubStep(int value) = 0;
    virtual void seekSub(int value) = 0;
    virtual void setSubForcedOnly(bool b) = 0;
    virtual void setSpeed(double value) = 0;
    virtual void enableKaraoke(bool b) = 0;
    virtual void enableExtrastereo(bool b) = 0;
    virtual void enableVolnorm(bool b, const QString& option) = 0;
    virtual void setAudioEqualizer(const QString& values) = 0;
    virtual void setAudioDelay(double delay) = 0;
    virtual void setSubDelay(double delay) = 0;
    virtual void setLoop(int v) = 0;
    virtual void takeScreenshot(ScreenshotType t,
                                bool include_subtitles = false) = 0;
    virtual void switchCapturing() = 0;
    virtual void setTitle(int ID) = 0;
    virtual void setVideoFilter(const QString& filter, bool enable,
                          const QVariant& option = QVariant()) = 0;
    virtual void setStereo3DFilter(bool enable, const QString& in,
                                      const QString& out) = 0;

    virtual void discSetMousePos(int x, int y) = 0;
    virtual void discButtonPressed(const QString& button_name) = 0;

    virtual void setAspect(double aspect) = 0;

    virtual void toggleDeinterlace() = 0;
    virtual void setOSDScale(double value) = 0;

    void setScreenshotDirectory(const QString& dir) { screenshot_dir = dir; }
    QString screenshotDirectory() const { return screenshot_dir; }
    virtual void setCaptureDirectory(const QString& dir);

    // Save current state to restore it after a restart
    virtual void save() = 0;

// Signals
signals:
    void processFinished(bool normal_exit, int exit_code, bool eof);

    void playingStarted();

    void receivedVideoOut();
    void durationChanged(int ms);
    void receivedPositionMS(int ms);
    void receivedPause();

    void receivedMessage(const QString&);
    void receivedBuffering();
    void receivedBufferingEnded();
    void receivedScreenshot(const QString&);
    void receivedUpdatingFontCache();

    void receivedStreamTitle();

    //! Emitted if one or more video track(s) added or changed
    void receivedVideoTracks();
    //! Emitted if player changed video track
    void receivedVideoTrackChanged(int);

    //! Emitted if audio track(s) added or changed
    void receivedAudioTracks();
    //! Emitted if player changed audio track
    void receivedAudioTrackChanged(int);

    //! Emitted if a new subtitle has been added or an old one changed
    void receivedSubtitleTracks();
    //! Emitted if player changed subtitle track
    void receivedSubtitleTrackChanged();

    void receivedTitleTracks();
    void receivedTitleTrackChanged(int);

    void receivedChapters();
    void receivedAngles();

    void videoBitRateChanged(int bitrate);
    void audioBitRateChanged(int bitrate);

protected:
    TMediaData* md;

    QString screenshot_dir;
    QString capture_filename;

    bool notified_player_is_running;
    int waiting_for_answers;
    bool paused;
    bool buffering;

    bool received_end_of_file;
    void setEOF();

    bool quit_send;
    int exit_code_override;

    QString temp_file_name;

    double guiTimeToPlayerTime(double sec);
    int playerTimeToGuiTime(int ms);

    void notifyTitleTrackChanged(int new_title);
    void notifyDuration(double durationSec, bool forceEmit = false);
    virtual void checkTime(int ms);
    void notifyTime(double time_sec);
    bool waitForAnswers();

    virtual void notifyPlayingStarted();
    virtual bool parseLine(QString& line);
    virtual bool parseAudioProperty(const QString& name, const QString& value);
    virtual bool parseVideoProperty(const QString& name, const QString& value);
    virtual bool parseMetaDataProperty(QString name, QString value);
    virtual bool parseProperty(const QString& name, const QString& value);

protected slots:
    virtual void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    int line_count;
    int waiting_for_answers_safe_guard;

    QTemporaryFile temp_file;

    bool parseAngle(const QString& value);
    bool parseVO(const QString& vo, int sw, int sh, int dw, int dh);
};

} // namespace Process
} // namespace Player

#endif // PLAYER_PROCESS_PLAYERPROCESS_H
