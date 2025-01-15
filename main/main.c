/**

	DHT22 driver

To generate the code 

. $HOME/esp/esp-idf/export.sh
cd ~/esp/DHT22

idf.py set-target esp32

idf.py menuconfig

idf.py build

Execute
idf.py -p /dev/ttyUSB1 flash monitor

**/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "DHT22.h"

#define	PIN_GPIO_DHT22 25

void DHT_task(void *pvParameter)
{
	uint16_t temperature, humidity;
	
	// First driver call
	DHTinit(PIN_GPIO_DHT22);
	
	vTaskDelay( 1000 / portTICK_PERIOD_MS );
	printf( "Starting DHT Task\n\n");

	while(1) {
	
		printf("\nGetting data ...\n" );
		TickType_t T0= xTaskGetTickCount();
		// Second driver call
		int r= DHTget(&temperature, &humidity);
		TickType_t T1 = xTaskGetTickCount();
		if(r < 0)
			printf("ERROR: DHT22 get failed\n");
		else
		{
			printf("Temperature %d.%dºC\n", temperature/10, temperature % 10 );
			printf("Humidity    %d.%d%%\n", humidity/10, humidity % 10 );
		}
		printf("elapsed time: %ld ms\n", (T1-T0) * portTICK_PERIOD_MS);	
		// No more than 0.5 Hz sampling rate (once every 2 seconds)
		// wait 5 seconds to repeat
		vTaskDelay( 5000 / portTICK_PERIOD_MS );
	}
}

void app_main()
{
	xTaskCreate( &DHT_task, "DHT_task", 2048, NULL, 5, NULL );
}

// END OF FILE