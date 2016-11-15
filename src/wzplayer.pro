TEMPLATE = app
LANGUAGE = C++

CONFIG += qt warn_on

lessThan(QT_MAJOR_VERSION, 5) {
lessThan(QT_MINOR_VERSION, 6) {
    error("Qt >= 4.6 required")
}
}

!CONFIG(debug, debug|release) {
!CONFIG(release, debug|release) {
    CONFIG += release
}
}

UI_DIR = .ui
MOC_DIR = .moc
OBJECTS_DIR = .obj

include(log4qt/log4qt.pri)

QT += network

RESOURCES = icons.qrc

DEFINES += WZPLAYER_VERSION_STR=\\\"$$system(git describe --dirty --always --tags)\\\"

# Support for program switch in TS files
#DEFINES += PROGRAM_SWITCH

isEqual(QT_MAJOR_VERSION, 5) {
    QT += widgets gui
}


HEADERS += wzdebug.h \
    version.h \
	helper.h \
	colorutils.h \
	subtracks.h \
	discname.h \
	extensions.h \
    player/process/exitmsg.h \
    player/process/process.h \
    player/process/playerprocess.h \
    player/process/mpvprocess.h \
    player/process/mplayerprocess.h \
    player/info/playerinfo.h \
    player/info/playerinfomplayer.h \
    player/info/playerinfompv.h \
    player/state.h \
    player/player.h \
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
    settings/filehash.h \
    settings/filesettingshash.h \
    settings/tvsettings.h \
    settings/recents.h \
    settings/urlhistory.h \
    settings/cleanconfig.h \
    settings/updatecheckerdata.h \
    iconprovider.h \
    images.h \
    gui/desktop.h \
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
    gui/action/menufile.h \
    gui/action/menuplay.h \
    gui/action/menuinoutpoints.h \
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
    gui/pref/languages.h \
    gui/pref/vdpauproperties.h \
    gui/pref/selectcolorbutton.h \
    gui/pref/tristatecombo.h \
    gui/pref/combobox.h \
    gui/pref/dialog.h \
    gui/pref/widget.h \
    gui/pref/playersection.h \
    gui/pref/demuxer.h \
    gui/pref/video.h \
    gui/pref/audio.h \
    gui/pref/subtitles.h \
    gui/pref/interface.h \
    gui/pref/playlistsection.h \
    gui/pref/input.h \
    gui/pref/drives.h \
    gui/pref/capture.h \
    gui/pref/performance.h \
    gui/pref/network.h \
    gui/pref/advanced.h \
    gui/msg.h \
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
    gui/lineedit_with_icon.h \
    gui/lineedit.h \
    gui/filechooser.h \
    gui/filedialog.h \
    gui/inputurl.h \
    gui/infoprovider.h \
    gui/playlist/addfilesthread.h \
    gui/playlist/playlistitem.h \
    gui/playlist/playlistwidgetitem.h \
    gui/playlist/playlistwidget.h \
    gui/playlist/playlist.h \
    gui/autohidetimer.h \
    gui/playerwindow.h \
    gui/mainwindow.h \
    gui/mainwindowplus.h \
    gui/logwindow.h \
    gui/helpwindow.h \
    gui/updatechecker.h \
    maps/map.h \
    maps/tracks.h \
    maps/titletracks.h \
    maps/chapters.h \
    clhelp.h \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/qtlocalpeer.h \
    config.h \
    app.h


SOURCES	+= wzdebug.cpp \
    version.cpp \
	helper.cpp \
	colorutils.cpp \
	subtracks.cpp \
	discname.cpp \
	extensions.cpp \
    player/process/exitmsg.cpp \
    player/process/process.cpp \
    player/process/playerprocess.cpp \
    player/process/mpvprocess.cpp \
    player/process/mplayerprocess.cpp \
    player/info/playerinfo.cpp \
    player/info/playerinfomplayer.cpp \
    player/info/playerinfompv.cpp \
    player/player.cpp \
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
    settings/filehash.cpp \
    settings/filesettingshash.cpp \
    settings/tvsettings.cpp \
    settings/recents.cpp \
    settings/urlhistory.cpp \
    settings/cleanconfig.cpp \
    settings/updatecheckerdata.cpp \
    iconprovider.cpp \
    images.cpp \
    gui/desktop.cpp \
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
    gui/action/menufile.cpp \
    gui/action/menuplay.cpp \
    gui/action/menuinoutpoints.cpp \
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
    gui/pref/languages.cpp \
    gui/pref/vdpauproperties.cpp \
    gui/pref/selectcolorbutton.cpp \
    gui/pref/tristatecombo.cpp \
    gui/pref/combobox.cpp \
    gui/pref/dialog.cpp \
    gui/pref/widget.cpp \
    gui/pref/playersection.cpp \
    gui/pref/demuxer.cpp \
    gui/pref/video.cpp \
    gui/pref/audio.cpp \
    gui/pref/subtitles.cpp \
    gui/pref/interface.cpp \
    gui/pref/playlistsection.cpp \
    gui/pref/input.cpp \
    gui/pref/drives.cpp \
    gui/pref/capture.cpp \
    gui/pref/performance.cpp \
    gui/pref/network.cpp \
    gui/pref/advanced.cpp \
    gui/msg.cpp \
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
    gui/lineedit_with_icon.cpp \
    gui/lineedit.cpp \
    gui/filechooser.cpp \
    gui/filedialog.cpp \
    gui/inputurl.cpp \
    gui/infoprovider.cpp \
    gui/playlist/addfilesthread.cpp \
    gui/playlist/playlistitem.cpp \
    gui/playlist/playlistwidgetitem.cpp \
    gui/playlist/playlistwidget.cpp \
    gui/playlist/playlist.cpp \
    gui/autohidetimer.cpp \
    gui/playerwindow.cpp \
    gui/mainwindow.cpp \
    gui/mainwindowplus.cpp \
    gui/logwindow.cpp \
    gui/helpwindow.cpp \
    gui/updatechecker.cpp \
    maps/map.cpp \
    maps/tracks.cpp \
    maps/titletracks.cpp \
    maps/chapters.cpp \
    clhelp.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    config.cpp \
    app.cpp \
    main.cpp

FORMS = gui/inputdvddirectory.ui \
    gui/logwindow.ui \
    gui/helpwindow.ui \
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
    gui/pref/vdpauproperties.ui \
    gui/pref/dialog.ui \
    gui/pref/playersection.ui \
    gui/pref/demuxer.ui \
    gui/pref/video.ui \
    gui/pref/audio.ui \
    gui/pref/subtitles.ui \
    gui/pref/interface.ui \
    gui/pref/playlistsection.ui \
    gui/pref/input.ui \
    gui/pref/drives.ui \
    gui/pref/capture.ui \
    gui/pref/performance.ui \
    gui/pref/network.ui \
    gui/pref/advanced.ui


unix {
    DEFINES += DATA_PATH=$(DATA_PATH)
    DEFINES += DOC_PATH=$(DOC_PATH)
    DEFINES += TRANSLATION_PATH=$(TRANSLATION_PATH)
    DEFINES += THEMES_PATH=$(THEMES_PATH)
    DEFINES += SHORTCUTS_PATH=$(SHORTCUTS_PATH)
}

win32 {
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
	
    RC_FILE = wzplayer.rc
}

os2 {
    INCLUDEPATH += .
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
    ../create_deb.sh \
    ../create_rpm.sh \
    ../Install.txt \
    ../latest_versions.txt \
    ../Readme.txt \
    ../wzplayer.spec \
    ../setup/scripts/install_wzplayer.cmd \
    ../setup/scripts/make_pkgs.cmd \
    ../man/wzplayer.1 \
    wzplayer.rc \
    ../Watching_TV.txt \
    ../setup/wzplayer.nsi \
    ../Makefile
