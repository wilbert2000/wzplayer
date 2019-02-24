TEMPLATE = app
LANGUAGE = C++

# Require Qt 5.6
lessThan(QT_VERSION, 0x050600) {
    error("Qt 5.6 or later required")
}

CONFIG += qt warn_on

# Default to release build
!CONFIG(debug, debug|release) {
!CONFIG(release, debug|release) {
    CONFIG += release
}
}

# Intermediates
UI_DIR = .ui
MOC_DIR = .moc
OBJECTS_DIR = .obj

# Log4qt
include(log4qt/log4qt.pri)

QT += network
QT += widgets gui

RESOURCES = icons.qrc

DEFINES += WZPLAYER_VERSION_STR=\\\"$$system(git describe --dirty --always --tags)\\\"

# Support for program switch in TS files
#DEFINES += PROGRAM_SWITCH


HEADERS += wzdebug.h \
    version.h \
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
    settings/preferences.h \
    settings/filesettingsbase.h \
    settings/filesettings.h \
    settings/filesettingshash.h \
    settings/tvsettings.h \
    settings/lrulist.h \
    settings/recents.h \
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
    gui/action/actionitem.h \
    gui/action/toolbareditor.h \
    gui/action/editabletoolbar.h \
    gui/action/menu/menuexec.h \
    gui/action/menu/menu.h \
    gui/action/menu/menufile.h \
    gui/action/menu/menuplay.h \
    gui/action/menu/menuinoutpoints.h \
    gui/action/menu/menuaspect.h \
    gui/action/menu/menuvideofilter.h \
    gui/action/menu/menuvideosize.h \
    gui/action/menu/menuvideotracks.h \
    gui/action/menu/menuvideocolorspace.h \
    gui/action/menu/menuvideo.h \
    gui/action/menu/menuaudiotracks.h \
    gui/action/menu/menuaudio.h \
    gui/action/menu/menusubtitle.h \
    gui/action/menu/menubrowse.h \
    gui/action/menu/menuview.h \
    gui/action/menu/menuhelp.h \
    gui/action/menu/favorite.h \
    gui/action/menu/favorites.h \
    gui/action/menu/favoriteeditor.h \
    gui/deviceinfo.h \
    gui/pref/languages.h \
    gui/pref/vdpauproperties.h \
    gui/pref/selectcolorbutton.h \
    gui/pref/tristatecombo.h \
    gui/pref/combobox.h \
    gui/pref/dialog.h \
    gui/pref/section.h \
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
    gui/msg.h \
    gui/multilineinputdialog.h \
    gui/infofile.h \
    gui/filepropertiesdialog.h \
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
    gui/playlist/addfilesthread.h \
    gui/playlist/playlistwidgetitem.h \
    gui/playlist/playlistwidget.h \
    gui/playlist/menuadd.h \
    gui/playlist/menuremove.h \
    gui/playlist/menucontext.h \
    gui/playlist/playlist.h \
    gui/autohidetimer.h \
    gui/playerwindow.h \
    gui/mainwindow.h \
    gui/mainwindowtray.h \
    gui/dockwidget.h \
    gui/logwindowappender.h \
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
    name.h \
    wzfiles.h \
    wztime.h \
    app.h


SOURCES += wzdebug.cpp \
    version.cpp \
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
    settings/preferences.cpp \
    settings/filesettingsbase.cpp \
    settings/filesettings.cpp \
    settings/filesettingshash.cpp \
    settings/tvsettings.cpp \
    settings/lrulist.cpp \
    settings/recents.cpp \
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
    gui/action/actionitem.cpp \
    gui/action/toolbareditor.cpp \
    gui/action/editabletoolbar.cpp \
    gui/action/menu/menuexec.cpp \
    gui/action/menu/menu.cpp \
    gui/action/menu/menufile.cpp \
    gui/action/menu/menuplay.cpp \
    gui/action/menu/menuinoutpoints.cpp \
    gui/action/menu/menuaspect.cpp \
    gui/action/menu/menuvideofilter.cpp \
    gui/action/menu/menuvideosize.cpp \
    gui/action/menu/menuvideotracks.cpp \
    gui/action/menu/menuvideocolorspace.cpp \
    gui/action/menu/menuvideo.cpp \
    gui/action/menu/menuaudiotracks.cpp \
    gui/action/menu/menuaudio.cpp \
    gui/action/menu/menusubtitle.cpp \
    gui/action/menu/menubrowse.cpp \
    gui/action/menu/menuview.cpp \
    gui/action/menu/menuhelp.cpp \
    gui/action/menu/favorite.cpp \
    gui/action/menu/favorites.cpp \
    gui/action/menu/favoriteeditor.cpp \
    gui/deviceinfo.cpp \
    gui/pref/languages.cpp \
    gui/pref/vdpauproperties.cpp \
    gui/pref/selectcolorbutton.cpp \
    gui/pref/tristatecombo.cpp \
    gui/pref/combobox.cpp \
    gui/pref/dialog.cpp \
    gui/pref/section.cpp \
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
    gui/msg.cpp \
    gui/multilineinputdialog.cpp \
    gui/infofile.cpp \
    gui/filepropertiesdialog.cpp \
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
    gui/playlist/addfilesthread.cpp \
    gui/playlist/playlistwidgetitem.cpp \
    gui/playlist/playlistwidget.cpp \
    gui/playlist/menuadd.cpp \
    gui/playlist/menuremove.cpp \
    gui/playlist/menucontext.cpp \
    gui/playlist/playlist.cpp \
    gui/autohidetimer.cpp \
    gui/playerwindow.cpp \
    gui/mainwindow.cpp \
    gui/mainwindowtray.cpp \
    gui/dockwidget.cpp \
    gui/logwindowappender.cpp \
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
    name.cpp \
    wzfiles.cpp \
    wztime.cpp \
    app.cpp \
    main.cpp

FORMS = gui/logwindow.ui \
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
    gui/action/menu/favoriteeditor.ui \
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
    gui/pref/network.ui


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
