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
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <TextControl.h>


//[…]

//Cambia esto siempre
//const char *kTemperaturePath = "/dev/power/acpi_thermal/0";
//por esto:
#ifndef KTEMPERATUREPATH_DEFINED
#define KTEMPERATUREPATH_DEFINED
extern const char *kTemperaturePath;
#endif




class MainWindow : public BWindow
{
public:
			MainWindow(void);
			void		MessageReceived(BMessage *msg);
			bool		QuitRequested(void);
			BStringView *fLabel;
  			BButton *fButton;
private:
  bool ReadTemperature(char *temperature, size_t size)
   {
    int fd = open(kTemperaturePath, O_RDONLY);
    if (fd < 0) {
      return false;
    }
    ssize_t len = read(fd, temperature, size - 1);
    close(fd);
    if (len < 0) {
      return false;
    }
    temperature[len] = '\0';
    return true;
  }
			BMenuBar	*fMenuBar;
			BMessage	*update;
			BTextControl *criticalTempControl;
			BTextControl *currentTempControl;
			BButton *celsiusButton;
			BButton *kelvinButton;
			int showCelsius;
		
};

#endif
