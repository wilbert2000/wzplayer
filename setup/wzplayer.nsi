﻿;Installer script for win32/win64 WZPlayer
;Written by redxii (redxii@users.sourceforge.net)
;Tested/Developed with Unicode NSIS 2.46.5

!ifndef VER_MAJOR | VER_MINOR | VER_BUILD
  !error "Version information not defined (or incomplete). You must define: VER_MAJOR, VER_MINOR, VER_BUILD."
!endif

;--------------------------------
;Compressor

  SetCompressor /SOLID lzma
  SetCompressorDictSize 32

;--------------------------------
;Additional plugin folders

  !addplugindir .
  !addincludedir .

;--------------------------------
;Defines

!ifdef VER_REVISION
  !define WZPLAYER_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.${VER_REVISION}"
  !define WZPLAYER_PRODUCT_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.${VER_REVISION}"
!else ifndef VER_REVISION
  !define WZPLAYER_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.0"
  !define WZPLAYER_PRODUCT_VERSION "${VER_MAJOR}.${VER_MINOR}.${VER_BUILD}.0"
!endif

!ifdef WIN64
  !define WZPLAYER_BUILD_DIR "wzplayer-build64"
!else
  !define WZPLAYER_BUILD_DIR "wzplayer-build"
!endif

  !define WZPLAYER_REG_KEY "Software\WH\WZPlayer"
  !define WZPLAYER_APP_PATHS_KEY "Software\Microsoft\Windows\CurrentVersion\App Paths\wzplayer.exe"
  !define WZPLAYER_DEF_PROGS_KEY "Software\Clients\Media\WZPlayer"

  !define WZPLAYER_UNINST_EXE "uninst.exe"
  !define WZPLAYER_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\WZPlayer"

;--------------------------------
;General

  ;Name and file
  Name "WZPlayer ${WZPLAYER_VERSION}"
  BrandingText "WZPlayer for Windows v${WZPLAYER_VERSION}"
!ifdef WIN64
  OutFile "output\Qt5\wzplayer-${WZPLAYER_VERSION}-x64-qt5.exe"
!else
  OutFile "output\Qt5\wzplayer-${WZPLAYER_VERSION}-win32-qt5.exe"
!endif

  ;Version tab properties
  VIProductVersion "${WZPLAYER_PRODUCT_VERSION}"
  VIAddVersionKey "ProductName" "WZPlayer"
  VIAddVersionKey "ProductVersion" "${WZPLAYER_VERSION}"
  VIAddVersionKey "FileVersion" "${WZPLAYER_VERSION}"
  VIAddVersionKey "LegalCopyright" ""
!ifdef WIN64
  VIAddVersionKey "FileDescription" "WZPlayer Installer (64-bit)"
!else
  VIAddVersionKey "FileDescription" "WZPlayer Installer (32-bit)"
!endif

  ;Default installation folder
!ifdef WIN64
  InstallDir "$PROGRAMFILES64\WH\WZPlayer"
!else
  InstallDir "$PROGRAMFILES\WH\WZPlayer"
!endif

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "${WZPLAYER_REG_KEY}" "Path"

  ;Vista+ XML manifest, does not affect older OSes
  RequestExecutionLevel admin

  ShowInstDetails show
  ShowUnInstDetails show

;--------------------------------
;Variables

  Var Dialog_Reinstall
  Var Inst_Type
  Var Previous_Version
  Var Previous_Version_State
  Var Reinstall_ChgSettings
  Var Reinstall_ChgSettings_State
  Var Reinstall_Message
  Var Reinstall_OverwriteButton
  Var Reinstall_OverwriteButton_State
  Var Reinstall_RemoveSettings
  Var Reinstall_RemoveSettings_State
  Var Reinstall_Uninstall
  Var Reinstall_UninstallButton
  Var Reinstall_UninstallButton_State
!ifndef WIN64
  Var Restore_Codecs
!endif
  Var WZPlayer_Path
  Var WZPlayer_UnStrPath
  Var WZPlayer_StartMenuFolder

;--------------------------------
;Interface Settings

  ;Installer/Uninstaller icons
  !define MUI_ICON "wzplayer-orange-installer.ico"
  !define MUI_UNICON "wzplayer-orange-uninstaller.ico"

  ;Misc
  !define MUI_WELCOMEFINISHPAGE_BITMAP "wzplayer-orange-wizard.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "wzplayer-orange-wizard-un.bmp"
  !define MUI_ABORTWARNING

  ;Welcome page
  !define MUI_WELCOMEPAGE_TITLE $(WelcomePage_Title)
  !define MUI_WELCOMEPAGE_TEXT $(WelcomePage_Text)

  ;License page
  !define MUI_LICENSEPAGE_RADIOBUTTONS

  ;Components page
  !define MUI_COMPONENTSPAGE_SMALLDESC

  ;Finish page
  !define MUI_FINISHPAGE_LINK "https://github.com/wilbert2000/wzplayer"
  !define MUI_FINISHPAGE_LINK_LOCATION "https://github.com/wilbert2000/wzplayer"
  !define MUI_FINISHPAGE_NOREBOOTSUPPORT
  !define MUI_FINISHPAGE_RUN $INSTDIR\wzplayer.exe
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_FINISHPAGE_SHOWREADME $INSTDIR\Readme.txt
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED

  ;Language Selection Dialog Settings
  !define MUI_LANGDLL_REGISTRY_ROOT HKLM
  !define MUI_LANGDLL_REGISTRY_KEY "${WZPLAYER_UNINST_KEY}"
  !define MUI_LANGDLL_REGISTRY_VALUENAME "NSIS:Language"

  ;Memento Settings
  !define MEMENTO_REGISTRY_ROOT HKLM
  !define MEMENTO_REGISTRY_KEY "${WZPLAYER_REG_KEY}"

  ;Start Menu Settings
  !define MUI_STARTMENUPAGE_DEFAULTFOLDER "WZPlayer"
  !define MUI_STARTMENUPAGE_NODISABLE
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT HKLM
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "${WZPLAYER_UNINST_KEY}"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "NSIS:StartMenu"

;--------------------------------
;Include Modern UI and functions

  !include MUI2.nsh
  !include FileFunc.nsh
  !include Memento.nsh
  !include nsDialogs.nsh
  !include Sections.nsh
  !include WinVer.nsh
  !include WordFunc.nsh
  !include x64.nsh

;--------------------------------
;Pages

  ;Install pages
  #Welcome
  !insertmacro MUI_PAGE_WELCOME

  #License
  !insertmacro MUI_PAGE_LICENSE "license.txt"

  #Upgrade/Reinstall
  Page custom PageReinstall PageReinstallLeave

  #Components
  !define MUI_PAGE_CUSTOMFUNCTION_PRE PageComponentsPre
  !insertmacro MUI_PAGE_COMPONENTS

  #Install Directory
  !define MUI_PAGE_CUSTOMFUNCTION_PRE PageDirectoryPre
  !insertmacro MUI_PAGE_DIRECTORY

  #Start Menu
  !define MUI_PAGE_CUSTOMFUNCTION_PRE PageStartMenuPre
  !insertmacro MUI_PAGE_STARTMENU "WZP_SMenu" $WZPlayer_StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  ;Uninstall pages
  !define MUI_PAGE_CUSTOMFUNCTION_PRE un.ConfirmPagePre
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "Albanian"
  ;!insertmacro MUI_LANGUAGE "Amharic"
  !insertmacro MUI_LANGUAGE "Arabic"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Hungarian"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Vietnamese"

;Custom translations for setup

  !insertmacro LANGFILE_INCLUDE "translations\english.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\albanian.nsh"
  ;!insertmacro LANGFILE_INCLUDE "translations\amharic.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\arabic.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\basque.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\bulgarian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\catalan.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\croatian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\czech.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\danish.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\dutch.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\farsi.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\finnish.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\french.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\german.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\greek.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\hebrew.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\hungarian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\italian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\japanese.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\korean.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\malay.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\norwegian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\polish.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\portuguese.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\portuguesebrazil.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\romanian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\russian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\serbian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\simpchinese.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\slovak.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\slovenian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\spanish.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\thai.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\tradchinese.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\ukrainian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\galician.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\indonesian.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\turkish.nsh"
  !insertmacro LANGFILE_INCLUDE "translations\vietnamese.nsh"

;--------------------------------
;Reserve Files

  ;These files should be inserted before other files in the data block
  ;Keep these lines before any File command
  ;Only for solid compression (by default, solid compression is enabled for BZIP2 and LZMA)

  !insertmacro MUI_RESERVEFILE_LANGDLL
  ReserveFile "${NSISDIR}\Plugins\UserInfo.dll"

;--------------------------------
;Installer Sections

;--------------------------------
;Main WZPlayer files
Section $(Section_WZPlayer) SecWZPlayer

  SectionIn RO

  ${If} $Reinstall_Uninstall == 1

    ${If} $Reinstall_UninstallButton_State == 1
      Exec '"$WZPlayer_UnStrPath" /X'
      Quit
    ${ElseIf} $Reinstall_OverwriteButton_State == 1

!ifndef WIN64
      Call Backup_Codecs
!endif

      ${If} "$INSTDIR" == "$WZPlayer_Path"
        ExecWait '"$WZPlayer_UnStrPath" /S /R _?=$WZPlayer_Path'
      ${Else}
        ExecWait '"$WZPlayer_UnStrPath" /S /R'
      ${EndIf}

      Sleep 2500

    ${EndIf}

  ${EndIf}

  SetOutPath "$INSTDIR"
  File "${WZPLAYER_BUILD_DIR}\*"

  ;WZPlayer docs
  SetOutPath "$INSTDIR\docs"
  File /r "${WZPLAYER_BUILD_DIR}\docs\*.*"

  ;Qt imageformats
  SetOutPath "$INSTDIR\imageformats"
  File /r "${WZPLAYER_BUILD_DIR}\imageformats\*.*"

  ;Open fonts
  ; SetOutPath "$INSTDIR\open-fonts"
  ; File /r "${WZPLAYER_BUILD_DIR}\open-fonts\*.*"

  ;Qt platforms (Qt 5+)
  SetOutPath "$INSTDIR\platforms"
  File /r "${WZPLAYER_BUILD_DIR}\platforms\*.*"

  ;WZPlayer key shortcuts
  SetOutPath "$INSTDIR\shortcuts"
  File /r "${WZPLAYER_BUILD_DIR}\shortcuts\*.*"

SectionEnd

;--------------------------------
;Shortcuts
SectionGroup $(ShortcutGroupTitle)

  ${MementoSection} $(Section_DesktopShortcut) SecDesktopShortcut

    SetOutPath "$INSTDIR"
    CreateShortCut "$DESKTOP\WZPlayer.lnk" "$INSTDIR\wzplayer.exe"

  ${MementoSectionEnd}

  ${MementoSection} $(Section_StartMenu) SecStartMenuShortcut

    SetOutPath "$INSTDIR"
    !insertmacro MUI_STARTMENU_WRITE_BEGIN WZP_SMenu
      CreateDirectory "$SMPROGRAMS\$WZPlayer_StartMenuFolder"
      CreateShortCut "$SMPROGRAMS\$WZPlayer_StartMenuFolder\WZPlayer.lnk" "$INSTDIR\wzplayer.exe"
      WriteINIStr    "$SMPROGRAMS\$WZPlayer_StartMenuFolder\WZPlayer on the Web.url" "InternetShortcut" "URL" "http://www.wzplayer.info"
      CreateShortCut "$SMPROGRAMS\$WZPlayer_StartMenuFolder\Uninstall WZPlayer.lnk" "$INSTDIR\${WZPLAYER_UNINST_EXE}"
    !insertmacro MUI_STARTMENU_WRITE_END

  ${MementoSectionEnd}

SectionGroupEnd

;--------------------------------
;MPlayer & MPV
SectionGroup $(MPlayerMPVGroupTitle)

  ${MementoSection} "MPlayer" SecMPlayer

    SetOutPath "$INSTDIR\mplayer"
    File /r /x mplayer.exe /x mencoder.exe /x mplayer64.exe /x mencoder64.exe /x *.exe.debug /x gdb.exe /x gdb64.exe /x vfw2menc.exe /x buildinfo /x buildinfo64 /x buildinfo-mencoder-32 /x buildinfo-mencoder-debug-32 /x buildinfo-mplayer-32 /x buildinfo-mplayer-debug-32 /x buildinfo-mencoder-64 /x buildinfo-mencoder-debug-64 /x buildinfo-mplayer-64 /x buildinfo-mplayer-debug-64 "${WZPLAYER_BUILD_DIR}\mplayer\*.*"
!ifdef WIN64
    File /oname=mplayer.exe "${WZPLAYER_BUILD_DIR}\mplayer\mplayer64.exe"
    RMDir "$INSTDIR\mplayer\codecs"
!else
    File "${WZPLAYER_BUILD_DIR}\mplayer\mplayer.exe"
!endif

  ${MementoSectionEnd}

  ${MementoSection} "MPV" SecMPV

  SetOutPath "$INSTDIR\mpv"
!ifdef WIN64
  File /r /x mpv.exe /x mpv.com /x mpv64.exe /x mpv64.com /x fonts /x mpv "${WZPLAYER_BUILD_DIR}\mpv\*.*"
  File /oname=mpv.exe "${WZPLAYER_BUILD_DIR}\mpv\mpv64.exe"
  File /oname=mpv.com "${WZPLAYER_BUILD_DIR}\mpv\mpv64.com"
!else
  File /r /x mpv64.exe /x mpv64.com "${WZPLAYER_BUILD_DIR}\mpv\*.*"
!endif

  IfFileExists "$PLUGINSDIR\youtube-dl.exe" 0 YTDL
    CopyFiles /SILENT "$PLUGINSDIR\youtube-dl.exe" "$INSTDIR\mpv"

    DetailPrint $(YTDL_Update_Check)
    NsExec::ExecToLog '"$INSTDIR\mpv\youtube-dl.exe" -U'

    Goto skip_ytdl

  YTDL:
  NSISdl::download /TIMEOUT=30000 \
  "http://yt-dl.org/latest/youtube-dl.exe" \
  "$INSTDIR\mpv\youtube-dl.exe" /END
  Pop $R0
  StrCmp $R0 "success" +3 0
    DetailPrint $(YTDL_DL_Failed)
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION $(YTDL_DL_Retry) /SD IDCANCEL IDRETRY YTDL

  skip_ytdl:

  ${MementoSectionEnd}

SectionGroupEnd

;--------------------------------
;Icon themes
${MementoSection} $(Section_IconThemes) SecThemes

  SetOutPath "$INSTDIR\themes"
  File /r "${WZPLAYER_BUILD_DIR}\themes\*.*"

${MementoSectionEnd}

;--------------------------------
;Translations
${MementoSection} $(Section_Translations) SecTranslations

  SetOutPath "$INSTDIR\translations"
  File /r "${WZPLAYER_BUILD_DIR}\translations\*.*"

${MementoSectionEnd}

Section -RestorePrograms

!ifndef WIN64
  ${If} $Restore_Codecs == 1
    DetailPrint $(Info_Codecs_Restore)
    CopyFiles /SILENT "$PLUGINSDIR\codecbak\*" "$INSTDIR\mplayer\codecs"
  ${EndIf}
!endif

SectionEnd

;--------------------------------
;Install/Uninstall information
Section -Post

  ;Uninstall file
  WriteUninstaller "$INSTDIR\${WZPLAYER_UNINST_EXE}"

  ;Store installed path & version
  WriteRegStr HKLM "${WZPLAYER_REG_KEY}" "Path" "$INSTDIR"
  WriteRegStr HKLM "${WZPLAYER_REG_KEY}" "Version" "${WZPLAYER_VERSION}"

  ;Allows user to use 'start wzplayer.exe'
  WriteRegStr HKLM "${WZPLAYER_APP_PATHS_KEY}" "" "$INSTDIR\wzplayer.exe"
  WriteRegStr HKLM "${WZPLAYER_APP_PATHS_KEY}" "Path" "$INSTDIR"

  ;Default Programs Registration (Vista & later)
  ${If} ${AtLeastWinVista}
    Call RegisterDefaultPrograms
  ${EndIf}

  ;Registry Uninstall information
!ifdef WIN64
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "DisplayName" "$(^Name) (x64)"
!else
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "DisplayName" "$(^Name)"
!endif
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "DisplayIcon" "$INSTDIR\wzplayer.exe"
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "DisplayVersion" "${WZPLAYER_VERSION}"
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "HelpLink" "https://github.com/wilbert2000/wzplayer"
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "Publisher" "Wilbert Hengst"
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "UninstallString" "$INSTDIR\${WZPLAYER_UNINST_EXE}"
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "URLInfoAbout" "https://github.com/wilbert2000/wzplayer"
  WriteRegStr HKLM "${WZPLAYER_UNINST_KEY}" "URLUpdateInfo" "https://github.com/wilbert2000/wzplayer"
  WriteRegDWORD HKLM "${WZPLAYER_UNINST_KEY}" "NoModify" "1"
  WriteRegDWORD HKLM "${WZPLAYER_UNINST_KEY}" "NoRepair" "1"

  DetailPrint $(Info_Cleaning_Fontconfig)
  SetDetailsPrint none
  Delete "$LOCALAPPDATA\fontconfig\cache\CACHEDIR.TAG"
  Delete "$LOCALAPPDATA\fontconfig\cache\*.cache*"
  RMDir "$LOCALAPPDATA\fontconfig\cache"
  RMDir "$LOCALAPPDATA\fontconfig"
  SetDetailsPrint both

  ${If} $Reinstall_RemoveSettings_State == 1
    DetailPrint $(Info_Cleaning_WZPlayer)
    SetDetailsPrint none
    NsExec::Exec '"$INSTDIR\wzplayer.exe" -delete-config'
    SetDetailsPrint both
  ${EndIf}

  Sleep 2500

  ;SetAutoClose false

SectionEnd

${MementoSectionDone}

;--------------------------------
;Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecWZPlayer} $(Section_WZPlayer_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktopShortcut} $(Section_DesktopShortcut_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecStartMenuShortcut} $(Section_StartMenu_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMPlayer} $(Section_MPlayer_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecMPV} $(Section_MPV_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecThemes} $(Section_IconThemes_Desc)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecTranslations} $(Section_Translations_Desc)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Macros

!macro MacroAllExtensions _action
  !insertmacro ${_action} ".3gp"
  !insertmacro ${_action} ".aac"
  !insertmacro ${_action} ".ac3"
  !insertmacro ${_action} ".ape"
  !insertmacro ${_action} ".asf"
  !insertmacro ${_action} ".avi"
  !insertmacro ${_action} ".bik"
  !insertmacro ${_action} ".bin"
  !insertmacro ${_action} ".dat"
  !insertmacro ${_action} ".divx"
  !insertmacro ${_action} ".dts"
  !insertmacro ${_action} ".dv"
  !insertmacro ${_action} ".dvr-ms"
  !insertmacro ${_action} ".f4v"
  !insertmacro ${_action} ".flac"
  !insertmacro ${_action} ".flv"
  !insertmacro ${_action} ".hdmov"
  !insertmacro ${_action} ".iso"
  !insertmacro ${_action} ".m1v"
  !insertmacro ${_action} ".m2t"
  !insertmacro ${_action} ".m2ts"
  !insertmacro ${_action} ".mts"
  !insertmacro ${_action} ".m2v"
  !insertmacro ${_action} ".m3u"
  !insertmacro ${_action} ".m3u8"
  !insertmacro ${_action} ".m4a"
  !insertmacro ${_action} ".m4v"
  !insertmacro ${_action} ".mka"
  !insertmacro ${_action} ".mkv"
  !insertmacro ${_action} ".mov"
  !insertmacro ${_action} ".mp3"
  !insertmacro ${_action} ".mp4"
  !insertmacro ${_action} ".mpeg"
  !insertmacro ${_action} ".mpg"
  !insertmacro ${_action} ".mpv"
  !insertmacro ${_action} ".mqv"
  !insertmacro ${_action} ".nsv"
  !insertmacro ${_action} ".oga"
  !insertmacro ${_action} ".ogg"
  !insertmacro ${_action} ".ogm"
  !insertmacro ${_action} ".ogv"
  !insertmacro ${_action} ".ogx"
  !insertmacro ${_action} ".pls"
  !insertmacro ${_action} ".ra"
  !insertmacro ${_action} ".ram"
  !insertmacro ${_action} ".rec"
  !insertmacro ${_action} ".rm"
  !insertmacro ${_action} ".rmvb"
  !insertmacro ${_action} ".smk"
  !insertmacro ${_action} ".swf"
  !insertmacro ${_action} ".thd"
  !insertmacro ${_action} ".ts"
  !insertmacro ${_action} ".vcd"
  !insertmacro ${_action} ".vfw"
  !insertmacro ${_action} ".vob"
  !insertmacro ${_action} ".vp8"
  !insertmacro ${_action} ".wav"
  !insertmacro ${_action} ".webm"
  !insertmacro ${_action} ".wma"
  !insertmacro ${_action} ".wmv"
  !insertmacro ${_action} ".wtv"
!macroend

!macro WriteRegStrSupportedTypes EXT
  WriteRegStr HKLM  "${WZPLAYER_DEF_PROGS_KEY}\Capabilities\FileAssociations" ${EXT} "MPlayerFileVideo"
!macroend

!macro MacroRemoveWZPlayer
  ;Delete desktop and start menu shortcuts
  SetDetailsPrint textonly
  DetailPrint $(Info_Del_Shortcuts)
  SetDetailsPrint listonly

  SetShellVarContext all
  Delete "$DESKTOP\WZPlayer.lnk"
  Delete "$SMPROGRAMS\$WZPlayer_StartMenuFolder\WZPlayer.lnk"
  Delete "$SMPROGRAMS\$WZPlayer_StartMenuFolder\WZPlayer on the Web.url"
  Delete "$SMPROGRAMS\$WZPlayer_StartMenuFolder\Uninstall WZPlayer.lnk"
  RMDir "$SMPROGRAMS\$WZPlayer_StartMenuFolder"

  ;Delete directories recursively except for main directory
  ;Do not recursively delete $INSTDIR
  SetDetailsPrint textonly
  DetailPrint $(Info_Del_Files)
  SetDetailsPrint listonly

  RMDir /r "$INSTDIR\docs"
  RMDir /r "$INSTDIR\imageformats"
  RMDir /r "$INSTDIR\mplayer"
  RMDir /r "$INSTDIR\mpv"
  ; RMDir /r "$INSTDIR\open-fonts"
  RMDir /r "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\shortcuts"
  RMDir /r "$INSTDIR\themes"
  RMDir /r "$INSTDIR\translations"

  ;Txt
  Delete "$INSTDIR\Copying.txt"
  Delete "$INSTDIR\Copying_BSD.txt"
  Delete "$INSTDIR\Install.txt"
  Delete "$INSTDIR\Readme.txt"

  ;Binaries
  Delete "$INSTDIR\wzplayer.exe"
  Delete "$INSTDIR\icudt5*.dll"
  Delete "$INSTDIR\icuin5*.dll"
  Delete "$INSTDIR\icuuc5*.dll"
  Delete "$INSTDIR\libgcc_s_*.dll"
  Delete "$INSTDIR\libstdc++-6.dll"
  Delete "$INSTDIR\libwinpthread-1.dll"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\Qt*.dll"
  Delete "$INSTDIR\sample.avi"

  ;Delete registry keys
  SetDetailsPrint textonly
  DetailPrint $(Info_Del_Registry)
  SetDetailsPrint listonly

  DeleteRegKey HKLM "${WZPLAYER_REG_KEY}"
  DeleteRegKey HKLM "${WZPLAYER_APP_PATHS_KEY}"
  DeleteRegKey HKLM "${WZPLAYER_DEF_PROGS_KEY}"
  DeleteRegKey HKLM "${WZPLAYER_UNINST_KEY}"
  DeleteRegKey HKCR "MPlayerFileVideo"
  DeleteRegValue HKLM "Software\RegisteredApplications" "WZPlayer"

  SetDetailsPrint both
!macroend

;--------------------------------
;Shared functions

!ifdef USE_RUNCHECK
!macro RunCheckMacro UN
Function ${UN}RunCheck

    retry_runcheck:
    FindWindow $0 "QWidget" "WZPlayer"
    StrCmp $0 0 notRunning
      MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION $(WZPlayer_Is_Running) /SD IDCANCEL IDRETRY retry_runcheck
      Abort
    notrunning:

FunctionEnd
!macroend
!insertmacro RunCheckMacro ""
!insertmacro RunCheckMacro "un."
!endif

;--------------------------------
;Installer functions

Function .onInit

!ifdef WIN64
  ${Unless} ${AtLeastWinVista}
!else
  ${Unless} ${AtLeastWinXP}
!endif
    MessageBox MB_YESNO|MB_ICONSTOP $(OS_Not_Supported) /SD IDNO IDYES installonoldwindows
    Abort
  installonoldwindows:
  ${EndIf}

!ifdef WIN64
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP $(Win64_Required)
    Abort
  ${EndIf}

  SetRegView 32
  ClearErrors
  ReadRegStr $R0 HKLM "${WZPLAYER_UNINST_KEY}" "UninstallString"

  IfErrors +3 0
    MessageBox MB_OK|MB_ICONSTOP $(Existing_32bitInst)
    Abort

  SetRegView 64
!else
  ${If} ${RunningX64}
    SetRegView 64
    ClearErrors
    ReadRegStr $R0 HKLM "${WZPLAYER_UNINST_KEY}" "UninstallString"

    IfErrors +3 0
      MessageBox MB_OK|MB_ICONSTOP $(Existing_64bitInst)
      Abort

    SetRegView 32
  ${EndIf}
!endif

  ;Check if setup is already running
  System::Call 'kernel32::CreateMutexW(i 0, i 0, t "WZPlayerSetup") i .r1 ?e'
  Pop $R0

  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONEXCLAMATION $(Installer_Is_Running)
    Abort

!ifdef USE_RUNCHECK
  ;Check if WZPlayer is running
  ;Allow skipping check using /NORUNCHECK
  ${GetParameters} $R0
  ${GetOptions} $R0 "/NORUNCHECK" $R1
  IfErrors 0 +2
    Call RunCheck
!endif

  ;Check for admin on < Vista
  UserInfo::GetAccountType
  Pop $R0
  ${If} $R0 != "admin"
    MessageBox MB_OK|MB_ICONSTOP $(Installer_No_Admin)
    Abort
  ${EndIf}

  ;Setup language selection
  !insertmacro MUI_LANGDLL_DISPLAY

  Call LoadPreviousSettings

  Call CheckPreviousVersion

  SetShellVarContext all

FunctionEnd

Function .onInstSuccess

  ${MementoSectionSave}

FunctionEnd

Function .onInstFailed

  SetDetailsPrint textonly
  DetailPrint $(Info_RollBack)
  SetDetailsPrint listonly

  !insertmacro MacroRemoveWZPlayer

  Delete "$INSTDIR\${WZPLAYER_UNINST_EXE}"
  RMDir "$INSTDIR"

FunctionEnd

Function .onSelChange

  ${Unless} ${SectionIsSelected} ${SecMPlayer}
  ${AndUnless} ${SectionIsSelected} ${SecMPV}
    !insertmacro SelectSection ${SecMPlayer}
  ${EndUnless}

FunctionEnd

Function CheckPreviousVersion

  ClearErrors
  ReadRegStr $Previous_Version HKLM "${WZPLAYER_REG_KEY}" "Version"
  ReadRegStr $WZPlayer_UnStrPath HKLM "${WZPLAYER_UNINST_KEY}" "UninstallString"
  ReadRegStr $WZPlayer_Path HKLM "${WZPLAYER_REG_KEY}" "Path"

  ${IfNot} ${Errors}
    StrCpy $Reinstall_Uninstall 1
    !ifdef WIN64
    ;Workaround for InstallDirRegKey on 64-bit
    StrCpy $INSTDIR $WZPlayer_Path
    !endif

    ;Since we can't get input from a silent install to initialize the variables, prefer upgrading
    ${If} ${Silent}
      StrCpy $Reinstall_UninstallButton_State 0
      StrCpy $Reinstall_OverwriteButton_State 1
    ${EndIf}
  ${EndIf}

  /* $Previous_Version_State Assignments:
  $Previous_Version_State=0  This installer is the same version as the installed copy
  $Previous_Version_State=1  A newer version than this installer is already installed
  $Previous_Version_State=2  An older version than this installer is already installed */
  ${VersionCompare} $Previous_Version ${WZPLAYER_VERSION} $Previous_Version_State

  ${If} $Previous_Version_State == 0
    StrCpy $Inst_Type $(Type_Reinstall)
  ${ElseIf} $Previous_Version_State == 1
    StrCpy $Inst_Type $(Type_Downgrade)
  ${ElseIf} $Previous_Version_State == 2
    StrCpy $Inst_Type $(Type_Upgrade)
  ${EndIf}

FunctionEnd

!ifndef WIN64
Function Backup_Codecs

  ${IfNot} ${SectionIsSelected} ${SecMPlayer}
    Return
  ${EndIf}

  IfFileExists "$WZPlayer_Path\mplayer\codecs\*.dll" 0 NoBackup
    DetailPrint $(Info_Codecs_Backup)
    CreateDirectory "$PLUGINSDIR\codecbak"
    CopyFiles /SILENT "$WZPlayer_Path\mplayer\codecs\*" "$PLUGINSDIR\codecbak"
    StrCpy $Restore_Codecs 1
    Return
  NoBackup:
    StrCpy $Restore_Codecs 0

FunctionEnd
!endif


Function LoadPreviousSettings

  ;Gets previous start menu folder name
  !insertmacro MUI_STARTMENU_GETFOLDER "WZP_SMenu" $WZPlayer_StartMenuFolder

  ${MementoSectionRestore}

FunctionEnd

Function PageReinstall

  ${If} $Reinstall_Uninstall != 1
    Abort
  ${EndIf}

  nsDialogs::Create /NOUNLOAD 1018
  Pop $Dialog_Reinstall

  nsDialogs::SetRTL $(^RTL)

  !insertmacro MUI_HEADER_TEXT $(Reinstall_Header_Text) $(Reinstall_Header_SubText)

  ${NSD_CreateLabel} 0 0 225u 8u $(Reinstall_Msg1)

  ${NSD_CreateText} 10u 15u 290u 14u "$WZPlayer_Path"
  Pop $R0

  ${NSD_CreateLabel} 0 40u 100u 8u $(Reinstall_Msg2)

  ${NSD_CreateRadioButton} 10u 58u 200u 8u $(Reinstall_Overwrite)
  Pop $Reinstall_OverwriteButton
  ${NSD_CreateRadioButton} 10u 73u 200u 8u $(Reinstall_Uninstall)
  Pop $Reinstall_UninstallButton

  ${NSD_CreateCheckBox} 0 90u 100% 8u $(Reinstall_Msg4)
  Pop $Reinstall_ChgSettings

  ${NSD_CreateCheckBox} 0 102u 100% 8u $(Reinstall_Msg5)
  Pop $Reinstall_RemoveSettings

  ${NSD_CreateLabel} 0 121u 100% 16u 
  Pop $Reinstall_Message

  SendMessage $Reinstall_OverwriteButton ${BM_SETCHECK} 1 0
  EnableWindow $R0 0

  ${If} $Reinstall_ChgSettings_State == 1
    SendMessage $Reinstall_ChgSettings ${BM_SETCHECK} 1 0
  ${Endif}

  ${If} $Reinstall_RemoveSettings_State == 1
    SendMessage $Reinstall_RemoveSettings ${BM_SETCHECK} 1 0
  ${Endif}

  ${NSD_OnClick} $Reinstall_OverwriteButton PageReinstallUpdate
  ${NSD_OnClick} $Reinstall_UninstallButton PageReinstallUpdate
  ${NSD_OnClick} $Reinstall_ChgSettings PageReinstallUpdate
  ${NSD_OnClick} $Reinstall_RemoveSettings RemoveSettingsUpdate

  Call PageReinstallUpdate

  nsDialogs::Show

FunctionEnd

Function PageReinstallLeave

  ${NSD_GetState} $Reinstall_OverwriteButton $Reinstall_OverwriteButton_State
  ${NSD_GetState} $Reinstall_UninstallButton $Reinstall_UninstallButton_State
  ${NSD_GetState} $Reinstall_ChgSettings $Reinstall_ChgSettings_State
  ${NSD_GetState} $Reinstall_RemoveSettings $Reinstall_RemoveSettings_State

FunctionEnd

Function RemoveSettingsUpdate

  ${NSD_GetState} $Reinstall_RemoveSettings $Reinstall_RemoveSettings_State

  ${If} $Reinstall_RemoveSettings_State == 1
    MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 $(Remove_Settings_Confirmation) /SD IDNO IDYES reset_done
      ${NSD_SetState} $Reinstall_RemoveSettings 0
    reset_done:
  ${EndIf}

FunctionEnd

Function PageReinstallUpdate

  ${NSD_GetState} $Reinstall_OverwriteButton $Reinstall_OverwriteButton_State
  ${NSD_GetState} $Reinstall_UninstallButton $Reinstall_UninstallButton_State
  ${NSD_GetState} $Reinstall_ChgSettings $Reinstall_ChgSettings_State

  ${If} $Reinstall_OverwriteButton_State == 1

    EnableWindow $Reinstall_ChgSettings 1
    EnableWindow $Reinstall_RemoveSettings 1

    GetDlgItem $R0 $HWNDPARENT 1
    ${If} $Reinstall_ChgSettings_State != 1
      SendMessage $R0 ${WM_SETTEXT} 0 "STR:$(StartBtn)"
      ${NSD_SetText} $Reinstall_Message $(Reinstall_Msg3_1)
    ${ElseIf} $Reinstall_ChgSettings_State == 1
      SendMessage $R0 ${WM_SETTEXT} 0 "STR:$(^NextBtn)"
      ${NSD_SetText} $Reinstall_Message $(Reinstall_Msg3_2)
    ${EndIf}

  ${ElseIf} $Reinstall_UninstallButton_State == 1

    EnableWindow $Reinstall_ChgSettings 0
    ${NSD_SetState} $Reinstall_ChgSettings 0

    EnableWindow $Reinstall_RemoveSettings 0
    ${NSD_SetState} $Reinstall_RemoveSettings 0

    GetDlgItem $R0 $HWNDPARENT 1
    SendMessage $R0 ${WM_SETTEXT} 0 "STR:$(^UninstallBtn)"

    ${NSD_SetText} $Reinstall_Message $(Reinstall_Msg3_3)

  ${EndIf}

FunctionEnd

Function PageComponentsPre

  ${If} $Reinstall_Uninstall == 1
  ${AndIf} $Reinstall_ChgSettings_State != 1
    Abort
  ${EndIf}

FunctionEnd

Function PageDirectoryPre

  ${If} $Reinstall_Uninstall == 1
  ${AndIf} $Reinstall_ChgSettings_State != 1
    Abort
  ${EndIf}

FunctionEnd

Function PageStartMenuPre

  ${If} $Reinstall_Uninstall == 1
  ${AndIf} $Reinstall_ChgSettings_State != 1
    Abort
  ${EndIf}

  ${IfNot} ${SectionIsSelected} ${SecStartMenuShortcut}
    Abort
  ${EndIf}

FunctionEnd

Function RegisterDefaultPrograms

  WriteRegStr HKCR "MPlayerFileVideo\DefaultIcon" "" '"$INSTDIR\wzplayer.exe",1'
  WriteRegStr HKCR "MPlayerFileVideo\shell\enqueue" "" "Enqueue in WZPlayer"
  WriteRegStr HKCR "MPlayerFileVideo\shell\enqueue\command" "" '"$INSTDIR\wzplayer.exe" -add-to-playlist "%1"'
  WriteRegStr HKCR "MPlayerFileVideo\shell\open" "FriendlyAppName" "WZPlayer Media Player"
  WriteRegStr HKCR "MPlayerFileVideo\shell\open\command" "" '"$INSTDIR\wzplayer.exe" "%1"'

  ;Modify the list of extensions added in the MacroAllExtensions macro
  WriteRegStr HKLM "${WZPLAYER_DEF_PROGS_KEY}" "" "WZPlayer"
  WriteRegStr HKLM "${WZPLAYER_DEF_PROGS_KEY}\Capabilities" "ApplicationDescription" $(Application_Description)
  WriteRegStr HKLM "${WZPLAYER_DEF_PROGS_KEY}\Capabilities" "ApplicationName" "WZPlayer"
  WriteRegStr HKLM "Software\RegisteredApplications" "WZPlayer" "${WZPLAYER_DEF_PROGS_KEY}\Capabilities"
  !insertmacro MacroAllExtensions WriteRegStrSupportedTypes

FunctionEnd

/*************************************** Uninstaller *******************************************/

Section Uninstall

  ;Make sure WZPlayer is installed from where the uninstaller is being executed.
  IfFileExists "$INSTDIR\wzplayer.exe" +2
    Abort $(Uninstaller_InvalidDirectory)

  SetDetailsPrint textonly
  DetailPrint $(Info_Rest_Assoc)
  SetDetailsPrint listonly

  ;Don't restore file associations if reinstalling
  ${un.GetParameters} $R0
  ${un.GetOptionsS} $R0 "/R" $R1

  IfErrors 0 +2
  ExecWait '"$INSTDIR\wzplayer.exe" -uninstall'

  !insertmacro MacroRemoveWZPlayer

  Delete "$INSTDIR\${WZPLAYER_UNINST_EXE}"
  RMDir "$INSTDIR"

SectionEnd

;--------------------------------
;Required functions

!insertmacro un.GetParameters
!insertmacro un.GetOptions

;--------------------------------
;Uninstaller functions

Function un.onInit

!ifdef WIN64
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP $(Uninstaller_64bitOnly)
    Abort
  ${EndIf}

  SetRegView 64
!endif

  ;Check for admin on < Vista
  UserInfo::GetAccountType
  Pop $R0
  ${If} $R0 != "admin"
    MessageBox MB_OK|MB_ICONSTOP $(Uninstaller_No_Admin)
    Abort
  ${EndIf}

!ifdef USE_RUNCHECK
  ;Check if WZPlayer is running
  ;Allow skipping check using /NORUNCHECK
  ${un.GetParameters} $R0
  ${un.GetOptions} $R0 "/NORUNCHECK" $R1
  IfErrors 0 +2
    Call un.RunCheck
!endif

  ;Gets start menu folder name
  !insertmacro MUI_STARTMENU_GETFOLDER "WZP_SMenu" $WZPlayer_StartMenuFolder

  ;Get the stored language preference
  !insertmacro MUI_UNGETLANGUAGE

FunctionEnd

Function un.ConfirmPagePre

  ${un.GetParameters} $R0

  ${un.GetOptionsS} $R0 "/X" $R1
  ${Unless} ${Errors}
    Abort
  ${EndUnless}

FunctionEnd
