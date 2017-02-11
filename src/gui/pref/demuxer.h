#ifndef GUI_PREF_DEMUXER_H
#define GUI_PREF_DEMUXER_H

#include "ui_demuxer.h"
#include "gui/pref/section.h"

namespace Settings {
class TPreferences;
}

namespace Gui {
namespace Pref {

class TDemuxer : public TSection, public Ui::TDemuxer {
    Q_OBJECT

public:
    TDemuxer(QWidget* parent);
    virtual ~TDemuxer();

    // Return the name of the section
    virtual QString sectionName();
    // Return the icon of the section
    virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Settings::TPreferences* pref);
    // Apply changes
    virtual void getData(Settings::TPreferences* pref);

protected:
    virtual void retranslateStrings();

private:
    void createHelp();
};

} // namespace Pref
} // namespace Gui

#endif // GUI_PREF_DEMUXER_H
