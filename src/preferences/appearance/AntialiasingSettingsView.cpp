/*
 * Copyright 2008, Stephan Aßmus, <superstippi@gmx.de>
 * Copyright 2008, Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AntialiasingSettingsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Box.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <TextView.h>


#include "AppearanceWindow.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AntialiasingSettingsView"


static const int32 kMsgSetRenderMode = 'rndm';
static const int32 kMsgSetAverageWeight = 'avrg';

// Combo labels
static const char* kStandardNoHintingLabel
	= B_TRANSLATE_MARK("Standard - No hinting");
static const char* kStandardLightLabel
	= B_TRANSLATE_MARK("Standard - Light hinting");
static const char* kStandardFullLabel
	= B_TRANSLATE_MARK("Standard - Full hinting");
static const char* kStandardMonospacedLabel
	= B_TRANSLATE_MARK("Standard - Full hinting (Monospaced only)");
static const char* kSubpixelLabel
	= B_TRANSLATE_MARK("Subpixel LCD");
static const char* kSubpixelLightLabel
	= B_TRANSLATE_MARK("Subpixel LCD - Light hinting");


// #pragma mark - private libbe API


enum {
	HINTING_MODE_OFF = 0,
	HINTING_MODE_ON,
	HINTING_MODE_MONOSPACED_ONLY,
	HINTING_MODE_LIGHT
};

// Unified render mode indices for the combo
enum {
	RENDER_MODE_STANDARD_NO_HINTING = 0,
	RENDER_MODE_STANDARD_LIGHT,
	RENDER_MODE_STANDARD_FULL,
	RENDER_MODE_STANDARD_MONOSPACED,
	RENDER_MODE_SUBPIXEL,
	RENDER_MODE_SUBPIXEL_LIGHT
};

static const uint8 kDefaultHintingMode = HINTING_MODE_ON;
static const unsigned char kDefaultAverageWeight = 120;
static const bool kDefaultSubpixelAntialiasing = true;

extern void set_subpixel_antialiasing(bool subpix);
extern status_t get_subpixel_antialiasing(bool* subpix);
extern void set_hinting_mode(uint8 hinting);
extern status_t get_hinting_mode(uint8* hinting);
extern void set_average_weight(unsigned char averageWeight);
extern status_t get_average_weight(unsigned char* averageWeight);


//	#pragma mark -


AntialiasingSettingsView::AntialiasingSettingsView(const char* name)
	: BView(name, 0)
{
	// collect the current system settings
	if (get_subpixel_antialiasing(&fCurrentSubpixelAntialiasing) != B_OK)
		fCurrentSubpixelAntialiasing = kDefaultSubpixelAntialiasing;
	fSavedSubpixelAntialiasing = fCurrentSubpixelAntialiasing;

	if (get_hinting_mode(&fCurrentHinting) != B_OK)
		fCurrentHinting = kDefaultHintingMode;
	fSavedHinting = fCurrentHinting;

	if (get_average_weight(&fCurrentAverageWeight) != B_OK)
		fCurrentAverageWeight = kDefaultAverageWeight;
	fSavedAverageWeight = fCurrentAverageWeight;

	// create the controls

	// unified render mode menu
	_BuildRenderModeMenu();
	fRenderModeMenuField = new BMenuField("renderMode",
		B_TRANSLATE("Font rendering:"), fRenderModeMenu);

	// "average weight" in subpixel filtering
	fAverageWeightControl = new BSlider("averageWeightControl",
		B_TRANSLATE("Reduce colored edges filter strength:"),
		new BMessage(kMsgSetAverageWeight), 0, 255, B_HORIZONTAL);
	fAverageWeightControl->SetLimitLabels(B_TRANSLATE("Off"),
		B_TRANSLATE("Strong"));
	fAverageWeightControl->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fAverageWeightControl->SetHashMarkCount(255 / 15);
	fAverageWeightControl->SetEnabled(fCurrentSubpixelAntialiasing);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fRenderModeMenuField->CreateLabelLayoutItem(), 0, 0)
			.Add(fRenderModeMenuField->CreateMenuBarLayoutItem(), 1, 0)
			.AddGlue(2, 0)
		.End()
		.Add(fAverageWeightControl)
		.AddGlue()
		.SetInsets(B_USE_WINDOW_SPACING);

	_SetCurrentRenderMode();
	_SetCurrentAverageWeight();
}


AntialiasingSettingsView::~AntialiasingSettingsView()
{
}


void
AntialiasingSettingsView::AttachedToWindow()
{
	AdoptParentColors();

	if (Parent() == NULL)
		SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	fRenderModeMenu->SetTargetForItems(this);
	fAverageWeightControl->SetTarget(this);
}


void
AntialiasingSettingsView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgSetRenderMode:
		{
			int32 renderMode;
			if (msg->FindInt32("render_mode", &renderMode) != B_OK)
				break;

			_ApplyRenderMode(renderMode);
			Window()->PostMessage(kMsgUpdate);
			break;
		}
		case kMsgSetAverageWeight:
		{
			int32 averageWeight = fAverageWeightControl->Value();
			if (averageWeight == fCurrentAverageWeight)
				break;

			fCurrentAverageWeight = averageWeight;
			set_average_weight(fCurrentAverageWeight);

			Window()->PostMessage(kMsgUpdate);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
AntialiasingSettingsView::_BuildRenderModeMenu()
{
	fRenderModeMenu = new BPopUpMenu(B_TRANSLATE("Render mode menu"));

	struct {
		const char* label;
		int32 mode;
	} items[] = {
		{ kStandardNoHintingLabel, RENDER_MODE_STANDARD_NO_HINTING },
		{ kStandardLightLabel, RENDER_MODE_STANDARD_LIGHT },
		{ kStandardFullLabel, RENDER_MODE_STANDARD_FULL },
		{ kStandardMonospacedLabel, RENDER_MODE_STANDARD_MONOSPACED },
		{ kSubpixelLabel, RENDER_MODE_SUBPIXEL },
		{ kSubpixelLightLabel, RENDER_MODE_SUBPIXEL_LIGHT },
	};

	for (size_t i = 0; i < B_COUNT_OF(items); i++) {
		BMessage *message = new BMessage(kMsgSetRenderMode);
		message->AddInt32("render_mode", items[i].mode);
		fRenderModeMenu->AddItem(new BMenuItem(
			B_TRANSLATE_NOCOLLECT(items[i].label), message));
	}
}


void
AntialiasingSettingsView::_ApplyRenderMode(int32 renderMode)
{
	bool subpixel = false;
	uint8 hinting = HINTING_MODE_ON;

	switch (renderMode) {
		case RENDER_MODE_STANDARD_NO_HINTING:
			subpixel = false;
			hinting = HINTING_MODE_OFF;
			break;
		case RENDER_MODE_STANDARD_LIGHT:
			subpixel = false;
			hinting = HINTING_MODE_LIGHT;
			break;
		case RENDER_MODE_STANDARD_FULL:
			subpixel = false;
			hinting = HINTING_MODE_ON;
			break;
		case RENDER_MODE_STANDARD_MONOSPACED:
			subpixel = false;
			hinting = HINTING_MODE_MONOSPACED_ONLY;
			break;
		case RENDER_MODE_SUBPIXEL:
			subpixel = true;
			hinting = HINTING_MODE_ON;
			break;
		case RENDER_MODE_SUBPIXEL_LIGHT:
			subpixel = true;
			hinting = HINTING_MODE_LIGHT;
			break;
	}

	if (subpixel != fCurrentSubpixelAntialiasing) {
		fCurrentSubpixelAntialiasing = subpixel;
		set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
	}

	if (hinting != fCurrentHinting) {
		fCurrentHinting = hinting;
		set_hinting_mode(fCurrentHinting);
	}

	fAverageWeightControl->SetEnabled(fCurrentSubpixelAntialiasing);
}


int32
AntialiasingSettingsView::_CurrentRenderMode() const
{
	if (fCurrentSubpixelAntialiasing) {
		if (fCurrentHinting == HINTING_MODE_LIGHT)
			return RENDER_MODE_SUBPIXEL_LIGHT;
		return RENDER_MODE_SUBPIXEL;
	}

	switch (fCurrentHinting) {
		case HINTING_MODE_OFF:
			return RENDER_MODE_STANDARD_NO_HINTING;
		case HINTING_MODE_LIGHT:
			return RENDER_MODE_STANDARD_LIGHT;
		case HINTING_MODE_MONOSPACED_ONLY:
			return RENDER_MODE_STANDARD_MONOSPACED;
		default:
			return RENDER_MODE_STANDARD_FULL;
	}
}


void
AntialiasingSettingsView::_SetCurrentRenderMode()
{
	int32 renderMode = _CurrentRenderMode();
	BMenuItem *item = fRenderModeMenu->ItemAt(renderMode);
	if (item != NULL)
		item->SetMarked(true);
	fAverageWeightControl->SetEnabled(fCurrentSubpixelAntialiasing);
}


void
AntialiasingSettingsView::_SetCurrentAverageWeight()
{
	fAverageWeightControl->SetValue(fCurrentAverageWeight);
}


void
AntialiasingSettingsView::SetDefaults()
{
	if (!IsDefaultable())
		return;

	fCurrentSubpixelAntialiasing = kDefaultSubpixelAntialiasing;
	fCurrentHinting = kDefaultHintingMode;
	fCurrentAverageWeight = kDefaultAverageWeight;

	set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
	set_hinting_mode(fCurrentHinting);
	set_average_weight(fCurrentAverageWeight);

	_SetCurrentRenderMode();
	_SetCurrentAverageWeight();
}


bool
AntialiasingSettingsView::IsDefaultable()
{
	return fCurrentSubpixelAntialiasing != kDefaultSubpixelAntialiasing
		|| fCurrentHinting != kDefaultHintingMode
		|| fCurrentAverageWeight != kDefaultAverageWeight;
}


bool
AntialiasingSettingsView::IsRevertable()
{
	return fCurrentSubpixelAntialiasing != fSavedSubpixelAntialiasing
		|| fCurrentHinting != fSavedHinting
		|| fCurrentAverageWeight != fSavedAverageWeight;
}


void
AntialiasingSettingsView::Revert()
{
	if (!IsRevertable())
		return;

	fCurrentSubpixelAntialiasing = fSavedSubpixelAntialiasing;
	fCurrentHinting = fSavedHinting;
	fCurrentAverageWeight = fSavedAverageWeight;

	set_subpixel_antialiasing(fCurrentSubpixelAntialiasing);
	set_hinting_mode(fCurrentHinting);
	set_average_weight(fCurrentAverageWeight);

	_SetCurrentRenderMode();
	_SetCurrentAverageWeight();
}
