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


HEADERS += gui/action/menu/menu.h \
    gui/action/menu/menuaudio.h \
    gui/action/menu/menubrowse.h \
    gui/action/menu/menufile.h \
    gui/action/menu/menuhelp.h \
    gui/action/menu/menuplay.h \
    gui/action/menu/menusubtitle.h \
    gui/action/menu/menuvideo.h \
    gui/action/menu/menuvideocolorspace.h \
    gui/action/menu/menuvideofilter.h \
    gui/action/menu/menuview.h \
    gui/action/menu/menuwindowsize.h \
    gui/action/action.h \
    gui/action/actiongroup.h \
    gui/action/actionitem.h \
    gui/action/actionseditor.h \
    gui/action/editabletoolbar.h \
    gui/action/shortcutgetter.h \
    gui/action/slider.h \
    gui/action/timeslider.h \
    gui/action/timeslideraction.h \
    gui/action/toolbareditor.h \
    gui/action/widgetactions.h \
    gui/playlist/addfilesthread.h \
    gui/playlist/favlist.h \
    gui/playlist/playlist.h \
    gui/playlist/playlistitem.h \
    gui/playlist/playlistwidget.h \
    gui/playlist/plist.h \
    gui/pref/audio.h \
    gui/pref/capture.h \
    gui/pref/combobox.h \
    gui/pref/demuxer.h \
    gui/pref/dialog.h \
    gui/pref/drives.h \
    gui/pref/input.h \
    gui/pref/interface.h \
    gui/pref/languages.h \
    gui/pref/network.h \
    gui/pref/performance.h \
    gui/pref/playersection.h \
    gui/pref/playlistsection.h \
    gui/pref/section.h \
    gui/pref/selectcolorbutton.h \
    gui/pref/subtitles.h \
    gui/pref/tristatecombo.h \
    gui/pref/vdpauproperties.h \
    gui/pref/video.h \
    gui/about.h \
    gui/audioequalizer.h \
    gui/autohidetimer.h \
    gui/deviceinfo.h \
    gui/dockwidget.h \
    gui/eqslider.h \
    gui/filechooser.h \
    gui/filedialog.h \
    gui/helpwindow.h \
    gui/inputurl.h \
    gui/lineedit.h \
    gui/lineedit_with_icon.h \
    gui/logwindow.h \
    gui/logwindowappender.h \
    gui/mainwindow.h \
    gui/mainwindowtray.h \
    gui/msg.h \
    gui/multilineinputdialog.h \
    gui/playerwindow.h \
    gui/propertiesdialog.h \
    gui/stereo3ddialog.h \
    gui/timedialog.h \
    gui/updatechecker.h \
    gui/verticaltext.h \
    gui/videoequalizer.h \
    maps/chapters.h \
    maps/map.h \
    maps/titletracks.h \
    maps/tracks.h \
    player/info/playerinfo.h \
    player/info/playerinfomplayer.h \
    player/info/playerinfompv.h \
    player/process/exitmsg.h \
    player/process/mplayerprocess.h \
    player/process/mpvprocess.h \
    player/process/playerprocess.h \
    player/process/process.h \
    player/player.h \
    player/state.h \
    qtfilecopier/qtcopydialog.h \
    qtfilecopier/qtfilecopier.h \
    qtsingleapplication/qtlocalpeer.h \
    qtsingleapplication/qtsingleapplication.h \
    settings/aspectratio.h \
    settings/assstyles.h \
    settings/cleanconfig.h \
    settings/filesettings.h \
    settings/filesettingsbase.h \
    settings/filesettingshash.h \
    settings/filters.h \
    settings/lrulist.h \
    settings/mediasettings.h \
    settings/paths.h \
    settings/preferences.h \
    settings/recents.h \
    settings/tvsettings.h \
    settings/updatecheckerdata.h \
    clhelp.h \
    colorutils.h \
    config.h \
    desktop.h \
    discname.h \
    extensions.h \
    iconprovider.h \
    images.h \
    mediadata.h \
    name.h \
    subtracks.h \
    version.h \
    wzdebug.h \
    wzfiles.h \
    wztime.h \
    wztimer.h \
    app.h


SOURCES += gui/action/menu/menu.cpp \
    gui/action/menu/menuaudio.cpp \
    gui/action/menu/menubrowse.cpp \
    gui/action/menu/menufile.cpp \
    gui/action/menu/menuhelp.cpp \
    gui/action/menu/menuplay.cpp \
    gui/action/menu/menusubtitle.cpp \
    gui/action/menu/menuvideo.cpp \
    gui/action/menu/menuvideocolorspace.cpp \
    gui/action/menu/menuvideofilter.cpp \
    gui/action/menu/menuview.cpp \
    gui/action/menu/menuwindowsize.cpp \
    gui/action/action.cpp \
    gui/action/actiongroup.cpp \
    gui/action/actionitem.cpp \
    gui/action/actionseditor.cpp \
    gui/action/editabletoolbar.cpp \
    gui/action/shortcutgetter.cpp \
    gui/action/slider.cpp \
    gui/action/timeslider.cpp \
    gui/action/timeslideraction.cpp \
    gui/action/toolbareditor.cpp \
    gui/action/widgetactions.cpp \
    gui/playlist/addfilesthread.cpp \
    gui/playlist/favlist.cpp \
    gui/playlist/playlist.cpp \
    gui/playlist/playlistitem.cpp \
    gui/playlist/playlistwidget.cpp \
    gui/playlist/plist.cpp \
    gui/pref/audio.cpp \
    gui/pref/capture.cpp \
    gui/pref/combobox.cpp \
    gui/pref/demuxer.cpp \
    gui/pref/dialog.cpp \
    gui/pref/drives.cpp \
    gui/pref/input.cpp \
    gui/pref/interface.cpp \
    gui/pref/languages.cpp \
    gui/pref/network.cpp \
    gui/pref/performance.cpp \
    gui/pref/playersection.cpp \
    gui/pref/playlistsection.cpp \
    gui/pref/section.cpp \
    gui/pref/selectcolorbutton.cpp \
    gui/pref/subtitles.cpp \
    gui/pref/tristatecombo.cpp \
    gui/pref/vdpauproperties.cpp \
    gui/pref/video.cpp \
    gui/about.cpp \
    gui/audioequalizer.cpp \
    gui/autohidetimer.cpp \
    gui/deviceinfo.cpp \
    gui/dockwidget.cpp \
    gui/eqslider.cpp \
    gui/filechooser.cpp \
    gui/filedialog.cpp \
    gui/helpwindow.cpp \
    gui/inputurl.cpp \
    gui/lineedit.cpp \
    gui/lineedit_with_icon.cpp \
    gui/logwindow.cpp \
    gui/logwindowappender.cpp \
    gui/mainwindow.cpp \
    gui/mainwindowtray.cpp \
    gui/msg.cpp \
    gui/multilineinputdialog.cpp \
    gui/playerwindow.cpp \
    gui/propertiesdialog.cpp \
    gui/stereo3ddialog.cpp \
    gui/timedialog.cpp \
    gui/updatechecker.cpp \
    gui/verticaltext.cpp \
    gui/videoequalizer.cpp \
    maps/chapters.cpp \
    maps/map.cpp \
    maps/titletracks.cpp \
    maps/tracks.cpp \
    player/info/playerinfo.cpp \
    player/info/playerinfomplayer.cpp \
    player/info/playerinfompv.cpp \
    player/process/exitmsg.cpp \
    player/process/mplayerprocess.cpp \
    player/process/mpvprocess.cpp \
    player/process/playerprocess.cpp \
    player/process/process.cpp \
    player/player.cpp \
    qtfilecopier/qtcopydialog.cpp \
    qtfilecopier/qtfilecopier.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    settings/aspectratio.cpp \
    settings/assstyles.cpp \
    settings/cleanconfig.cpp \
    settings/filesettings.cpp \
    settings/filesettingsbase.cpp \
    settings/filesettingshash.cpp \
    settings/filters.cpp \
    settings/lrulist.cpp \
    settings/mediasettings.cpp \
    settings/paths.cpp \
    settings/preferences.cpp \
    settings/recents.cpp \
    settings/tvsettings.cpp \
    settings/updatecheckerdata.cpp \
    clhelp.cpp \
    colorutils.cpp \
    config.cpp \
    desktop.cpp \
    discname.cpp \
    extensions.cpp \
    iconprovider.cpp \
    images.cpp \
    mediadata.cpp \
    name.cpp \
    subtracks.cpp \
    version.cpp \
    wzdebug.cpp \
    wzfiles.cpp \
    wztime.cpp \
    wztimer.cpp \
    app.cpp \
    main.cpp


FORMS = gui/action/toolbareditor.ui \
    gui/pref/audio.ui \
    gui/pref/capture.ui \
    gui/pref/demuxer.ui \
    gui/pref/dialog.ui \
    gui/pref/drives.ui \
    gui/pref/input.ui \
    gui/pref/interface.ui \
    gui/pref/network.ui \
    gui/pref/performance.ui \
    gui/pref/playersection.ui \
    gui/pref/playlistsection.ui \
    gui/pref/subtitles.ui \
    gui/pref/vdpauproperties.ui \
    gui/pref/video.ui \
    gui/about.ui \
    gui/eqslider.ui \
    gui/helpwindow.ui \
    gui/inputurl.ui \
    gui/logwindow.ui \
    gui/multilineinputdialog.ui \
    gui/propertiesdialog.ui \
    gui/stereo3ddialog.ui \
    gui/timedialog.ui \
    gui/videoequalizer.ui \
    qtfilecopier/qtcopydialog.ui \
    qtfilecopier/qtoverwritedialog.ui \
    qtfilecopier/qtotherdialog.ui



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
    ../setup/scripts/make_pkgs.cmd \
    ../man/wzplayer.1 \
    wzplayer.rc \
    ../setup/wzplayer.nsi \
    ../Makefile
