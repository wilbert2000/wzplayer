#include "gui/pref/demuxer.h"
#include "images.h"
#include "settings/preferences.h"


namespace Gui {
namespace Pref {

TDemuxer::TDemuxer(QWidget* parent) :
	TWidget(parent, 0) {

	setupUi(this);

	retranslateStrings();
}

TDemuxer::~TDemuxer() {
}

void TDemuxer::retranslateStrings() {

	retranslateUi(this);

	icon1_label->setPixmap(Images::icon("logo", 64));
	icon2_label->setPixmap(Images::icon("mirror"));
	icon3_label->setPixmap(Images::icon("pref_video"));
	icon4_label->setPixmap(Images::icon("speaker"));
	icon5_label->setPixmap(Images::icon("pref_subtitles"));

	createHelp();
}

QString TDemuxer::sectionName() {
	return tr("Demuxer");
}

QPixmap TDemuxer::sectionIcon() {
	return Images::icon("pref_demuxer", 22);
}

void TDemuxer::setData(Settings::TPreferences* pref) {

	idx_check->setChecked(pref->use_idx);
	lavf_demuxer_check->setChecked(pref->use_lavf_demuxer);
}

void TDemuxer::getData(Settings::TPreferences* pref) {

	requires_restart = false;

	restartIfBoolChanged(pref->use_idx, idx_check->isChecked());
	restartIfBoolChanged(pref->use_lavf_demuxer, lavf_demuxer_check->isChecked());
}

void TDemuxer::createHelp() {

	addSectionTitle(tr("Demuxer"));

	setWhatsThis(idx_check, tr("Rebuild index if needed"),
		tr("Rebuilds index of files if no index was found, allowing seeking. "
		   "Useful with broken/incomplete downloads, or badly created files. "
		   "This option only works if the underlying media supports "
		   "seeking (i.e. not with stdin, pipe, etc).<br> "
		   "<b>Note:</b> the creation of the index may take some time."));

	setWhatsThis(lavf_demuxer_check, tr("Use the lavf demuxer by default"),
		tr("If this option is checked, the lavf demuxer will be used for all formats."));

}

} // namespace Pref
} // namespace Gui

#include "moc_demuxer.cpp"
