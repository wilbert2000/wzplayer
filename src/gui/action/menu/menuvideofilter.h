#ifndef GUI_ACTION_MENU_MENUVIDEOFILTER_H
#define GUI_ACTION_MENU_MENUVIDEOFILTER_H

#include "gui/action/menu/menu.h"


namespace Gui {
namespace Action {

class TAction;
class TActionGroup;

namespace Menu {

class TMenuVideoFilter : public TMenu {
    Q_OBJECT
public:
    explicit TMenuVideoFilter(TMainWindow* mw);
protected:
    virtual void enableActions();
    virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
    virtual void onAboutToShow();
private:
    QActionGroup* group;
    TAction* postProcessingAct;
    TAction* deblockAct;
    TAction* deringAct;
    TAction* gradfunAct;
    TAction* addNoiseAct;
    TAction* addLetterboxAct;
    TAction* softwareScalingAct;
    TAction* phaseAct;

    // Denoise Action Group
    TActionGroup* denoiseGroup;
    TAction* denoiseNoneAct;
    TAction* denoiseNormalAct;
    TAction* denoiseSoftAct;

    // Blur-sharpen group
    TActionGroup* sharpenGroup;
    TAction* sharpenNoneAct;
    TAction* blurAct;
    TAction* sharpenAct;

    void updateFilters();

private slots:
    void onAboutToShowDenoise();
    void onAboutToShowUnSharp();
}; // class TMenuVideoFilter

} // namespace Menu
} // namespace Action
} // namespace GUI

#endif // GUI_ACTION_MENU_MENUVIDEOFILTER_H
