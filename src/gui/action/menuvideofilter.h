#ifndef GUI_VIDEOFILTERMENU_H
#define GUI_VIDEOFILTERMENU_H

#include "gui/action/menu.h"


class TCore;

namespace Gui {

class TAction;
class TActionGroup;

class TVideoFilterMenu : public TMenu {
	Q_OBJECT
public:
	explicit TVideoFilterMenu(QWidget* parent, TCore* c);
protected:
	virtual void enableActions(bool stopped, bool video, bool);
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
};

} // namespace GUI

#endif // GUI_VIDEOFILTERMENU_H
