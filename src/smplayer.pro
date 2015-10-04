TEMPLATE = app
LANGUAGE = C++

CONFIG += qt warn_on
CONFIG += release
#CONFIG += debug

QT += network xml

RESOURCES = icons.qrc

#DEFINES += EXPERIMENTAL
DEFINES += SINGLE_INSTANCE
DEFINES += FIND_SUBTITLES
DEFINES += VIDEOPREVIEW
DEFINES += YOUTUBE_SUPPORT
DEFINES += OUTPUT_ON_CONSOLE
DEFINES += MPCGUI
DEFINES += SKINS
DEFINES += MPRIS2
DEFINES += UPDATE_CHECKER
DEFINES += CHECK_UPGRADED
#DEFINES += USE_FONTCONFIG_OPTIONS
DEFINES += AUTO_SHUTDOWN_PC

#DEFINES += REMINDER_ACTIONS
#DEFINES += SHAREWIDGET

DEFINES += MPV_SUPPORT
DEFINES += MPLAYER_SUPPORT

#DEFINES += SIMPLE_BUILD

contains( DEFINES, SIMPLE_BUILD ) {
	DEFINES -= SINGLE_INSTANCE
	DEFINES -= FIND_SUBTITLES
	DEFINES -= VIDEOPREVIEW
	DEFINES -= MPCGUI
	DEFINES -= SKINS
	DEFINES -= MPRIS2
	DEFINES -= UPDATE_CHECKER
	DEFINES -= CHECK_UPGRADED
	DEFINES -= REMINDER_ACTIONS
	DEFINES -= SHAREWIDGET
	DEFINES -= AUTO_SHUTDOWN_PC
}

isEqual(QT_MAJOR_VERSION, 5) {
	QT += widgets gui
	#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x040000
	DEFINES -= MPCGUI
	win32 {
		DEFINES -= MPRIS2
	}
}

contains(QT_VERSION, ^4\\.[0-3]\\..*) {
	message("Some features requires Qt > 4.3.")

	contains( DEFINES, SINGLE_INSTANCE ) {
		DEFINES -= SINGLE_INSTANCE
		message("SINGLE_INSTANCE disabled.")
	}

	contains( DEFINES, YOUTUBE_SUPPORT ) {
		DEFINES -= YOUTUBE_SUPPORT
		message("YOUTUBE_SUPPORT disabled.")
	}

	contains( DEFINES, SKINS ) {
		DEFINES -= SKINS
		message("SKINS disabled.")
	}

	contains( DEFINES, FIND_SUBTITLES ) {
		DEFINES -= FIND_SUBTITLES
		message("FIND_SUBTITLES disabled.")
	}

	contains( DEFINES, UPDATE_CHECKER ) {
		DEFINES -= UPDATE_CHECKER
		message("UPDATE_CHECKER disabled.")
	}
}

HEADERS += guiconfig.h \
	config.h \
	links.h \
	svn_revision.h \
	version.h \
	paths.h \
	helper.h \
	colorutils.h \
	subtracks.h \
	discname.h \
	extensions.h \
	desktopinfo.h \
	mplayerversion.h \
	playerid.h \
	proc/process.h \
	proc/playerprocess.h \
	infoprovider.h \
	mplayerwindow.h \
	mediadata.h \
	audioequalizerlist.h \
	mediasettings.h \
	assstyles.h \
    settings/filters.h \
    settings/smplayersettings.h \
    settings/preferences.h \
    settings/filesettingsbase.h \
    settings/filesettings.h \
    filehash.h \
    settings/filesettingshash.h \
    settings/tvsettings.h \
    settings/recents.h \
    settings/urlhistory.h \
    images.h \
	inforeader.h \
	deviceinfo.h \
	core.h \
	infofile.h \
	seekwidget.h \
	shortcutgetter.h \
	actionseditor.h \
	filechooser.h \
	lineedit_with_icon.h \
	languages.h \
    pref/vdpauproperties.h \
    pref/selectcolorbutton.h \
    pref/tristatecombo.h \
    pref/combobox.h \
	pref/dialog.h \
	pref/widget.h \
	pref/general.h \
	pref/drives.h \
	pref/interface.h \
	pref/performance.h \
	pref/input.h \
	pref/subtitles.h \
	pref/advanced.h \
	pref/prefplaylist.h \
	pref/tv.h \
	pref/updates.h \
	pref/network.h \
	filepropertiesdialog.h \
	multilineinputdialog.h \
	verticaltext.h \
	inputdvddirectory.h \
	stereo3ddialog.h \
	filedialog.h \
	inputmplayerversion.h \
	favorites.h \
	tvlist.h \
	favoriteeditor.h \
    gui/eqslider.h \
    gui/videoequalizer.h \
    gui/audioequalizer.h \
    gui/errordialog.h \
    gui/about.h \
    gui/timedialog.h \
    gui/lineedit.h \
    gui/inputurl.h \
    gui/action.h \
	gui/actiongroup.h \
	gui/widgetactions.h \
	gui/editabletoolbar.h \
	gui/slider.h \
	gui/timeslider.h \
    gui/autohidewidget.h \
	gui/tablewidget.h \
	gui/playlist.h \
	gui/playlistdock.h \
    gui/base.h \
	gui/baseplus.h \
    gui/default.h \
    gui/mini.h \
    gui/logwindow.h \
    toolbareditor.h \
	clhelp.h \
	cleanconfig.h \
	smplayer.h \
    maps/map.h \
    maps/tracks.h \
    maps/titletracks.h \
    maps/chapters.h \
    log.h


SOURCES	+= version.cpp \
	paths.cpp \
	helper.cpp \
	colorutils.cpp \
	subtracks.cpp \
	discname.cpp \
	extensions.cpp \
	desktopinfo.cpp \
	mplayerversion.cpp \
	playerid.cpp \
	proc/process.cpp \
	proc/playerprocess.cpp \
	infoprovider.cpp \
	mplayerwindow.cpp \
	mediadata.cpp \
	mediasettings.cpp \
	assstyles.cpp \
    settings/filters.cpp \
    settings/smplayersettings.cpp \
    settings/preferences.cpp \
    settings/filesettingsbase.cpp \
    settings/filesettings.cpp \
    filehash.cpp \
    settings/filesettingshash.cpp \
    settings/tvsettings.cpp \
    settings/recents.cpp \
    settings/urlhistory.cpp \
    images.cpp \
	inforeader.cpp \
	deviceinfo.cpp \
	core.cpp \
	infofile.cpp \
	seekwidget.cpp \
	shortcutgetter.cpp \
	actionseditor.cpp \
	filechooser.cpp \
	lineedit_with_icon.cpp \
	languages.cpp \
    pref/vdpauproperties.cpp \
    pref/selectcolorbutton.cpp \
    pref/tristatecombo.cpp \
    pref/combobox.cpp \
	pref/dialog.cpp \
	pref/widget.cpp \
	pref/general.cpp \
	pref/drives.cpp \
	pref/interface.cpp \
	pref/performance.cpp \
	pref/input.cpp \
	pref/subtitles.cpp \
	pref/advanced.cpp \
	pref/prefplaylist.cpp \
	pref/tv.cpp \
	pref/updates.cpp \
	pref/network.cpp \
	filepropertiesdialog.cpp \
	multilineinputdialog.cpp \
	verticaltext.cpp \
	inputdvddirectory.cpp \
	stereo3ddialog.cpp \
	filedialog.cpp \
	inputmplayerversion.cpp \
	favorites.cpp \
	tvlist.cpp \
	favoriteeditor.cpp \
    gui/eqslider.cpp \
    gui/videoequalizer.cpp \
    gui/audioequalizer.cpp \
    gui/errordialog.cpp \
    gui/about.cpp \
    gui/timedialog.cpp \
    gui/lineedit.cpp \
    gui/inputurl.cpp \
    gui/action.cpp \
	gui/actiongroup.cpp \
	gui/widgetactions.cpp \
	gui/editabletoolbar.cpp \
	gui/slider.cpp \
	gui/timeslider.cpp \
    gui/autohidewidget.cpp \
   	gui/tablewidget.cpp \
	gui/playlist.cpp \
	gui/playlistdock.cpp \
    gui/base.cpp \
	gui/baseplus.cpp \
    gui/default.cpp \
    gui/mini.cpp \
    gui/logwindow.cpp \
    toolbareditor.cpp \
	clhelp.cpp \
	cleanconfig.cpp \
	smplayer.cpp \
    maps/map.cpp \
    maps/tracks.cpp \
    maps/titletracks.cpp \
    main.cpp \
    maps/chapters.cpp \
    log.cpp

FORMS = inputdvddirectory.ui gui/logwindow.ui filepropertiesdialog.ui \
        gui/eqslider.ui seekwidget.ui gui/inputurl.ui gui/videoequalizer.ui pref/vdpauproperties.ui \
        pref/dialog.ui pref/general.ui pref/drives.ui pref/interface.ui \
        pref/performance.ui pref/input.ui pref/subtitles.ui pref/advanced.ui \
        pref/prefplaylist.ui pref/tv.ui pref/updates.ui pref/network.ui favoriteeditor.ui \
        gui/about.ui inputmplayerversion.ui gui/errordialog.ui gui/timedialog.ui stereo3ddialog.ui \
        toolbareditor.ui multilineinputdialog.ui

contains( DEFINES, MPV_SUPPORT ) {
    HEADERS += proc/mpvprocess.h inforeadermpv.h
    SOURCES += proc/mpvprocess.cpp inforeadermpv.cpp
}

contains( DEFINES, MPLAYER_SUPPORT ) {
    HEADERS += proc/mplayerprocess.h inforeadermplayer.h
    SOURCES += proc/mplayerprocess.cpp inforeadermplayer.cpp
}

# qtsingleapplication
contains( DEFINES, SINGLE_INSTANCE ) {
	INCLUDEPATH += qtsingleapplication
	DEPENDPATH += qtsingleapplication

	SOURCES += qtsingleapplication/qtsingleapplication.cpp qtsingleapplication/qtlocalpeer.cpp
	HEADERS += qtsingleapplication/qtsingleapplication.h qtsingleapplication/qtlocalpeer.h
}

# Find subtitles dialog
contains( DEFINES, FIND_SUBTITLES ) {
	DEFINES += DOWNLOAD_SUBS
	DEFINES += OS_SEARCH_WORKAROUND
	#DEFINES += USE_QUAZIP

	INCLUDEPATH += findsubtitles
	DEPENDPATH += findsubtitles

	INCLUDEPATH += findsubtitles/maia
	DEPENDPATH += findsubtitles/maia

	HEADERS += findsubtitles/findsubtitlesconfigdialog.h findsubtitles/findsubtitleswindow.h
	SOURCES += findsubtitles/findsubtitlesconfigdialog.cpp findsubtitles/findsubtitleswindow.cpp
	FORMS += findsubtitles/findsubtitleswindow.ui findsubtitles/findsubtitlesconfigdialog.ui

	# xmlrpc client code to connect to opensubtitles.org
	HEADERS += findsubtitles/maia/maiaObject.h findsubtitles/maia/maiaFault.h findsubtitles/maia/maiaXmlRpcClient.h findsubtitles/osclient.h
	SOURCES += findsubtitles/maia/maiaObject.cpp findsubtitles/maia/maiaFault.cpp findsubtitles/maia/maiaXmlRpcClient.cpp findsubtitles/osclient.cpp
}

# Download subtitles
contains( DEFINES, DOWNLOAD_SUBS ) {
	INCLUDEPATH += findsubtitles/filedownloader
	DEPENDPATH += findsubtitles/filedownloader

	HEADERS += findsubtitles/filedownloader/filedownloader.h findsubtitles/subchooserdialog.h findsubtitles/fixsubs.h
	SOURCES += findsubtitles/filedownloader/filedownloader.cpp findsubtitles/subchooserdialog.cpp findsubtitles/fixsubs.cpp

	FORMS += findsubtitles/subchooserdialog.ui

	contains( DEFINES, USE_QUAZIP ) {
		INCLUDEPATH += findsubtitles/quazip
		DEPENDPATH += findsubtitles/quazip

		HEADERS += crypt.h \
		           ioapi.h \
		           quazip.h \
		           quazipfile.h \
		           quazipfileinfo.h \
		           quazipnewinfo.h \
		           unzip.h \
		           zip.h

		SOURCES += ioapi.c \
		           quazip.cpp \
		           quazipfile.cpp \
		           quazipnewinfo.cpp \
		           unzip.c \
		           zip.c
}

	LIBS += -lz
	
	win32 {
		INCLUDEPATH += ..\\zlib
		LIBS += -L..\\zlib
	}
}

# Youtube support
contains( DEFINES, YOUTUBE_SUPPORT ) {
	DEFINES += YT_USE_SCRIPT
	INCLUDEPATH += youtube
	DEPENDPATH += youtube

	HEADERS += youtube/retrieveyoutubeurl.h youtube/loadpage.h
	SOURCES += youtube/retrieveyoutubeurl.cpp youtube/loadpage.cpp

	contains( DEFINES, YT_USE_SCRIPT ) {
		DEFINES += YT_USE_SIG
		DEFINES += YT_USE_YTSIG
		QT += script
	}

	contains( DEFINES, YT_USE_SIG ) {
		HEADERS += youtube/sig.h
		SOURCES += youtube/sig.cpp
	}

	contains( DEFINES, YT_USE_YTSIG ) {
		HEADERS += youtube/ytsig.h youtube/codedownloader.h
		SOURCES += youtube/ytsig.cpp youtube/codedownloader.cpp
	}
}

# mpcgui
contains( DEFINES, MPCGUI ) {
	INCLUDEPATH += gui/mpc
	DEPENDPATH += gui/mpc

	HEADERS += gui/mpc/mpc.h gui/mpc/styles.h
	SOURCES += gui/mpc/mpc.cpp gui/mpc/styles.cpp
}

# Skins support
contains( DEFINES, SKINS ) {
	INCLUDEPATH += gui/skin
	DEPENDPATH += gui/skin

    HEADERS += gui/skin/icon.h gui/skin/button.h gui/skin/panelseeker.h \
               gui/skin/playcontrol.h gui/skin/mediapanel.h \
               gui/skin/volumecontrolpanel.h gui/skin/mediabarpanel.h \
               gui/skin/iconsetter.h gui/skin/actiontools.h gui/skin/skin.h
    SOURCES += gui/skin/icon.cpp gui/skin/button.cpp gui/skin/panelseeker.cpp \
               gui/skin/playcontrol.cpp gui/skin/mediapanel.cpp \
               gui/skin/volumecontrolpanel.cpp gui/skin/mediabarpanel.cpp \
               gui/skin/iconsetter.cpp gui/skin/actiontools.cpp gui/skin/skin.cpp
	FORMS += gui/skin/mediapanel.ui gui/skin/mediabarpanel.ui
}

contains( DEFINES, MPRIS2 ) {
	INCLUDEPATH += mpris2
	DEPENDPATH += mpris2

	HEADERS += mpris2/mediaplayer2.h mpris2/mediaplayer2player.h mpris2/mpris2.h
	SOURCES += mpris2/mediaplayer2.cpp mpris2/mediaplayer2player.cpp mpris2/mpris2.cpp

	QT += dbus
}

# Update checker
contains( DEFINES, UPDATE_CHECKER ) {
	HEADERS += updatechecker.h updatecheckerdata.h
	SOURCES += updatechecker.cpp updatecheckerdata.cpp
}

# Videopreview
contains( DEFINES, VIDEOPREVIEW ) {
	INCLUDEPATH += videopreview
	DEPENDPATH += videopreview

	HEADERS += videopreview/videopreview.h videopreview/videopreviewconfigdialog.h
	SOURCES += videopreview/videopreview.cpp videopreview/videopreviewconfigdialog.cpp

	FORMS += videopreview/videopreviewconfigdialog.ui
}

contains( DEFINES, REMINDER_ACTIONS ) {
	HEADERS += sharedialog.h
	SOURCES += sharedialog.cpp
	FORMS += sharedialog.ui
}

contains( DEFINES, SHAREWIDGET|REMINDER_ACTIONS ) {
	HEADERS += sharewidget.h sharedata.h
	SOURCES += sharewidget.cpp sharedata.cpp
}

contains( DEFINES, AUTO_SHUTDOWN_PC ) {
	HEADERS += shutdowndialog.h shutdown.h
	SOURCES += shutdowndialog.cpp shutdown.cpp
	FORMS += shutdowndialog.ui

	unix { QT += dbus }
}

unix {
	UI_DIR = .ui
	MOC_DIR = .moc
	OBJECTS_DIR = .obj

	DEFINES += DATA_PATH=$(DATA_PATH)
	DEFINES += DOC_PATH=$(DOC_PATH)
	DEFINES += TRANSLATION_PATH=$(TRANSLATION_PATH)
	DEFINES += THEMES_PATH=$(THEMES_PATH)
	DEFINES += SHORTCUTS_PATH=$(SHORTCUTS_PATH)
}

win32 {
	DEFINES += SCREENSAVER_OFF
	DEFINES += AVOID_SCREENSAVER
	DEFINES += USE_FONTCONFIG_OPTIONS

	contains( DEFINES, SCREENSAVER_OFF ) {
		HEADERS += screensaver.h
		SOURCES += screensaver.cpp
	}

	!contains( DEFINES, PORTABLE_APP ) {
		DEFINES += USE_ASSOCIATIONS
	}
	
	contains( DEFINES, USE_ASSOCIATIONS ) {
		HEADERS += pref/associations.h winfileassoc.h
		SOURCES += pref/associations.cpp winfileassoc.cpp
		FORMS += pref/associations.ui
	}

	contains(TEMPLATE,vcapp) {
		LIBS += ole32.lib user32.lib
	} else {
		LIBS += libole32
	}
	
	RC_FILE = smplayer.rc
    DEFINES -= OUTPUT_ON_CONSOLE
	#debug {
	#	CONFIG += console
	#}
}

os2 {
	DEFINES += SCREENSAVER_OFF
	INCLUDEPATH += .
	contains( DEFINES, SCREENSAVER_OFF ) {
		HEADERS += screensaver.h
		SOURCES += screensaver.cpp
	}
	RC_FILE = smplayer_os2.rc
}


TRANSLATIONS = translations/smplayer_es.ts translations/smplayer_de.ts \
               translations/smplayer_sk.ts translations/smplayer_it.ts \
               translations/smplayer_fr.ts translations/smplayer_zh_CN.ts \
               translations/smplayer_ru_RU.ts translations/smplayer_hu.ts \
               translations/smplayer_en_US.ts translations/smplayer_pl.ts \
               translations/smplayer_ja.ts translations/smplayer_nl.ts \
               translations/smplayer_uk_UA.ts translations/smplayer_pt_BR.ts \
               translations/smplayer_ka.ts translations/smplayer_cs.ts \
               translations/smplayer_bg.ts translations/smplayer_tr.ts \
               translations/smplayer_sv.ts translations/smplayer_sr.ts \
               translations/smplayer_zh_TW.ts translations/smplayer_ro_RO.ts \
               translations/smplayer_pt.ts translations/smplayer_el_GR.ts \
               translations/smplayer_fi.ts translations/smplayer_ko.ts \
               translations/smplayer_mk.ts translations/smplayer_eu.ts \
               translations/smplayer_ca.ts translations/smplayer_sl_SI.ts \
               translations/smplayer_ar_SY.ts translations/smplayer_ku.ts \
               translations/smplayer_gl.ts translations/smplayer_vi_VN.ts \
               translations/smplayer_et.ts translations/smplayer_lt.ts \
               translations/smplayer_da.ts translations/smplayer_hr.ts \
               translations/smplayer_he_IL.ts translations/smplayer_th.ts \
               translations/smplayer_ms_MY.ts translations/smplayer_uz.ts \
               translations/smplayer_nn_NO.ts translations/smplayer_id.ts \
               translations/smplayer_ar.ts translations/smplayer_en_GB.ts \
               translations/smplayer_sq_AL.ts
