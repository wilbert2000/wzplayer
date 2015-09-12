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

#ifndef _MPLAYERPROCESS_H_
#define _MPLAYERPROCESS_H_

#include <QString>
#include "playerprocess.h"
#include "mediadata.h"
#include "config.h"

class QStringList;

class MplayerProcess : public PlayerProcess
{
	Q_OBJECT

public:
	MplayerProcess(MediaData * mdata);
	~MplayerProcess();

	virtual bool startPlayer();

	// Command line options
	void setMedia(const QString & media, bool is_playlist = false);
	void setFixedOptions();
	void disableInput();
	void setOption(const QString & option_name, const QVariant & value = QVariant());
	void addUserOption(const QString & option);
	void addVF(const QString & filter_name, const QVariant & value = QVariant());
	void addAF(const QString & filter_name, const QVariant & value = QVariant());
	void addStereo3DFilter(const QString & in, const QString & out);
	void setSubStyles(const AssStyles & styles, const QString & assStylesFile = QString::null);

	// Slave commands
	void quit();
	void setVolume(int v);
	void setOSDLevel(int level);
	void setAudio(int ID);
	void setVideo(int ID);
	void setSubtitle(SubData::Type type, int ID);
	void disableSubtitles();
	void setSecondarySubtitle(int) {}
	void disableSecondarySubtitles() {}
	void setSubtitlesVisibility(bool b);
	void seekPlayerTime(double secs, int mode, bool precise, bool currently_paused);
	void mute(bool b);
	void setPause(bool pause);
	void frameStep();
	void frameBackStep();
	void showOSDText(const QString & text, int duration, int level);
	void showFilenameOnOSD();
	void showTimeOnOSD();
	void setContrast(int value);
	void setBrightness(int value);
	void setHue(int value);
	void setSaturation(int value);
	void setGamma(int value);
	void setChapter(int ID);
	void setExternalSubtitleFile(const QString & filename);
	void setSubPos(int pos);
	void setSubScale(double value);
	void setSubStep(int value);
	void setSubForcedOnly(bool b);
	void setSpeed(double value);
	void enableKaraoke(bool b);
	void enableExtrastereo(bool b);
	void enableVolnorm(bool b, const QString & option);
	void setAudioEqualizer(const QString & values);
	void setAudioDelay(double delay);
	void setSubDelay(double delay);
	void setLoop(int v);
	void takeScreenshot(ScreenshotType t, bool include_subtitles = false);
	void setTitle(int ID);
	void changeVF(const QString & filter, bool enable, const QVariant & option = QVariant());
	void changeStereo3DFilter(bool enable, const QString & in, const QString & out);
#if DVDNAV_SUPPORT
	void discSetMousePos(int x, int y);
	void discButtonPressed(const QString & button_name);
#endif
	void setAspect(double aspect);
	void setFullscreen(bool b);
#if PROGRAM_SWITCH
	void setTSProgram(int ID);
#endif
	void toggleDeinterlace();
	void setOSDPos(const QPoint &pos);
	void setOSDScale(double value);
	void setChannelsFile(const QString &) {};


protected:
	virtual int getFrame(double sec, const QString &line);
	virtual void correctDuration(double sec);

	virtual bool parseLine(QString &line);
	virtual bool parseStatusLine(double seconds, double duration, QRegExp &rx, QString &line);
	virtual bool parseAudioProperty(const QString &name, const QString &value);
	virtual bool parseVideoProperty(const QString &name, const QString &value);
	virtual bool parseProperty(const QString &name, const QString &value);

private:
	int svn_version;

	int sub_source;
	bool sub_file;
	bool sub_vob;
	bool sub_demux;
	int sub_file_id;

	double check_duration_time;
	int check_duration_time_diff;
	bool corrected_duration;

	bool video_tracks_changed;
	bool audio_tracks_changed;
	bool subtitle_tracks_changed;
	bool title_tracks_changed;

	bool want_pause;

	void getSelectedTracks();
	void notifyChanges();

	bool parseVO(const QString &driver, int w, int h);
	bool parseSubID(const QString &type, int id);
	bool parseSubTrack(const QString &type, int id, const QString &name, const QString &value);
	bool parseChapter(int id, const QString &type, const QString &value);
	bool parseCDTrack(const QString &type, int id, const QString &length);
	bool parseTitle(int id, const QString &field, const QString &value);
	bool parseTitleChapters(QString chapters);
	bool parseAnswer(const QString &name, const QString &value);
	bool parsePause();
	void convertTitlesToChapters();
};

#endif
