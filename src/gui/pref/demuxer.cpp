#include "gui/pref/demuxer.h"
#include "images.h"
#include "settings/preferences.h"


namespace Gui {
namespace Pref {

TDemuxer::TDemuxer(QWidget* parent) :
    TSection(parent, 0) {

    setupUi(this);

    retranslateStrings();
}

void TDemuxer::retranslateStrings() {

    retranslateUi(this);

    icon1_label->setPixmap(Images::icon("logo", 64));
    icon2_label->setPixmap(Images::icon("mirror"));
    icon3_label->setPixmap(Images::icon("pref_video"));
    icon4_label->setPixmap(Images::icon("speaker"));
    //icon5_label->setPixmap(Images::icon("pref_subtitles"));
    icon5_label->setPixmap(Images::icon("sub"));

    createHelp();
}

QString TDemuxer::sectionName() {
    return tr("Demuxer");
}

QPixmap TDemuxer::sectionIcon() {
    return Images::icon("pref_demuxer", iconSize);
}

void TDemuxer::setData(Settings::TPreferences* pref) {

    lavf_demuxer_check->setChecked(pref->use_lavf_demuxer);
    idx_check->setChecked(pref->use_idx);
}

void TDemuxer::getData(Settings::TPreferences* pref) {

    TSection::getData(pref);
    restartIfBoolChanged(pref->use_lavf_demuxer,
                         lavf_demuxer_check->isChecked(),
                         "use_lavf_demuxer");
    restartIfBoolChanged(pref->use_idx, idx_check->isChecked(), "use_idx");
}

void TDemuxer::createHelp() {

    addSectionTitle(tr("Demuxer"));

    setWhatsThis(lavf_demuxer_check, tr("Use the lavf demuxer by default"),
        tr("If checked, the lavf demuxer will be used for all formats."
           " Note: you can set the demuxer for the current container through"
           " the demuxer tab of the properties dialog."));

    setWhatsThis(idx_check, tr("Rebuild index if needed"),
        tr("Rebuilds index of files if no index was found, allowing seeking. "
           "Useful with broken/incomplete downloads, or badly created files. "
           "This option only works if the underlying media supports "
           "seeking (i.e. not with stdin, pipe, etc).<br> "
           "<b>Note:</b> the creation of the index may take some time."));
}

} // namespace Pref
} // namespace Gui

#include "moc_demuxer.cpp"
