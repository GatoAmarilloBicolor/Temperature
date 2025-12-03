/*
 * Copyright 2025, cafeina, <cafeina@world>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Directory.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <Path.h>
#include <StringList.h>
#include <SupportDefs.h>
#include "DataFactory.h"
#include "TemperatureDefs.h"
#include "TemperatureUtils.h"

DataFactory::DataFactory()
: BArchivable(),
  fStandaloneMode(false),
  fWindowRect(BRect(100,100,500,400)),
  fActiveDevice(""),
  fSecondsToRefresh(1),
  fGraphWatermarkShown(true),
  fGraphLineColor(ui_color(B_FAILURE_COLOR)),
  fGraphRunningStatus(true),
  fTemperatureScale(SCALE_CELSIUS)
{
	DataFactory::FindThermalDevices(&fDevicesList);
}

DataFactory::DataFactory(BMessage* from)
: BArchivable(from),
  fStandaloneMode(false)
{
	fWindowRect = from->GetRect(kConfigWndFrame, BRect(100,100,500,400));
	fActiveDevice = from->GetString(kConfigDevicePath, "");
	fSecondsToRefresh = from->GetUInt32(kConfigWndPulse, 1);
	fGraphWatermarkShown = from->GetBool(kConfigGraphWMark, true);
	fGraphLineColor = from->GetColor(kConfigGraphLColor, ui_color(B_FAILURE_COLOR));
	fGraphRunningStatus = from->GetBool(kConfigGraphRun, true);
	const void* ptr = NULL;
	ssize_t length = 0;
	if(from->FindData(kConfigTempScale, B_CHAR_TYPE, &ptr, &length) == B_OK) {
		char c = *(static_cast<const char*>(ptr));
		if(c != SCALE_CELSIUS && c != SCALE_KELVIN && c != SCALE_FAHRENHEIT)
			c = SCALE_CELSIUS;
		else
			fTemperatureScale = c;
	}
	else
		fTemperatureScale = SCALE_CELSIUS;

	DataFactory::FindThermalDevices(&fDevicesList);
}

DataFactory::DataFactory(const DataFactory& other)
: fStandaloneMode(other.fStandaloneMode),
  fWindowRect(other.fWindowRect),
  fActiveDevice(other.fActiveDevice),
  fSecondsToRefresh(other.fSecondsToRefresh),
  fGraphWatermarkShown(other.fGraphWatermarkShown),
  fGraphLineColor(other.fGraphLineColor),
  fGraphRunningStatus(other.fGraphRunningStatus),
  fTemperatureScale(other.fTemperatureScale)
{
	DataFactory::FindThermalDevices(&fDevicesList);
}

DataFactory::~DataFactory()
{
}

// #pragma mark - BArchivable

status_t DataFactory::AllArchived(BMessage* archive) const
{
	return BArchivable::AllArchived(archive);
}

status_t DataFactory::AllUnarchived(const BMessage* archive)
{
	return BArchivable::AllUnarchived(archive);
}

status_t DataFactory::Archive(BMessage* into, bool deep) const
{
	if(into->ReplaceRect(kConfigWndFrame, fWindowRect) != B_OK)
		into->AddRect(kConfigWndFrame, fWindowRect);
	if(into->ReplaceString(kConfigDevicePath, fActiveDevice) != B_OK)
		into->AddString(kConfigDevicePath, fActiveDevice);
	if(into->ReplaceUInt32(kConfigWndPulse, fSecondsToRefresh) != B_OK)
		into->AddUInt32(kConfigWndPulse, fSecondsToRefresh);
	if(into->ReplaceBool(kConfigGraphWMark, fGraphWatermarkShown) != B_OK)
		into->AddBool(kConfigGraphWMark, fGraphWatermarkShown);
	if(into->ReplaceColor(kConfigGraphLColor, fGraphLineColor) != B_OK)
		into->AddColor(kConfigGraphLColor, fGraphLineColor);
	if(into->ReplaceBool(kConfigGraphRun, fGraphRunningStatus) != B_OK)
		into->AddBool(kConfigGraphRun, fGraphRunningStatus);
	if(into->ReplaceData(kConfigTempScale, B_CHAR_TYPE, &fTemperatureScale, sizeof(char)) != B_OK)
		into->AddData(kConfigTempScale, B_CHAR_TYPE, &fTemperatureScale, sizeof(char));

	return BArchivable::Archive(into, deep);
}

status_t DataFactory::Perform(perform_code opcode, void* argument)
{
	switch(static_cast<uint32>(opcode))
	{
		case M_RESTORE_DEFAULTS:
		{
			if(fStandaloneMode)
				return B_NOT_ALLOWED;

			BMessage defaults;
			DataFactory::DefaultSettings(&defaults);
			SetActiveDevice(fDevicesList.StringAt(0).String());
			SetRefreshRate(defaults.GetUInt32(kConfigWndPulse, 1));
			SetWatermarkVisibility(defaults.GetBool(kConfigGraphWMark));
			SetLineColor(defaults.GetColor(kConfigGraphLColor, ui_color(B_FAILURE_COLOR)));
			SetRunningStatus(defaults.GetBool(kConfigGraphRun));
			const void* ptr = NULL;
			ssize_t length = 0;
			defaults.FindData(kConfigTempScale, B_CHAR_TYPE, &ptr, &length);
			char c = *(static_cast<const char*>(ptr));
			SetTemperatureScale(c);
			return B_OK;
		}
		default:
			return BArchivable::Perform(opcode, argument);
	}
}

/* static */
DataFactory* DataFactory::Instantiate(BMessage* archive)
{
	if(!validate_instantiation(archive, "DataFactory"))
		return NULL;

	return new DataFactory(archive);
}

// #pragma mark - Devices

/* static */
status_t DataFactory::FindThermalDevices(BStringList* outList)
{
	BDirectory powerDirectory("/dev/power");
	if(powerDirectory.InitCheck() != B_OK)
		return B_IO_ERROR;

	BEntry powerEntry;
	while(powerDirectory.GetNextEntry(&powerEntry, false) == B_OK) {
		BPath path;
		powerEntry.GetPath(&path);
		if(strstr(path.Leaf(), "thermal") == nullptr)
			continue;

		if(powerEntry.IsDirectory()) {
			BEntry powerSubEntry;
			BDirectory powerEntryDirectory(&powerEntry);
			while(powerEntryDirectory.GetNextEntry(&powerSubEntry, false) == B_OK) {
				if(!powerSubEntry.IsDirectory()) {
					BPath subEntryPath;
					powerSubEntry.GetPath(&subEntryPath);
					outList->Add(BString(subEntryPath.Path()));
				}
			}
		}
		else {
			outList->Add(BString(path.Path()));
		}
	}

	return B_OK;
}

const BStringList& DataFactory::ThermalDevices() const
{
	return fDevicesList;
}

// #pragma mark - Settings

/* static */
void DataFactory::DefaultSettings(BMessage* archive)
{
	archive->MakeEmpty();

    archive->AddString(kConfigDevicePath, "");
    archive->AddRect(kConfigWndFrame, BRect(100,100,500,400));
    archive->AddUInt32(kConfigWndPulse, 1);
    archive->AddBool(kConfigGraphRun, true);
	archive->AddColor(kConfigGraphLColor, ui_color(B_FAILURE_COLOR));
    archive->AddBool(kConfigGraphWMark, true);
	char scale = SCALE_CELSIUS;
	archive->AddData(kConfigTempScale, B_CHAR_TYPE, &scale, sizeof(scale));
}

void DataFactory::SetWindowRect(BRect frame)
{
	fWindowRect = frame;
}

const BRect& DataFactory::WindowRect() const
{
	return fWindowRect;
}

void DataFactory::SetActiveDevice(const char* devicePath)
{
	fActiveDevice.SetTo(devicePath);
}

const char* DataFactory::ActiveDevice() const
{
	return fActiveDevice.String();
}

void DataFactory::SetRefreshRate(uint32 seconds)
{
	fSecondsToRefresh = seconds;
}

uint32 DataFactory::RefreshRate() const
{
	return fSecondsToRefresh;
}

void DataFactory::SetWatermarkVisibility(bool state)
{
	fGraphWatermarkShown = state;
}

bool DataFactory::WatermarkVisibility() const
{
	return fGraphWatermarkShown;
}

void DataFactory::SetLineColor(rgb_color color)
{
	fGraphLineColor = color;
}

rgb_color DataFactory::LineColor() const
{
	return fGraphLineColor;
}

void DataFactory::SetRunningStatus(bool state)
{
	fGraphRunningStatus = state;
}

bool DataFactory::RunningStatus() const
{
	return fGraphRunningStatus;
}

void DataFactory::SetTemperatureScale(char scale)
{
	fTemperatureScale = scale;
}

const char DataFactory::TemperatureScale() const
{
	return fTemperatureScale;
}
