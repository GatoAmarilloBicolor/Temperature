/*
 * Copyright 2025, cafeina, <cafeina@world>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __THERMAL_DEVICE__
#define __THERMAL_DEVICE__

#include <DataIO.h>
#include <ObjectList.h>
#include <String.h>
#include <SupportDefs.h>

enum DeviceTemperature {
	TEMPERATURE_CURRENT  = 0,
	TEMPERATURE_CRITICAL = 1,
	TEMPERATURE_HOT      = 2,

	TEMPERATURE_COUNT,
	TEMPERATURE_INVALID  = -1
};

class ThermalDevice
{
public:
						ThermalDevice(const char* path = NULL);
	virtual 			~ThermalDevice();

	virtual	status_t 	InitCheck() const;
			status_t 	SetTo(const char* path);
			void 		Unset();
			const char* Location() const;
			float 		ReadTemperature(DeviceTemperature = TEMPERATURE_CURRENT);
			bool		IsTemperatureReported(DeviceTemperature);
	virtual void 		PrintToStream();
private:
			status_t 	ReadDevice(BPositionIO* outData);
			status_t	ParseData(BPositionIO* inData, BObjectList<float>* outList);
private:
	BString fPath;
	int fFD;
};

#endif /* __THERMAL_DEVICE__ */
