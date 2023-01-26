#include "App.h"
#include "MainWindow.h"

const char *kTemperaturePath = "/dev/power/acpi_thermal/0";

App::App(void)
	:	BApplication("application/x-vnd.Loa-Temperature")
{
	MainWindow *mainwin = new MainWindow();
	mainwin->Show();
}


int main(int argc, char **argv)
{
	App *app = new App();
	app->Run();
	delete app;
	return 0;
}
