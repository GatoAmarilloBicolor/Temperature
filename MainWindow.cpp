#include "MainWindow.h"

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


#include <Menu.h>
#include <MenuItem.h>

#include <StringView.h>
#include <stdio.h>


MainWindow::MainWindow(void)
	:	BWindow(BRect(100,100,500,400),"Temperature",B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BRect r(Bounds());
	r.bottom = 20;
//	fMenuBar = new BMenuBar(r,"menubar");
//	AddChild(fMenuBar);
	
	/* "Stable" */
	BView *view = new BView(Bounds(), "view", B_FOLLOW_NONE, B_WILL_DRAW);
    AddChild(view);
    view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
//
    BRect rect(10,0,280,50);
    fLabel = new BStringView(rect, "label", "Temperatura: N/A");
    view->AddChild(fLabel);

    rect.OffsetBy(0,50);
    fButton = new BButton(rect, "button", "Actualizar", new BMessage('u'));
    view->AddChild(fButton);
    
    //Experimental
    
    
    criticalTempControl = new BTextControl(BRect(10, 100, 180, 30), "critical_temp", "Critical Temperature:", NULL, NULL);
	criticalTempControl->SetModificationMessage(new BMessage(B_MODIFIERS_CHANGED));
	view->AddChild(criticalTempControl);

	currentTempControl = new BTextControl(BRect(10, 120, 180, 60), "current_temp", "Current Temperature:", NULL, NULL);
	currentTempControl->SetModificationMessage(new BMessage(B_MODIFIERS_CHANGED));
	view->AddChild(currentTempControl);

	celsiusButton = new BButton(BRect(10, 180, 90, 90), "celsius", "Celsius", new BMessage(B_MODIFIERS_CHANGED));
	view->AddChild(celsiusButton);

	kelvinButton = new BButton(BRect(100, 180, 180, 90), "kelvin", "Kelvin", new BMessage('k'));
	view->AddChild(kelvinButton);

	showCelsius = 0;
    
    
    
}


void MainWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what)
	{
		 case 'u':
        {
          char temperature[100];
          if (ReadTemperature(temperature, sizeof(temperature))) 
          {
            
            BString label("Temperatura:\n ");
            
            // SSCANF()
            float critical_temp;
		    sscanf(label, "Critical Temperature: %f K", &critical_temp);
            

            label += temperature;
            fLabel->SetText(label.String());

          }
		//experimental
        }

		case B_MODIFIERS_CHANGED:
		{
			if (msg->FindInt32("modifiers") & B_CONTROL_KEY)
			{
			// Abre el archivo de temperatura
			FILE *fp = fopen(kTemperaturePath, "r");
			if (fp == NULL)
				{
					fprintf(stderr, "Error al abrir el archivo de temperatura\n");
					break;
				}

			// Lee las dos l\u00edneas del archivo
			char line1[256];
			char line2[256];

			fgets(line1, sizeof(line1), fp);
			fgets(line2, sizeof(line2), fp);

			// Extrae los valores de temperatura de las l\u00edneas
			float critical_temp, current_temp;
			sscanf(line1, "Critical Temperature: %f K", &critical_temp);
			sscanf(line2, "Current Temperature: %f K", &current_temp);

		// Convierte las temperaturas a Celsius o Kelvin seg\u00fan sea necesario
			if (showCelsius)
			{
				critical_temp -= 273;
				current_temp -= current_temp-273;
			}
		else
			{
				critical_temp += 273;
				current_temp += 273;
			}

		// Actualiza los controles de texto con las temperaturas convertidas
		char temp_str[16];
		sprintf(temp_str, "%.1f", critical_temp);
		criticalTempControl->SetText(temp_str);
		sprintf(temp_str, "%.1f", current_temp);
		currentTempControl->SetText(temp_str);


		// Cierra el archivo
		fclose(fp);
			break;
	}
		}
	case 'k':
	{
		// Obtiene el bot\u00f3n que fue presionado
		BButton *button;
		if (msg->FindPointer("source", (void **)&button) != B_OK)
			break;

			// Cambia la bandera de Celsius/Kelvin y actualiza los botones
		if (button == celsiusButton)
			{
				showCelsius = 1;
				celsiusButton->SetValue(B_CONTROL_ON);
				kelvinButton->SetValue(B_CONTROL_OFF);
			}
		else if (button == kelvinButton)
			{
				showCelsius = 0;
				celsiusButton->SetValue(B_CONTROL_OFF);
				kelvinButton->SetValue(B_CONTROL_ON);
			}

		//Vuelve a leer el archivo de temperatura y actualiza los controles de texto
		PostMessage(new BMessage(B_MODIFIERS_CHANGED));
          
            
          }
      break;
      default:
        BWindow::MessageReceived(msg);
        break;
	
//}



	}


}

bool MainWindow::QuitRequested(void)
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
