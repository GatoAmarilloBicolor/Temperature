/*
 * Copyright 2025, cafeina, <cafeina@world>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __TEMPERATURE_GRAPH__
#define __TEMPERATURE_GRAPH__

#include <PopUpMenu.h>
#include <View.h>
#include <queue>
#include "DataFactory.h"
#include "ThermalDevice.h"

class GraphView : public BView
{
public:
	GraphView(DataFactory* dataRepo);
	GraphView(BMessage* archive);
	~GraphView() override;

	void AttachedToWindow() override;
	void MessageReceived(BMessage* message) override;
	void Pulse() override;
	void Draw(BRect updateRect) override;
	void MouseDown(BPoint where) override;
    void FrameMoved(BPoint newPosition) override;
    void FrameResized(float newWidth, float newHeight) override;

	status_t Archive(BMessage* into, bool deep = true) const override;
	static GraphView* Instantiate(BMessage* archive);
protected:
	bool Standalone() const { return fStandaloneMode; }
private:
	void InitUIData(BMessage* settings);
	void InitGraphView();
	void InitDragger();
	BPopUpMenu* BuildPopUpMenu();
private:
	bool		fStandaloneMode;
	int			fMaxDataPoints;
	int*		fDataPoints;
	float*		fCurrentValues;
	rgb_color	fLineColor;

	DataFactory* fDataRepository;
	ThermalDevice* fStandaloneDevice;

	BPopUpMenu* fGraphMenu;
	BMenu*		fRefreshRateMenu;
	BMenu*		fTemperatureScaleMenu;
};

#endif /* __TEMPERATURE_GRAPH__ */
