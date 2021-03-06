﻿;Language: Turkish (1055)
;Turkish language strings for the Windows SMPlayer NSIS installer.
;
;Save file as UTF-8 w/ BOM
;

!insertmacro LANGFILE "Turkish" "Türkçe"

; Startup
${LangFileString} Installer_Is_Running "Kurulum zaten çalışıyor."
${LangFileString} Installer_No_Admin "Bu programı yüklerken yönetici olarak oturum açmanız gerekir."
${LangFileString} SMPlayer_Is_Running "SMPlayer bir örneği çalışıyor. SMPlayer'ı kapatın ve tekrar deneyin."

${LangFileString} OS_Not_Supported "İşletim sistemi desteklenmiyor.$\nSMPlayer ${SMPLAYER_VERSION} en az Windows XP gerektirir ve sisteminizde düzgün çalışmayabilir.$\nGerçekten kuruluma devam etmek istiyor musunuz?"
${LangFileString} Win64_Required "Bu yazılım kurulumu için 64-bit Windows işletim sistemi gerekli."
${LangFileString} Existing_32bitInst "SMPlayer için varolan bir 32-bit yükleme var. Öncelikle 32-bit SMPlayer'ı kaldırmanız gerekir."
${LangFileString} Existing_64bitInst "SMPlayer için varolan bir 64-bit yükleme var. Öncelikle 64-bit SMPlayer'ı kaldırmanız gerekir."

; Welcome page
${LangFileString} WelcomePage_Title "$(^NameDA) Kurulum"
${LangFileString} WelcomePage_Text "Kurulum $(^NameDA) 'ın kurulumunda size rehberlik edecek.$\r$\n$\r$\nKurulum başlamadan önce SMPlayer'ın tüm benzerlerini kapatmanız önerilir. Bu bilgisayarınızı yeniden başlatmak zorunda kalmadan ilgili program dosyalarını güncellemek mümkün olacaktır.$\r$\n$\r$\n$_CLICK"

; Components Page
${LangFileString} ShortcutGroupTitle "Kısayollar"
${LangFileString} MPlayerGroupTitle "MPlayer Bileşenleri"
${LangFileString} MPlayerMPVGroupTitle "Multimedya Motoru"

${LangFileString} Section_SMPlayer "SMPlayer (gerekli)"
${LangFileString} Section_SMPlayer_Desc "SMPlayer, paylaşımlı kütüphaneleri, ve dökümanı."

${LangFileString} Section_DesktopShortcut "Masaüstü"
${LangFileString} Section_DesktopShortcut_Desc "Masaüstünde SMPlayer kısayolu oluşturacak."

${LangFileString} Section_StartMenu "Başlat Menüsü"
${LangFileString} Section_StartMenu_Desc "SMPlayer için Başlat Menüsünde giriş oluşturulacak."

${LangFileString} Section_MPlayer "MPlayer (gerekli)"
${LangFileString} Section_MPlayer_Desc "MPlayer; oynatmak için gerekli."

${LangFileString} Section_MPlayerCodecs "İkili Kodekleri"
!ifdef WIN64
${LangFileString} Section_MPlayerCodecs_Desc "İkili Kodekler bu sürüm için desteklenmiyor."
!else
${LangFileString} Section_MPlayerCodecs_Desc "MPlayer için isteğe bağlı kodekler. (Kurulum için internet bağlantısı gereklidir)"
!endif

${LangFileString} Section_MPV_Desc "A feature-rich fork of MPlayer && MPlayer2"

${LangFileString} Section_MEncoder_Desc "A companion program to MPlayer that can be used to encode or transform supported audio or video streams."

${LangFileString} Section_IconThemes "Simge Temaları"
${LangFileString} Section_IconThemes_Desc "SMPlayer için ek simge temaları."

${LangFileString} Section_Translations "Diller"
${LangFileString} Section_Translations_Desc "SMPlayer için ingilizce olmayan dil dosyaları."

${LangFileString} Section_ResetSettings_Desc "SMPlayer'in önceki kurulum tercihleri silinir."

${LangFileString} MPlayer_Codec_Msg "The binary codec packages add support for codecs that are not yet implemented natively, like newer RealVideo variants and a lot of uncommon formats.$\nNote that they are not necessary to play most common formats like DVDs, MPEG-1/2/4, etc."

; Upgrade/Reinstall Page
${LangFileString} Reinstall_Header_Text "Kurulum Tipini Seç"
${LangFileString} Reinstall_Header_SubText "Üzerine yazma veya kaldırma modunu seç."

${LangFileString} Reinstall_Msg1 "Aşağıdaki klasörde SMPlayer'ın var olan bir kurulumu var:"
${LangFileString} Reinstall_Msg2 "Lütfen nasıl devam edeceğinizi seçin:"
${LangFileString} Reinstall_Overwrite "Önceki kurulum ($Inst_Type) üzerine yaz"
${LangFileString} Reinstall_Uninstall "Varolan yüklemeyi kaldırın (sil)"
${LangFileString} Reinstall_Msg3_1 "Hazır olduğunuzda devam etmek başlata tıklatın."
${LangFileString} Reinstall_Msg3_2 "Hazır olduğunuzda devam etmek ileri'yi tıklatın."
${LangFileString} Reinstall_Msg3_3 "Hazır olduğunuzda devam etmek Kaldır'ı tıklatın."
${LangFileString} Reinstall_Msg4 "Kurulum Ayarlarını Değiştir"
${LangFileString} Reinstall_Msg5 "SMPlayer yapılandırmasını sıfırla"

${LangFileString} Remove_Settings_Confirmation "Are you sure you want to reset your SMPlayer settings? This action cannot be reversed."

${LangFileString} Type_Reinstall "tekrar kur"
${LangFileString} Type_Downgrade "gerilet"
${LangFileString} Type_Upgrade "güncelle"

${LangFileString} StartBtn "Başlat"

; Codecs Section
${LangFileString} Codecs_DL_Msg "MPlayer Kodekleri İndiriliyor..."
${LangFileString} Codecs_DL_Retry "MPlayer kodekleri kurulumu başarısız oldu. Tekrar denensin mi?"
${LangFileString} Codecs_DL_Failed "MPlayer kodekleri indirilmesi başarısız oldu: '$R0'."
${LangFileString} Codecs_Inst_Failed "MPlayer kodekleri kurulumu başarısız oldu."

; Uninstaller
${LangFileString} Uninstaller_No_Admin "Bu kurulum sadece yönetici ayrıcalıklarına sahip bir kullanıcı tarafından kaldırılabilir."
${LangFileString} Uninstaller_Aborted "Kaldırma kullanıcı tarafından iptal edildi."
${LangFileString} Uninstaller_NotInstalled "It does not appear that SMPlayer is installed in the directory '$INSTDIR'.$\r$\nContinue anyway (not recommended)?"
${LangFileString} Uninstaller_InvalidDirectory "SMPlayer kurulumu bulunamadı."
${LangFileString} Uninstaller_64bitOnly "Bu yükleme yalnızca 64-bit Windows üzerinde kaldırılabilir."

; Vista & Later Default Programs Registration
${LangFileString} Application_Description "SMPlayer is a complete front-end for MPlayer, from basic features like playing videos, DVDs, VCDs to more advanced features like support for MPlayer filters, edl lists, and more."

; Misc
${LangFileString} Info_Codecs_Backup "Önceki kurulumdan kodekler yedekleniyor..."
${LangFileString} Info_Codecs_Restore "Önceki kurulumdaki kodekleri yenileniyor..."
${LangFileString} Info_Del_Files "Dosyalar Siliniyor..."
${LangFileString} Info_Del_Registry "Kayıt Anahtarları Siliniyor..."
${LangFileString} Info_Del_Shortcuts "Kısayollar Siliniyor..."
${LangFileString} Info_Rest_Assoc "Dosya ilişkileri yenileniyor..."
${LangFileString} Info_RollBack "Değişiklikler geri alınıyor..."
${LangFileString} Info_Files_Extract "Dosyaları ayıklanıyor..."
${LangFileString} Info_SMTube_Backup "SMTube Yedekleniyor..."
${LangFileString} Info_SMTube_Restore "SMTube önceki kurulumdan geri yükleniyor..."
${LangFileString} Info_MPV_Backup "Backing up mpv..."
${LangFileString} Info_MPV_Restore "Restoring mpv from previous installation..."

; MPV
${LangFileString} MPV_DL_Msg "Downloading mpv..."
${LangFileString} MPV_DL_Retry "mpv was not successfully installed. Retry?"
${LangFileString} MPV_DL_Failed "Failed to download mpv: '$R0'."
${LangFileString} MPV_Inst_Failed "Failed to install mpv."

; YouTube-DL
${LangFileString} YTDL_DL_Retry "youtube-dl was not successfully installed. Retry?"
${LangFileString} YTDL_DL_Failed "Failed to download youtube-dl: '$R0'."
${LangFileString} YTDL_Update_Check "Checking for youtube-dl updates..."

; Post install
${LangFileString} Info_Cleaning_Fontconfig "Cleaning fontconfig cache..."
${LangFileString} Info_Cleaning_SMPlayer "Cleaning SMPlayer settings..."
