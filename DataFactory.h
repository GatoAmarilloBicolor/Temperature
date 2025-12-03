/*
 * Copyright 2025, cafeina, <cafeina@world>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __DATA_FACTORY__
#define __DATA_FACTORY__

#include <Archivable.h>
#include <GraphicsDefs.h>
#include <String.h>
#include <StringList.h>

class DataFactory : public BArchivable
{
public:
	DataFactory();
	DataFactory(BMessage* from);
	DataFactory(const DataFactory& other);
	~DataFactory() override;

	// BArchivable interface
	status_t AllArchived(BMessage* archive) const override;
	status_t AllUnarchived(const BMessage* archive) override;
	status_t Archive(BMessage* into, bool deep = true) const override;
	status_t Perform(perform_code opcode, void* argument) override;

	static DataFactory* Instantiate(BMessage* archive);

	// Device utils
	static status_t FindThermalDevices(BStringList* outList);
	const BStringList& ThermalDevices() const;

	// Application settings
	static void DefaultSettings(BMessage* archive);

	void SetWindowRect(BRect frame);
	const BRect& WindowRect() const;

	void SetActiveDevice(const char* devicePath);
	const char* ActiveDevice() const;

	void SetRefreshRate(uint32 seconds);
	uint32 RefreshRate() const;

	void SetWatermarkVisibility(bool state);
	bool WatermarkVisibility() const;

	void SetLineColor(rgb_color color);
	rgb_color LineColor() const;

	void SetRunningStatus(bool state);
	bool RunningStatus() const;

	void SetTemperatureScale(char scale);
	const char TemperatureScale() const;
public:
	bool fStandaloneMode;
private:
	BStringList fDevicesList;

	BRect fWindowRect;
	BString fActiveDevice;
	uint32 fSecondsToRefresh;
	bool fGraphWatermarkShown;
	rgb_color fGraphLineColor;
	bool fGraphRunningStatus;
	char fTemperatureScale;
};

#endif /* __DATA_FACTORY__ */
