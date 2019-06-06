#include "config.h"


const Qt::WindowFlags TConfig::DIALOG_FLAGS =
        Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint;

const int TConfig::MESSAGE_DURATION = 3500;
const int TConfig::ERROR_MESSAGE_DURATION = 6000;

const double TConfig::ZOOM_MIN = 0.02;
const double TConfig::ZOOM_MAX = 8.0;
const double TConfig::ZOOM_STEP = 0.02;

const int TConfig::PAN_STEP = 8;

const QString TConfig::PROGRAM_ORG("WH");
const QString TConfig::PROGRAM_ID("wzplayer");
const QString TConfig::PROGRAM_NAME("WZPlayer");
const QString TConfig::PROGRAM_VERSION(WZPLAYER_VERSION_STR);
const QString TConfig::WZPLAYLIST(TConfig::PROGRAM_ID + ".m3u8");

const QString TConfig::URL_HOMEPAGE("https://github.com/wilbert2000/wzplayer");
const QString TConfig::URL_TRANSLATOR_TEAM("http://www.transifex.com/projects/p/wzplayer/");
const QString TConfig::URL_TRANSLATORS("http://www.smplayer.info/translators.php");
const QString TConfig::URL_CHANGES("https://github.com/wilbert2000/wzplayer/commits/master");
const QString TConfig::URL_VERSION_INFO("https://raw.githubusercontent.com/wilbert2000/wzplayer/master/latest_versions.txt");
const QString TConfig::URL_SMPLAYER("http://www.smplayer.info");
const QString TConfig::URL_MPLAYER("http://www.mplayerhq.hu/design7/info.html");
const QString TConfig::URL_MPV("http://www.smplayer.info");

