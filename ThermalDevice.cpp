/*
 * Copyright 2025, cafeina, <cafeina@world>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <DataIO.h>
#include <File.h>
#include <String.h>
#include <StringList.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include "ThermalDevice.h"

ThermalDevice::ThermalDevice(const char* path)
: fPath(""),
  fFD(-1)
{
	if(path)
		SetTo(path);
}

ThermalDevice::~ThermalDevice()
{
	Unset();
}

status_t
ThermalDevice::InitCheck() const
{
	return fFD < 0 ? B_NO_INIT : B_OK;
}

status_t
ThermalDevice::SetTo(const char* path)
{
	if(!path)
		return B_BAD_VALUE;

	fPath.SetTo(path);
	fFD = open(fPath, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if(fFD < 0) {
		fprintf(stderr, "Error: device file \'%s\' could not be POSIX-opened.\n", fPath.String());
		Unset();
		return B_ERROR;
	}

	return B_OK;
}

void
ThermalDevice::Unset()
{
	if(fFD >= 0) {
		close(fFD);
		fFD = -1;
	}

	fPath.SetTo("");
}

const char*
ThermalDevice::Location() const
{
	return fPath.String();
}

float
ThermalDevice::ReadTemperature(DeviceTemperature which)
{
	float temperature = 0.0f;
	BObjectList<float> temperatures;
	for(auto i = 0; i < TEMPERATURE_COUNT; i++)
		temperatures.AddItem(&temperature);

	switch(which)
	{
		case TEMPERATURE_CURRENT:
		case TEMPERATURE_CRITICAL:
		case TEMPERATURE_HOT:
		{
			BMallocIO alloc;
			ReadDevice(&alloc);
			BMemoryIO memory((const void*)alloc.Buffer(), alloc.BufferLength());
			ParseData(&memory, &temperatures);
			temperature = *temperatures.ItemAt(which);
			break;
		}
		default:
			break;
	}
	return temperature;
}

bool
ThermalDevice::IsTemperatureReported(DeviceTemperature which)
{
	if(InitCheck() != B_OK)
		return false;

	BMallocIO alloc;
	ReadDevice(&alloc);
	bool result = false;
	switch(which)
	{
		case TEMPERATURE_CURRENT:
			result = strstr(reinterpret_cast<const char*>(alloc.Buffer()), "Current") != NULL;
			break;
		case TEMPERATURE_CRITICAL:
			result = strstr(reinterpret_cast<const char*>(alloc.Buffer()), "Critical") != NULL;
			break;
		case TEMPERATURE_HOT:
			result = strstr(reinterpret_cast<const char*>(alloc.Buffer()), "Hot") != NULL;
			break;
		default:
			break;
	}

	return result;
}

void
ThermalDevice::PrintToStream()
{
	printf("Thermal device: %s\n", InitCheck() != B_OK ? "not initialized" : Location());
	if(InitCheck() == B_OK) {
		printf(
				"Reports temperatures:\n"
				"\t[%s]  Current temperature\n"
				"\t[%s]  Critical temperature\n"
				"\t[%s]  Hot temperature\n",
				IsTemperatureReported(TEMPERATURE_CURRENT)  ? "X" : " ",
				IsTemperatureReported(TEMPERATURE_CRITICAL) ? "X" : " ",
				IsTemperatureReported(TEMPERATURE_HOT)      ? "X" : " ");
		printf("Values at the time of report:\n");
		printf("\tCurrent: ");
		if(IsTemperatureReported(TEMPERATURE_CURRENT))
			printf("%.2f\n", ReadTemperature(TEMPERATURE_CURRENT));
		else
			printf("not reported\n");
		printf("\tCritical: ");
		if(IsTemperatureReported(TEMPERATURE_CRITICAL))
			printf("%.2f\n", ReadTemperature(TEMPERATURE_CRITICAL));
		else
			printf("not reported\n");
		printf("\tHot: ");
		if(IsTemperatureReported(TEMPERATURE_HOT))
			printf("%.2f\n", ReadTemperature(TEMPERATURE_HOT));
		else
			printf("not reported\n");
	}
}

// #pragma mark - Internal

status_t
ThermalDevice::ReadDevice(BPositionIO* outData)
{
	if(InitCheck() != B_OK || !outData)
		return B_ERROR;

	BFile deviceFile(Location(), B_READ_ONLY);
	if(deviceFile.InitCheck() != B_OK)
		return B_ERROR;

	char* buffer = new char[1024];
	if(!buffer)
		return B_NO_MEMORY;

	ssize_t readBytes = deviceFile.Read((void*)buffer, 1024);
	if(readBytes < 0) {
		delete[] buffer;
		return B_IO_ERROR;
	}
	ssize_t writtenBytes = outData->Write((const void*)buffer, readBytes);
	delete[] buffer;
	return writtenBytes < 0 ? B_IO_ERROR : B_OK;
}

status_t
ThermalDevice::ParseData(BPositionIO* inData, BObjectList<float>* outList)
{
	if(!inData || !outList) {
		fprintf(stderr, "Error: invalid parameter values.\n");
		return B_BAD_VALUE;
	}

	off_t size = 0;
	inData->GetSize(&size);
	char* buffer = new char[size + 1];
	buffer[size] = '\0';
	ssize_t readBytes = inData->Read((void*)buffer, size);
	if(readBytes < 0) {
		fprintf(stderr, "Error: I/O error while reading.\n");
		return B_IO_ERROR;
	}

	BStringList reportedList;
	BString(buffer).Split("\n", true, reportedList);

	float temperature = 0.0f;
	for(int i = 0; i < reportedList.CountStrings(); i++) {
		BString item = reportedList.StringAt(i);
		assert(item != NULL);
		if(strstr(item.String(), "Current") != NULL) {
			item.ScanWithFormat("  Current Temperature: %f C\n", &temperature);
			*outList->ItemAt(TEMPERATURE_CURRENT) = temperature;
		}
		else if(strstr(item.String(), "Critical") != NULL) {
			item.ScanWithFormat("  Critical Temperature: %f C\n", &temperature);
			*outList->ItemAt(TEMPERATURE_CRITICAL) = temperature;
		}
		else if(strstr(item.String(), "Hot") != NULL) {
			item.ScanWithFormat("  Hot Temperature: %f C\n", &temperature);
			*outList->ItemAt(TEMPERATURE_HOT) = temperature;
		}
	}

	delete[] buffer;
	return B_OK;
}
