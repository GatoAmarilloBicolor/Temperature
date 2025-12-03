#include <Catalog.h>
#include <Dragger.h>
#include <LayoutBuilder.h>
#include <NumberFormat.h>
#include <StringFormat.h>
#include "GraphView.h"
#include "TemperatureDefs.h"
#include "TemperatureUtils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GraphView"

GraphView::GraphView(DataFactory* dataRepo)
: BView("GraphView", B_SUPPORTS_LAYOUT | B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED, NULL),
  fStandaloneMode(false),
  fMaxDataPoints(100),
  fDataPoints(NULL),
  fCurrentValues(NULL),
  fDataRepository(dataRepo),
  fStandaloneDevice(NULL)
{
	InitGraphView();
	InitDragger();
}

GraphView::GraphView(BMessage* archive)
: BView(archive),
  fStandaloneMode(true),
  fMaxDataPoints(100),
  fDataPoints(NULL),
  fCurrentValues(NULL),
  fDataRepository(NULL),
  fStandaloneDevice(new ThermalDevice)
{
	BMessage config;
	status_t status = archive->FindMessage("config", &config);
	if(status == B_OK)
		fDataRepository = DataFactory::Instantiate(&config);
	else
		fDataRepository = new DataFactory;
	fDataRepository->fStandaloneMode = fStandaloneMode;
	fStandaloneDevice->SetTo(fDataRepository->ActiveDevice());

	InitGraphView();
}

GraphView::~GraphView()
{
	delete[] fCurrentValues;
	delete[] fDataPoints;

	if(Standalone() && fDataRepository)
		delete fDataRepository;
}

status_t GraphView::Archive(BMessage* into, bool deep) const
{
	status_t status = BView::Archive(into, deep);
	if(status == B_OK)
		status = into->AddString("add_on", kTemperatureMime);
	if(status == B_OK)
		status = into->AddString("class", "GraphView");
	if(status == B_OK) {
		BMessage config;
		fDataRepository->Archive(&config, deep);
		status = into->AddMessage("config", &config);
	}

	return status;
}

GraphView* GraphView::Instantiate(BMessage* archive)
{
	if(!validate_instantiation(archive, "GraphView")) {
		return NULL;
	}

	return new GraphView(archive);
}

void GraphView::AttachedToWindow()
{
	SetMouseEventMask(B_POINTER_EVENTS, B_SUSPEND_VIEW_FOCUS | B_LOCK_WINDOW_FOCUS);

	fGraphMenu->SetTargetForItems(this);
	fRefreshRateMenu->SetTargetForItems(this);
	fTemperatureScaleMenu->SetTargetForItems(this);
}

void GraphView::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case M_DEVICE_CHANGED:
		{
			fDataRepository->SetRunningStatus(false);

			for(int i = 0; i < fMaxDataPoints; i++)
				fCurrentValues[i] = 0.0f;

			if(Standalone()) {
				fDataRepository->SetActiveDevice(message->GetString("target"));
			}

			fDataRepository->SetRunningStatus(true);
			break;
		}
		case M_TEMPERATURE_REPLY:
		{
			float temperature = 0.0f;
			if(fDataRepository->RunningStatus() && message->FindFloat("temperature", &temperature) == B_OK) {
				for(int i = 0; i < fMaxDataPoints; i++)
					fCurrentValues[i] = fCurrentValues[i + 1];
				fCurrentValues[fMaxDataPoints - 1] = temperature;
			}
			break;
		}
		case M_GRAPHVIEW_PAUSE:
		{
			fDataRepository->SetRunningStatus(!fDataRepository->RunningStatus());
			fGraphMenu->FindItem(M_GRAPHVIEW_PAUSE)->SetMarked(!fDataRepository->RunningStatus());
			// Notify window...
			break;
		}
		case M_GRAPHVIEW_WATERMARK:
		{
			fDataRepository->SetWatermarkVisibility(!fDataRepository->WatermarkVisibility());
			fGraphMenu->FindItem(M_GRAPHVIEW_WATERMARK)->SetMarked(fDataRepository->WatermarkVisibility());
			break;
		}
		case M_GRAPHVIEW_COLOR_CHANGED:
		{
			status_t status = B_ERROR;
			const void* ptr = NULL;
			ssize_t length = 0;
			rgb_color color = {};
			if((status = message->FindData("RGBColor", B_RGB_32_BIT_TYPE, &ptr, &length)) == B_OK) {
				if(!ptr || length != sizeof(color))
					break;

				color = *(reinterpret_cast<rgb_color*>(const_cast<void*>(ptr)));
			}
			else if((status = message->FindData("RGBColor", B_RGB_COLOR_TYPE, &ptr, &length)) == B_OK) {
				if(!ptr || length != sizeof(color))
					break;

				color = *(reinterpret_cast<rgb_color*>(const_cast<void*>(ptr)));
			}

			if(status == B_OK) {
				fDataRepository->SetLineColor(color);
				fLineColor = fDataRepository->LineColor();

				Invalidate();
			}

			break;
		}
		case M_REFRESH_RATE:
		{
			uint32 rate = 1;
			if(message->FindUInt32("rate", &rate) == B_OK) {
				if(Window())
					Window()->PostMessage(message);
				fDataRepository->SetRefreshRate(rate);

				for(int32 i = 0; i < 5; i++)
					fRefreshRateMenu->ItemAt(i)->SetMarked(i == static_cast<int32>(rate - 1));
			}
			break;
		}
		case M_SCALE_CHANGED:
		{
			const void* ptr = NULL;
			ssize_t length = 0;
			if(message->FindData(kConfigTempScale, B_CHAR_TYPE, &ptr, &length) != B_OK)
				break;

			char c = *(static_cast<const char*>(ptr));
			if(!IsValidScale(c))
				break;

			int32 scaleIndex = c == SCALE_CELSIUS ? 0 : c == SCALE_FAHRENHEIT ? 1 : 2;

			if(Standalone())
				fDataRepository->SetTemperatureScale(c);
			else {
				message->AddInt32("menu_item", scaleIndex);
				message->AddBool("changed_from_graph", true);
				Window()->PostMessage(message);
			}

			for(int32 i = 0; i < SCALE_COUNT; i++)
				fTemperatureScaleMenu->ItemAt(i)->SetMarked(i == scaleIndex);

			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}

void GraphView::Pulse()
{
	if(fDataRepository->RunningStatus()) {
		BMessage request;

		if(Standalone()) {
			request.what = M_TEMPERATURE_REPLY;
			request.AddFloat("temperature", fStandaloneDevice->ReadTemperature());
			BMessenger(this).SendMessage(&request);
		}
		else {
			request.what = M_TEMPERATURE_REQUESTED;
			request.AddMessenger("handler", this);
			Window()->PostMessage(&request, Window(), this);
		}

		Invalidate();
	}
}

void GraphView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);

	auto calculateX = [=] (int initial) {
		return ((initial * Bounds().Width()) / fMaxDataPoints);
	};
	auto calculateY = [=] (float initial) {
		return (Bounds().Height() - ((initial * Bounds().Height()) / 100));
	};

	BRect borderFrame(Bounds());
	SetHighUIColor(B_CONTROL_BORDER_COLOR);
	SetPenSize(2.0f);
	StrokeRect(Bounds());

	BRect backgroundFrame(borderFrame.InsetByCopy(2.0f, 2.0f));

	if(fDataRepository->WatermarkVisibility()) {
		drawing_mode oldDwMode = DrawingMode();
		int32 stringProportion = fMaxDataPoints / 30;

		SetHighUIColor(B_CONTROL_BORDER_COLOR, B_DARKEN_2_TINT);

		BString numberString;
		BNumberFormat format;
		format.Format(numberString,
			ConvertToScale(static_cast<double>(fCurrentValues[fMaxDataPoints - 1]),
			SCALE_CELSIUS, fDataRepository->TemperatureScale()));
		BString watermarkTemp(B_TRANSLATE("%temperature% %scale%"));
		watermarkTemp.ReplaceAll("%temperature%", numberString);
		watermarkTemp.ReplaceAll("%scale%", SymbolForScale(fDataRepository->TemperatureScale()));

		BFont watermarkTempFont(be_plain_font);
		watermarkTempFont.SetSize(calculateX(stringProportion * 2));

		BPoint watermarkInsPoint(backgroundFrame.left + 10, backgroundFrame.bottom - 12);

		SetDrawingMode(B_OP_COPY);
		SetFont(&watermarkTempFont);

		DrawString(watermarkTemp.String(), watermarkInsPoint);

		BString watermarkDev(B_TRANSLATE("%device%"));
		watermarkDev.ReplaceAll("%device%", fDataRepository->ActiveDevice());

		BFont watermarkDevFont(be_plain_font);
		watermarkDevFont.SetSize(calculateX(stringProportion));

		BPoint watermarkDevInsert(backgroundFrame.right - (StringWidth(watermarkDev.String()) / 2) - 10,
			backgroundFrame.bottom - 12);

		SetFont(&watermarkDevFont);

		DrawString(watermarkDev.String(), watermarkDevInsert);

		SetDrawingMode(oldDwMode);
	}

	float midPointHeight = backgroundFrame.Height() / 2;
	BPoint midPointStart(backgroundFrame.left, midPointHeight);
	BPoint midPointEnd(backgroundFrame.Width() + 1, midPointHeight);

	SetHighUIColor(B_CONTROL_BORDER_COLOR);
	StrokeLine(midPointStart, midPointEnd);
	MovePenTo(midPointStart);

	SetHighColor(fLineColor);
	SetPenSize(4.0f);
	MovePenTo(BPoint(Bounds().left, Bounds().Height() - (fCurrentValues[0] * Bounds().Height() / 100)));

	BeginLineArray(fMaxDataPoints - 1);
	for(int i = 0; i < fMaxDataPoints; i++)
		AddLine(BPoint(calculateX(i), calculateY(fCurrentValues[i])),
			BPoint(calculateX(i + 1), calculateY(fCurrentValues[i + 1])), HighColor());
	EndLineArray();

	// Invalidate();
}

void GraphView::MouseDown(BPoint where)
{
	uint32 buttons = 0;
	GetMouse(&where, &buttons, false);

	BPoint screenWhere(where);
	ConvertToScreen(&screenWhere);

	if(buttons & B_SECONDARY_MOUSE_BUTTON)
		fGraphMenu->Go(screenWhere, true, true, true);

	BView::MouseDown(where);
}

void GraphView::FrameMoved(BPoint newPosition)
{
    BView::FrameMoved(newPosition);

    Invalidate();
}

void GraphView::FrameResized(float newWidth, float newHeight)
{
    BView::FrameResized(newWidth, newHeight);

    Invalidate();
}

// #pragma mark - Private

void GraphView::InitGraphView()
{
	fDataPoints = new int[fMaxDataPoints];
	fCurrentValues = new float[fMaxDataPoints];
	for(int i = 0; i < fMaxDataPoints; i++) {
		fDataPoints[i] = 0;
		fCurrentValues[i] = 0.0f;
	}

	SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
	fLineColor = fDataRepository->LineColor();

	SetExplicitMinSize(BSize(50, 50));

	fGraphMenu = BuildPopUpMenu();
}

void GraphView::InitDragger()
{
	BRect draggerRect(Bounds());
	draggerRect.left = Bounds().right - 7;
	draggerRect.top = Bounds().bottom - 7;

	BDragger* graphDragger = new BDragger(draggerRect, this, B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);

	BPopUpMenu* menu = new BPopUpMenu("Shelf", false, false, B_ITEMS_IN_COLUMN);
	BLayoutBuilder::Menu<>(menu)
		.AddItem(B_TRANSLATE("Remove replicant"), new BMessage('JAHA'))
	.End();
	graphDragger->SetPopUp(menu);

	AddChild(graphDragger);
}

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Graph menu"

BPopUpMenu* GraphView::BuildPopUpMenu()
{
	BPopUpMenu* menu = new BPopUpMenu(NULL, false, false);
	BLayoutBuilder::Menu<>(menu)
		.AddItem(B_TRANSLATE("Pause"), M_GRAPHVIEW_PAUSE)
		.AddMenu(fRefreshRateMenu = new BMenu(B_TRANSLATE("Refresh rate"))).End()
		.AddSeparator()
		.AddMenu(fTemperatureScaleMenu = new BMenu(B_TRANSLATE("Scale"))).End()
		.AddItem(B_TRANSLATE("Show watermark"), M_GRAPHVIEW_WATERMARK)
	.End();

	for(uint32 i = 1; i <= 5; i++) {
		BString menuItemName;
		static BStringFormat format(B_TRANSLATE(
			"{0, plural,"
			"=1{# second}"
			"other{# seconds}}"
		));
		format.Format(menuItemName, static_cast<long>(i));

		BMessage* message = new BMessage(M_REFRESH_RATE);
		message->AddUInt32("rate", i);
		fRefreshRateMenu->AddItem(new BMenuItem(menuItemName, message));
	}
	int32 rate = fDataRepository->RefreshRate();
	if(rate > 0 && rate <= 5)
		fRefreshRateMenu->ItemAt(rate - 1)->SetMarked(true);

	BMessage* scaleMessage = new BMessage(M_SCALE_CHANGED);
	auto c = SCALE_CELSIUS;
	scaleMessage->AddData(kConfigTempScale, B_CHAR_TYPE, &c, sizeof(c));
	fTemperatureScaleMenu->AddItem(new BMenuItem(B_TRANSLATE("Celsius"), std::move(scaleMessage)));
	scaleMessage = new BMessage(M_SCALE_CHANGED);
	c = SCALE_FAHRENHEIT;
	scaleMessage->AddData(kConfigTempScale, B_CHAR_TYPE, &c, sizeof(c));
	fTemperatureScaleMenu->AddItem(new BMenuItem(B_TRANSLATE("Fahrenheit"), std::move(scaleMessage)));
	scaleMessage = new BMessage(M_SCALE_CHANGED);
	c = SCALE_KELVIN;
	scaleMessage->AddData(kConfigTempScale, B_CHAR_TYPE, &c, sizeof(c));
	fTemperatureScaleMenu->AddItem(new BMenuItem(B_TRANSLATE("Kelvin"), std::move(scaleMessage)));

	menu->FindItem(M_GRAPHVIEW_PAUSE)->SetMarked(!fDataRepository->RunningStatus());
	menu->FindItem(M_GRAPHVIEW_WATERMARK)->SetMarked(fDataRepository->WatermarkVisibility());

	return menu;
}
