#ifndef GUI_VIDEOFILTERMENU_H
#define GUI_VIDEOFILTERMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {
namespace Action {

class TAction;
class TActionGroup;


class TMenuVideoFilter : public TMenu {
	Q_OBJECT
public:
    explicit TMenuVideoFilter(TBase* mw, TCore* c);
protected:
    virtual void enableActions();
	virtual void onMediaSettingsChanged(Settings::TMediaSettings*);
	virtual void onAboutToShow();
private:
	TCore* core;

	QActionGroup* group;
	TAction* postProcessingAct;
	TAction* deblockAct;
	TAction* deringAct;
	TAction* gradfunAct;
	TAction* addNoiseAct;
	TAction* addLetterboxAct;
	TAction* upscaleAct;
	TAction* phaseAct;

	// Denoise Action Group
	TActionGroup* denoiseGroup;
	TAction* denoiseNoneAct;
	TAction* denoiseNormalAct;
	TAction* denoiseSoftAct;

	// Blur-sharpen group
	TActionGroup* unsharpGroup;
	TAction* unsharpNoneAct;
	TAction* blurAct;
	TAction* sharpenAct;

	void updateFilters();

private slots:
	void onAboutToShowDenoise();
	void onAboutToShowUnSharp();
}; // class TMenuVideoFilter

} // namespace Action
} // namespace GUI

#endif // GUI_VIDEOFILTERMENU_H
