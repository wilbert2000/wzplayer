/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#ifndef PROC_MPLAYERPROCESS_H
#define PROC_MPLAYERPROCESS_H

#include <QObject>
#include <QString>

#include "proc/playerprocess.h"
#include "mediadata.h"

class QStringList;

namespace Proc {

class TMPlayerProcess : public TPlayerProcess
{
	Q_OBJECT

public:
	TMPlayerProcess(QObject* parent, TMediaData* mdata);
	virtual ~TMPlayerProcess();

	virtual bool startPlayer();

	// Command line options
	void setMedia(const QString& media, bool is_playlist = false);
	void setFixedOptions();
	void disableInput();
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
	void setSecondarySubtitle(SubData::Type, int) {}
	void disableSecondarySubtitles() {}
	void setSubtitlesVisibility(bool b);
	void seekPlayerTime(double secs, int mode, bool precise, bool currently_paused);
	void mute(bool b);
	void setPause(bool pause);
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
	void setAngle(int angle);
	void nextAngle();
	void setExternalSubtitleFile(const QString& filename);
	void setSubPos(int pos);
	void setSubScale(double value);
	void setSubStep(int value);
	void seekSub(int);
	void setSubForcedOnly(bool b);
	void setSpeed(double value);
	void enableKaraoke(bool b);
	void enableExtrastereo(bool b);
	void enableVolnorm(bool b, const QString& option);
	void setAudioEqualizer(const QString& values);
	void setAudioDelay(double delay);
	void setSubDelay(double delay);
	void setLoop(int v);
	void takeScreenshot(ScreenshotType t, bool include_subtitles = false);
	void switchCapturing();
	void setTitle(int ID);
	void changeVF(const QString& filter, bool enable, const QVariant& option = QVariant());
	void changeStereo3DFilter(bool enable, const QString& in, const QString& out);

	void discSetMousePos(int x, int y);
	void discButtonPressed(const QString& button_name);

	void setAspect(double aspect);
	void setFullscreen(bool b);
#if PROGRAM_SWITCH
	void setTSProgram(int ID);
#endif
	void toggleDeinterlace();
	void setOSDPos(const QPoint& pos, int);
	void setOSDScale(double value);
	void setChannelsFile(const QString&) {}
	void setCaptureDirectory(const QString & dir);

	virtual void save();

protected:
	virtual int getFrame(double sec, const QString& line);

	virtual void playingStarted();
	virtual bool parseLine(QString& line);
	virtual bool parseStatusLine(double seconds, double duration, QRegExp& rx, QString& line);
	virtual bool parseAudioProperty(const QString& name, const QString& value);
	virtual bool parseVideoProperty(const QString& name, const QString& value);
	virtual bool parseProperty(const QString& name, const QString& value);

private:
	int sub_source;
	bool sub_file;
	bool sub_vob;
	bool sub_demux;
	int sub_file_id;

	double check_duration_time;
	int check_duration_time_diff;

	double frame_backstep_time_start;
	double frame_backstep_time_requested;

	bool video_tracks_changed;
	bool get_selected_video_track;
	bool audio_tracks_changed;
	bool get_selected_audio_track;
	bool subtitles_changed;
	bool get_selected_subtitle;

	int clip_info_id;
	QString clip_info_name;

	bool want_pause;
	bool mute_option_set;

	// Restore DVDNAV to pos before restart
	bool restore_dvdnav;
	int dvdnav_vts_to_restore;
	int dvdnav_title_to_restore_vts;
	int dvdnav_title_to_restore;
	double dvdnav_time_to_restore;

	void clearSubSources();
	void getSelectedSubtitles();
	void getSelectedTracks();
	void getSelectedAngle();
	void notifyChanges();

	bool setVideoTrack(int id);
	bool setAudioTrack(int id);

	bool titleChanged(TMediaData::Type type, int title);

	bool dvdnavVTSChanged(int vts);
	bool dvdnavTitleChanged(int title);
	bool dvdnavTitleIsMenu();
	void dvdnavSave();
	void dvdnavRestore();

	bool parseVO(const QString& driver, int w, int h);
	bool parseSubID(const QString& type, int id);
	bool parseSubTrack(const QString& type, int id, const QString& name, const QString& value);
	bool parseChapter(int id, const QString& type, const QString& value);
	bool parseClipInfoName(int id, const QString& name);
	bool parseClipInfoValue(int id, const QString& value);
	bool parseCDTrack(const QString& type, int id, const QString& length);
	bool parseTitleLength(int id, const QString& value);
	bool parseTitleChapters(Maps::TChapters& chapters, const QString& chaps);
	bool parseAnswer(const QString& name, const QString& value);
	bool parsePause();
	void convertTitlesToChapters();

private slots:
	void dvdnavRestoreTime();
};

} // namesapce Proc

#endif // PROC_MPLAYERPROCESS_H
