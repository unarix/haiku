/*
 * Copyright 2008, Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ANTIALIASING_SETTINGS_VIEW_H
#define ANTIALIASING_SETTINGS_VIEW_H


#include <View.h>

class BMenuField;
class BPopUpMenu;
class BSlider;


class AntialiasingSettingsView : public BView {
public:
							AntialiasingSettingsView(const char* name);
	virtual					~AntialiasingSettingsView();

	virtual	void			AttachedToWindow();
	virtual	void			MessageReceived(BMessage* message);

			void			SetDefaults();
			void			Revert();
			bool			IsDefaultable();
			bool			IsRevertable();

private:
			void			_BuildRenderModeMenu();
			void			_ApplyRenderMode(int32 renderMode);
			int32			_CurrentRenderMode() const;
			void			_SetCurrentRenderMode();
			void			_SetCurrentAverageWeight();

protected:
			BMenuField*		fRenderModeMenuField;
			BPopUpMenu*		fRenderModeMenu;
			BSlider*		fAverageWeightControl;

			bool			fSavedSubpixelAntialiasing;
			bool			fCurrentSubpixelAntialiasing;
			uint8			fSavedHinting;
			uint8			fCurrentHinting;
			unsigned char	fSavedAverageWeight;
			unsigned char	fCurrentAverageWeight;
};


#endif // ANTIALIASING_SETTINGS_VIEW_H
