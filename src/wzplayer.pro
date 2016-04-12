TEMPLATE = app
LANGUAGE = C++

CONFIG += qt warn_on

!CONFIG(debug, debug|release) {
!CONFIG(release, debug|release) {
    CONFIG += release
}
}

QT += network xml

UI_DIR = .ui
MOC_DIR = .moc
OBJECTS_DIR = .obj

RESOURCES = icons.qrc

DEFINES += SINGLE_INSTANCE
DEFINES += FIND_SUBTITLES
DEFINES += MPRIS2
DEFINES += WZPLAYER_VERSION_STR=\\\"$$system(git describe --dirty --always --tags)\\\"

# Support for program switch in TS files
#DEFINES += PROGRAM_SWITCH
#DEFINES += SIMPLE_BUILD

contains(DEFINES, SIMPLE_BUILD) {
	DEFINES -= SINGLE_INSTANCE
	DEFINES -= FIND_SUBTITLES
	DEFINES -= MPRIS2
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

HEADERS += version.h \
	helper.h \
	colorutils.h \
	subtracks.h \
	discname.h \
	extensions.h \
    desktop.h \
    proc/errormsg.h \
    proc/process.h \
	proc/playerprocess.h \
    proc/mpvprocess.h \
    proc/mplayerprocess.h \
    playerwindow.h \
	mediadata.h \
    settings/paths.h \
    settings/assstyles.h \
    settings/aspectratio.h \
    settings/mediasettings.h \
    settings/filters.h \
    settings/playersettings.h \
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
    inforeadermpv.h \
    inforeadermplayer.h \
    corestate.h \
	core.h \
    lineedit_with_icon.h \
    filedialog.h \
    filechooser.h \
	languages.h \
    gui/action/action.h \
    gui/action/actiongroup.h \
    gui/action/actionlist.h \
    gui/action/slider.h \
    gui/action/timeslider.h \
    gui/action/widgetactions.h \
    gui/action/shortcutgetter.h \
    gui/action/actionseditor.h \
    gui/action/toolbareditor.h \
    gui/action/sizegrip.h \
    gui/action/editabletoolbar.h \
    gui/action/menu.h \
    gui/action/menuopen.h \
    gui/action/menuplay.h \
    gui/action/menuaspect.h \
    gui/action/menuvideofilter.h \
    gui/action/menuvideosize.h \
    gui/action/menuvideotracks.h \
    gui/action/menuvideo.h \
    gui/action/menuaudiotracks.h \
    gui/action/menuaudio.h \
    gui/action/menusubtitle.h \
    gui/action/menubrowse.h \
    gui/action/menuwindow.h \
    gui/action/menuhelp.h \
    gui/action/favoriteeditor.h \
    gui/action/favorites.h \
    gui/action/tvlist.h \
    gui/deviceinfo.h \
    gui/pref/seekwidget.h \
    gui/pref/vdpauproperties.h \
    gui/pref/selectcolorbutton.h \
    gui/pref/tristatecombo.h \
    gui/pref/combobox.h \
    gui/pref/dialog.h \
    gui/pref/widget.h \
    gui/pref/general.h \
    gui/pref/demuxer.h \
    gui/pref/video.h \
    gui/pref/audio.h \
    gui/pref/subtitles.h \
    gui/pref/interface.h \
    gui/pref/input.h \
    gui/pref/drives.h \
    gui/pref/capture.h \
    gui/pref/performance.h \
    gui/pref/network.h \
    gui/pref/updates.h \
    gui/pref/advanced.h \
    gui/multilineinputdialog.h \
    gui/infofile.h \
    gui/filepropertiesdialog.h \
    gui/inputdvddirectory.h \
    gui/stereo3ddialog.h \
    gui/verticaltext.h \
    gui/eqslider.h \
    gui/videoequalizer.h \
    gui/audioequalizer.h \
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
    gui/updatechecker.h \
    updatecheckerdata.h \
    maps/map.h \
    maps/tracks.h \
    maps/titletracks.h \
    maps/chapters.h \
    clhelp.h \
    log.h \
    app.h \
    config.h


SOURCES	+= version.cpp \
	helper.cpp \
	colorutils.cpp \
	subtracks.cpp \
	discname.cpp \
	extensions.cpp \
    desktop.cpp \
    proc/errormsg.cpp \
    proc/process.cpp \
    proc/playerprocess.cpp \
    proc/mpvprocess.cpp \
    proc/mplayerprocess.cpp \
    playerwindow.cpp \
	mediadata.cpp \
    settings/paths.cpp \
    settings/assstyles.cpp \
    settings/aspectratio.cpp \
    settings/mediasettings.cpp \
    settings/filters.cpp \
    settings/playersettings.cpp \
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
    inforeadermpv.cpp \
    inforeadermplayer.cpp \
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
    gui/action/menuopen.cpp \
    gui/action/menuplay.cpp \
    gui/action/menuaspect.cpp \
    gui/action/menuvideofilter.cpp \
    gui/action/menuvideosize.cpp \
    gui/action/menuvideotracks.cpp \
    gui/action/menuvideo.cpp \
    gui/action/menuaudiotracks.cpp \
    gui/action/menuaudio.cpp \
    gui/action/menusubtitle.cpp \
    gui/action/menubrowse.cpp \
    gui/action/menuwindow.cpp \
    gui/action/menuhelp.cpp \
    gui/action/favoriteeditor.cpp \
    gui/action/favorites.cpp \
    gui/action/tvlist.cpp \
    gui/deviceinfo.cpp \
    gui/pref/seekwidget.cpp \
    gui/pref/vdpauproperties.cpp \
    gui/pref/selectcolorbutton.cpp \
    gui/pref/tristatecombo.cpp \
    gui/pref/combobox.cpp \
    gui/pref/dialog.cpp \
    gui/pref/widget.cpp \
    gui/pref/general.cpp \
    gui/pref/demuxer.cpp \
    gui/pref/video.cpp \
    gui/pref/audio.cpp \
    gui/pref/subtitles.cpp \
    gui/pref/interface.cpp \
    gui/pref/input.cpp \
    gui/pref/drives.cpp \
    gui/pref/capture.cpp \
    gui/pref/performance.cpp \
    gui/pref/network.cpp \
    gui/pref/updates.cpp \
    gui/pref/advanced.cpp \
    gui/multilineinputdialog.cpp \
    gui/infofile.cpp \
    gui/filepropertiesdialog.cpp \
    gui/inputdvddirectory.cpp \
    gui/stereo3ddialog.cpp \
    gui/verticaltext.cpp \
    gui/eqslider.cpp \
    gui/videoequalizer.cpp \
    gui/audioequalizer.cpp \
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
    gui/updatechecker.cpp \
    updatecheckerdata.cpp \
    maps/map.cpp \
    maps/tracks.cpp \
    maps/titletracks.cpp \
    maps/chapters.cpp \
    clhelp.cpp \
    app.cpp \
    log.cpp \
    main.cpp \
    config.cpp

FORMS = gui/inputdvddirectory.ui \
    gui/logwindow.ui \
    gui/filepropertiesdialog.ui \
    gui/eqslider.ui \
    gui/inputurl.ui \
    gui/videoequalizer.ui \
    gui/about.ui \
    gui/timedialog.ui \
    gui/stereo3ddialog.ui \
    gui/multilineinputdialog.ui \
    gui/action/toolbareditor.ui \
    gui/action/favoriteeditor.ui \
    gui/pref/seekwidget.ui \
    gui/pref/vdpauproperties.ui \
    gui/pref/dialog.ui \
    gui/pref/general.ui \
    gui/pref/demuxer.ui \
    gui/pref/video.ui \
    gui/pref/audio.ui \
    gui/pref/subtitles.ui \
    gui/pref/interface.ui \
    gui/pref/input.ui \
    gui/pref/drives.ui \
    gui/pref/capture.ui \
    gui/pref/performance.ui \
    gui/pref/network.ui \
    gui/pref/updates.ui \
    gui/pref/advanced.ui


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

contains(DEFINES, MPRIS2) {
	INCLUDEPATH += mpris2
	DEPENDPATH += mpris2

	HEADERS += mpris2/mediaplayer2.h mpris2/mediaplayer2player.h mpris2/mpris2.h
	SOURCES += mpris2/mediaplayer2.cpp mpris2/mediaplayer2player.cpp mpris2/mpris2.cpp

	QT += dbus
}

unix {
    DEFINES += DATA_PATH=$(DATA_PATH)
    DEFINES += DOC_PATH=$(DOC_PATH)
    DEFINES += TRANSLATION_PATH=$(TRANSLATION_PATH)
    DEFINES += THEMES_PATH=$(THEMES_PATH)
    DEFINES += SHORTCUTS_PATH=$(SHORTCUTS_PATH)
}

win32 {
    DEFINES += DISABLE_SCREENSAVER

    contains(DEFINES, DISABLE_SCREENSAVER) {
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
	
    DEFINES += USE_ADAPTER
    RC_FILE = wzplayer.rc
}

os2 {
    DEFINES += DISABLE_SCREENSAVER
	INCLUDEPATH += .
    contains(DEFINES, DISABLE_SCREENSAVER) {
		HEADERS += screensaver.h
		SOURCES += screensaver.cpp
	}
    RC_FILE = wzplayer_os2.rc
}


TRANSLATIONS = translations/es.ts translations/de.ts \
               translations/sk.ts translations/it.ts \
               translations/fr.ts translations/zh_CN.ts \
               translations/ru_RU.ts translations/hu.ts \
               translations/en_US.ts translations/pl.ts \
               translations/ja.ts translations/nl.ts \
               translations/uk_UA.ts translations/pt_BR.ts \
               translations/ka.ts translations/cs.ts \
               translations/bg.ts translations/tr.ts \
               translations/sv.ts translations/sr.ts \
               translations/zh_TW.ts translations/ro_RO.ts \
               translations/pt.ts translations/el_GR.ts \
               translations/fi.ts translations/ko.ts \
               translations/mk.ts translations/eu.ts \
               translations/ca.ts translations/sl_SI.ts \
               translations/ar_SY.ts translations/ku.ts \
               translations/gl.ts translations/vi_VN.ts \
               translations/et.ts translations/lt.ts \
               translations/da.ts translations/hr.ts \
               translations/he_IL.ts translations/th.ts \
               translations/ms_MY.ts translations/uz.ts \
               translations/nn_NO.ts translations/id.ts \
               translations/ar.ts translations/en_GB.ts \
               translations/sq_AL.ts

OTHER_FILES += \
    ../Readme.txt \
    ../wzplayer.spec \
    ../create_rpm.sh \
    ../Install.txt \
    ../create_deb.sh \
    ../setup/scripts/install_wzplayer.cmd \
    ../setup/scripts/make_pkgs.cmd \
    ../latest_versions.txt \
    ../man/wzplayer.1
