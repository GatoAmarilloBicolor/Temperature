#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Window.h>
#include <MenuBar.h>
#include <StringView.h>

#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Button.h>
#include <String.h>
#include <TextControl.h>
#include <atomic>

#include "DataFactory.h"
#include "GraphView.h"
#include "ThermalDevice.h"

class MainWindow : public BWindow
{
public:
						MainWindow(BRect frame, DataFactory* dataRepo);
						~MainWindow();

			void		MessageReceived(BMessage *msg);
			bool		QuitRequested(void);

			void		DeviceChanged(const char* devicePath);
	static	int32 		CallUpdateTemperature(void* data);
			void		Update();
	static	void		CallNotifyStopped(void* data);

	static  status_t    CallRestoreSettingsUI(void* data);
			void		RestoreSettingsUI(BMessage *data);

			bool		RunningStatus() const { return !shouldStopUpdater; }
			void		SetRunningStatus(bool shouldRun)
							{ shouldStopUpdater = !shouldRun; }

			bool		HasDevice()  const;
private:
		DataFactory*	dataRepository;
		ThermalDevice 	activeDevice;

		GraphView*		temperatureGraph;
		BMenuField*		devicesField;
		BMenuField*		temperatureField;
		BTextControl*	criticalTempControl;
		BTextControl*	currentTempControl;

		thread_id		tempUpdaterThread;
		std::atomic<bool> shouldStopUpdater;
};

#endif
