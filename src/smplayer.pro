TEMPLATE = app
LANGUAGE = C++

CONFIG += qt warn_on
CONFIG += release
#CONFIG += debug

QT += network xml

RESOURCES = icons.qrc

DEFINES += SINGLE_INSTANCE
DEFINES += FIND_SUBTITLES
DEFINES += VIDEOPREVIEW
DEFINES += YOUTUBE_SUPPORT
DEFINES += OUTPUT_ON_CONSOLE
DEFINES += SKINS
DEFINES += MPRIS2
DEFINES += UPDATE_CHECKER
DEFINES += CHECK_UPGRADED
DEFINES += AUTO_SHUTDOWN_PC
DEFINES += CAPTURE_STREAM

DEFINES += MPV_SUPPORT
DEFINES += MPLAYER_SUPPORT

#DEFINES += SIMPLE_BUILD

contains(DEFINES, SIMPLE_BUILD) {
	DEFINES -= SINGLE_INSTANCE
	DEFINES -= FIND_SUBTITLES
	DEFINES -= VIDEOPREVIEW
	DEFINES -= SKINS
	DEFINES -= MPRIS2
	DEFINES -= UPDATE_CHECKER
	DEFINES -= CHECK_UPGRADED
	DEFINES -= AUTO_SHUTDOWN_PC
}

isEqual(QT_MAJOR_VERSION, 5) {
	QT += widgets gui
	#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x040000
	win32 {
		DEFINES -= MPRIS2
	}
}

contains(QT_VERSION, ^4\\.[0-3]\\..*) {
    error("Qt > 4.3 required")
}

HEADERS += config.h \
	svn_revision.h \
	version.h \
	helper.h \
	colorutils.h \
	subtracks.h \
	discname.h \
	extensions.h \
    desktop.h \
	proc/process.h \
	proc/playerprocess.h \
    playerwindow.h \
	mediadata.h \
    settings/paths.h \
    settings/assstyles.h \
    settings/mediasettings.h \
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
    settings/cleanconfig.h \
    images.h \
	inforeader.h \
	core.h \
    lineedit_with_icon.h \
    filedialog.h \
    filechooser.h \
	languages.h \
    gui/links.h \
    gui/action/action.h \
    gui/action/actiongroup.h \
    gui/action/slider.h \
    gui/action/timeslider.h \
    gui/action/widgetactions.h \
    gui/action/shortcutgetter.h \
    gui/action/actionseditor.h \
    gui/action/toolbareditor.h \
    gui/action/sizegrip.h \
    gui/action/editabletoolbar.h \
    gui/action/menu.h \
    gui/action/openmenu.h \
    gui/action/playmenu.h \
    gui/action/videofiltermenu.h \
    gui/action/videosizemenu.h \
    gui/action/videomenu.h \
    gui/action/audiomenu.h \
    gui/action/subtitlemenu.h \
    gui/action/browsemenu.h \
    gui/action/optionsmenu.h \
    gui/deviceinfo.h \
    gui/pref/seekwidget.h \
    gui/pref/vdpauproperties.h \
    gui/pref/selectcolorbutton.h \
    gui/pref/tristatecombo.h \
    gui/pref/combobox.h \
    gui/pref/dialog.h \
    gui/pref/widget.h \
    gui/pref/general.h \
    gui/pref/drives.h \
    gui/pref/interface.h \
    gui/pref/performance.h \
    gui/pref/input.h \
    gui/pref/subtitles.h \
    gui/pref/advanced.h \
    gui/pref/prefplaylist.h \
    gui/pref/tv.h \
    gui/pref/updates.h \
    gui/pref/network.h \
    gui/multilineinputdialog.h \
    gui/infofile.h \
    gui/filepropertiesdialog.h \
    gui/inputdvddirectory.h \
    gui/stereo3ddialog.h \
    gui/verticaltext.h \
    gui/favoriteeditor.h \
    gui/favorites.h \
    gui/tvlist.h \
    gui/eqslider.h \
    gui/videoequalizer.h \
    gui/audioequalizer.h \
    gui/errordialog.h \
    gui/about.h \
    gui/timedialog.h \
    gui/lineedit.h \
    gui/inputurl.h \
	gui/tablewidget.h \
    gui/infoprovider.h \
    gui/playlist.h \
    gui/autohidetimer.h \
    gui/base.h \
	gui/baseplus.h \
    gui/default.h \
    gui/logwindow.h \
    maps/map.h \
    maps/tracks.h \
    maps/titletracks.h \
    maps/chapters.h \
    clhelp.h \
    log.h \
    smplayer.h


SOURCES	+= version.cpp \
	helper.cpp \
	colorutils.cpp \
	subtracks.cpp \
	discname.cpp \
	extensions.cpp \
    desktop.cpp \
	proc/process.cpp \
	proc/playerprocess.cpp \
    playerwindow.cpp \
	mediadata.cpp \
    settings/paths.cpp \
    settings/assstyles.cpp \
    settings/mediasettings.cpp \
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
    settings/cleanconfig.cpp \
    images.cpp \
	inforeader.cpp \
	core.cpp \
    lineedit_with_icon.cpp \
    filedialog.cpp \
    filechooser.cpp \
	languages.cpp \
    gui/action/action.cpp \
    gui/action/actiongroup.cpp \
    gui/action/slider.cpp \
    gui/action/timeslider.cpp \
    gui/action/widgetactions.cpp \
    gui/action/shortcutgetter.cpp \
    gui/action/actionseditor.cpp \
    gui/action/toolbareditor.cpp \
    gui/action/sizegrip.cpp \
    gui/action/editabletoolbar.cpp \
    gui/action/menu.cpp \
    gui/action/openmenu.cpp \
    gui/action/playmenu.cpp \
    gui/action/videofiltermenu.cpp \
    gui/action/videosizemenu.cpp \
    gui/action/videomenu.cpp \
    gui/action/audiomenu.cpp \
    gui/action/subtitlemenu.cpp \
    gui/action/browsemenu.cpp \
    gui/action/optionsmenu.cpp \
    gui/deviceinfo.cpp \
    gui/pref/seekwidget.cpp \
    gui/pref/vdpauproperties.cpp \
    gui/pref/selectcolorbutton.cpp \
    gui/pref/tristatecombo.cpp \
    gui/pref/combobox.cpp \
    gui/pref/dialog.cpp \
    gui/pref/widget.cpp \
    gui/pref/general.cpp \
    gui/pref/drives.cpp \
    gui/pref/interface.cpp \
    gui/pref/performance.cpp \
    gui/pref/input.cpp \
    gui/pref/subtitles.cpp \
    gui/pref/advanced.cpp \
    gui/pref/prefplaylist.cpp \
    gui/pref/tv.cpp \
    gui/pref/updates.cpp \
    gui/pref/network.cpp \
    gui/multilineinputdialog.cpp \
    gui/infofile.cpp \
    gui/filepropertiesdialog.cpp \
    gui/inputdvddirectory.cpp \
    gui/stereo3ddialog.cpp \
    gui/verticaltext.cpp \
    gui/favoriteeditor.cpp \
    gui/favorites.cpp \
    gui/tvlist.cpp \
    gui/eqslider.cpp \
    gui/videoequalizer.cpp \
    gui/audioequalizer.cpp \
    gui/errordialog.cpp \
    gui/about.cpp \
    gui/timedialog.cpp \
    gui/lineedit.cpp \
    gui/inputurl.cpp \
   	gui/tablewidget.cpp \
    gui/infoprovider.cpp \
    gui/playlist.cpp \
    gui/autohidetimer.cpp \
    gui/base.cpp \
	gui/baseplus.cpp \
    gui/default.cpp \
    gui/logwindow.cpp \
    maps/map.cpp \
    maps/tracks.cpp \
    maps/titletracks.cpp \
    maps/chapters.cpp \
    clhelp.cpp \
    smplayer.cpp \
    log.cpp \
    main.cpp

FORMS = gui/inputdvddirectory.ui gui/logwindow.ui gui/filepropertiesdialog.ui \
        gui/eqslider.ui gui/inputurl.ui gui/videoequalizer.ui \
        gui/favoriteeditor.ui gui/about.ui gui/errordialog.ui gui/timedialog.ui \
        gui/stereo3ddialog.ui gui/multilineinputdialog.ui \
        gui/action/toolbareditor.ui \
        gui/pref/seekwidget.ui gui/pref/vdpauproperties.ui gui/pref/dialog.ui \
        gui/pref/general.ui gui/pref/drives.ui gui/pref/interface.ui \
        gui/pref/performance.ui gui/pref/input.ui gui/pref/subtitles.ui \
        gui/pref/advanced.ui gui/pref/prefplaylist.ui gui/pref/tv.ui \
        gui/pref/updates.ui gui/pref/network.ui \

contains(DEFINES, MPV_SUPPORT) {
    HEADERS += proc/mpvprocess.h inforeadermpv.h
    SOURCES += proc/mpvprocess.cpp inforeadermpv.cpp
}

contains(DEFINES, MPLAYER_SUPPORT) {
    HEADERS += proc/mplayerprocess.h inforeadermplayer.h
    SOURCES += proc/mplayerprocess.cpp inforeadermplayer.cpp
}

# qtsingleapplication
contains(DEFINES, SINGLE_INSTANCE) {
	INCLUDEPATH += qtsingleapplication
	DEPENDPATH += qtsingleapplication

	SOURCES += qtsingleapplication/qtsingleapplication.cpp qtsingleapplication/qtlocalpeer.cpp
	HEADERS += qtsingleapplication/qtsingleapplication.h qtsingleapplication/qtlocalpeer.h
}

# Find subtitles dialog
contains(DEFINES, FIND_SUBTITLES) {
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
contains(DEFINES, DOWNLOAD_SUBS) {
	INCLUDEPATH += findsubtitles/filedownloader
	DEPENDPATH += findsubtitles/filedownloader

	HEADERS += findsubtitles/filedownloader/filedownloader.h findsubtitles/subchooserdialog.h findsubtitles/fixsubs.h
	SOURCES += findsubtitles/filedownloader/filedownloader.cpp findsubtitles/subchooserdialog.cpp findsubtitles/fixsubs.cpp

	FORMS += findsubtitles/subchooserdialog.ui

	contains(DEFINES, USE_QUAZIP) {
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
contains(DEFINES, YOUTUBE_SUPPORT) {
	DEFINES += YT_USE_SCRIPT
	INCLUDEPATH += youtube
	DEPENDPATH += youtube

	HEADERS += youtube/retrieveyoutubeurl.h youtube/loadpage.h
	SOURCES += youtube/retrieveyoutubeurl.cpp youtube/loadpage.cpp

	contains(DEFINES, YT_USE_SCRIPT) {
		DEFINES += YT_USE_SIG
		DEFINES += YT_USE_YTSIG
		QT += script
	}

	contains(DEFINES, YT_USE_SIG) {
		HEADERS += youtube/sig.h
		SOURCES += youtube/sig.cpp
	}

	contains(DEFINES, YT_USE_YTSIG) {
		HEADERS += youtube/ytsig.h youtube/codedownloader.h
		SOURCES += youtube/ytsig.cpp youtube/codedownloader.cpp
	}
}

# Skins support
contains(DEFINES, SKINS) {
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

contains(DEFINES, MPRIS2) {
	INCLUDEPATH += mpris2
	DEPENDPATH += mpris2

	HEADERS += mpris2/mediaplayer2.h mpris2/mediaplayer2player.h mpris2/mpris2.h
	SOURCES += mpris2/mediaplayer2.cpp mpris2/mediaplayer2player.cpp mpris2/mpris2.cpp

	QT += dbus
}

# Update checker
contains(DEFINES, UPDATE_CHECKER) {
    HEADERS += gui/updatechecker.h updatecheckerdata.h
    SOURCES += gui/updatechecker.cpp updatecheckerdata.cpp
}

# Videopreview
contains(DEFINES, VIDEOPREVIEW) {
	INCLUDEPATH += videopreview
	DEPENDPATH += videopreview

	HEADERS += videopreview/videopreview.h videopreview/videopreviewconfigdialog.h
	SOURCES += videopreview/videopreview.cpp videopreview/videopreviewconfigdialog.cpp

	FORMS += videopreview/videopreviewconfigdialog.ui
}

contains(DEFINES, AUTO_SHUTDOWN_PC) {
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

	contains(DEFINES, SCREENSAVER_OFF) {
		HEADERS += screensaver.h
		SOURCES += screensaver.cpp
	}

	!contains(DEFINES, PORTABLE_APP) {
		DEFINES += USE_ASSOCIATIONS
	}
	
	contains(DEFINES, USE_ASSOCIATIONS) {
        HEADERS += gui/pref/associations.h winfileassoc.h
        SOURCES += gui/pref/associations.cpp winfileassoc.cpp
        FORMS += gui/pref/associations.ui
	}

	contains(TEMPLATE,vcapp) {
		LIBS += ole32.lib user32.lib
	} else {
		LIBS += libole32
	}
	
	RC_FILE = smplayer.rc
    DEFINES -= OUTPUT_ON_CONSOLE
}

os2 {
	DEFINES += SCREENSAVER_OFF
	INCLUDEPATH += .
	contains(DEFINES, SCREENSAVER_OFF) {
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
