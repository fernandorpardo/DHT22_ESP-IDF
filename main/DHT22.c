/**

	DHT22 driver
	Fernando R
	www.iambobot.com
	
	interrupts and ESP IDF Timer (High Resolution Timer) 
	
	Data Sheet
	http://www.adafruit.com/datasheets/DHT22.pdf

	version
	1.0		January 2025
**/

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "DHT22.h"

#define	_DEBUG_		0
#define	_VERBOSE_	1

static const char *TAG = "DHT22";

static volatile unsigned int isr_count;
static volatile int64_t edge_times[64];

static void IRAM_ATTR gpio_DHT_isr_handler(void* arg)
{
	// record time
	if(isr_count < 64) edge_times[isr_count]= esp_timer_get_time();
	// count pulses
	isr_count ++;
} // gpio_DHT_isr_handler()

#define ESP_INTR_FLAG_DEFAULT 0

static gpio_num_t gpio_number_DHT22;

void DHTinit(gpio_num_t gpio_num)
{
	gpio_number_DHT22= gpio_num;
	// GPIO INPUT 
	gpio_config_t io_conf_in = {};
    //interrupt
	//io_conf_in.intr_type = GPIO_INTR_ANYEDGE;	//  both rising and falling edge
	io_conf_in.intr_type = GPIO_INTR_NEGEDGE; // falling edge
    //bit mask of the pins, use GPIO4/5 here
    io_conf_in.pin_bit_mask = (1ULL<<gpio_num);
    //set as input mode
    io_conf_in.mode = GPIO_MODE_INPUT_OUTPUT_OD;
    //disable pull-down mode
    io_conf_in.pull_down_en = 0;	
    //disable pull-up mode
	//use a extenal pull-up of 10kOhm
    io_conf_in.pull_up_en = 0;
    gpio_config(&io_conf_in);
	
    //install gpio isr service
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
    //hook isr handler for specific gpio pin
    ESP_ERROR_CHECK(gpio_isr_handler_add(gpio_num, gpio_DHT_isr_handler, (void*) gpio_num));	
	
	// Create ESP Timer
	ESP_ERROR_CHECK(esp_timer_early_init());
	
	// leave the output IDLE (open-drain pulled up to VCC)
	gpio_set_level(gpio_number_DHT22, 1);

} // DHTinit()

/* void printbinary(uint8_t b)
{
	for(uint8_t mask=0x80; mask; mask=mask>>1) printf((b & mask)?"1":"0");
}
 */
#define EDGE_THRESHOLD_MICROSECONDS	110L

int DHTget(uint16_t* temperature, uint16_t* humidity)
{
	*temperature= 0;
	*humidity= 0;

	// ---------------------------------------------------------------------------
	// ESP32 drives the DATA wire
	// ESP32 sends start signal
	// ESP32 requests data by pulling low the bus for 500us and then up for 20-40 us 
	//           500us         20-40us   
	// ___               _______________ - - - -
	//    \             /    
	//     \___________/                  DHT22 start transmission
	// 
	//    1 down + 1 up             

	// (1) pull low data-bus ...
	gpio_set_level( gpio_number_DHT22, 0 );
	// ... for at least 500us. The value varies with the sources of the information
	// By experience less than 10ms does not work. Probably is a dependence with the HW version. Increase this value if you get no response
	vTaskDelay( 12 / portTICK_PERIOD_MS ); 
	// (2) ESP32 pulls up voltage and wait 20-40us for DHT22's response
	gpio_set_level( gpio_number_DHT22, 1 );
	// Wait 30us
	// esp_rom_delay_us is part of the "Internal and Unstable APIs", but don't know another way to wait micro seconds
//	esp_rom_delay_us( 30 );
	// we better avoid the call above, so we should get a count of 42= 1 for the end of the start / beginning of ready + 1 for the ready + 40 data bits
	isr_count= 0;
	// DHT22 now drives the DATA wire
	// In response to the start signal, the DHT22 sends 41 down+up pulses
	// first pulse means "ready" and is followed by 40 pulses meaning data 0 or 1
	// 41 pulses are genearted in total
	// bit 0 or 1 is discriminated by the time between two consecutive falling edges

	// bit data 0
	//                         26-28us   
	//                   ____________
	//    \             /    
	//     \___________/                 

	// bit data 1
	//                              70us   
	//                   ______________________ 
	//    \             /    
	//     \___________/                 

	// give time to the ISR to do its part of the job
	// leave free the CPU for other taks using a preemptive function
	// it should not take longer than 200us x 41 < 10ms
	vTaskDelay( 10 / portTICK_PERIOD_MS );
	if(_VERBOSE_) ESP_LOGI(TAG,"count %d", isr_count);
	// check we got all the pulses
	if(isr_count >= 42 )
	{
		if(isr_count != 42) ESP_LOGE(TAG, "WARNING pulse count %d != 42", isr_count);
		// Convert pulse lengh (time) to bits
		uint8_t mask= 0x80;
		uint8_t data[6];
		int j=0;
		data[j] =0; 
		// take the last 40 time records, i.e., the data bits
		for (unsigned int i=(isr_count - 40); i<isr_count; i++)
//		for (unsigned int i=1; i<41; i++)
		{
			int64_t pulse_width_us= edge_times[i] - edge_times[i-1];
			if(_DEBUG_) printf("%2d: %3lld us  %d\n", i, pulse_width_us, (pulse_width_us > EDGE_THRESHOLD_MICROSECONDS)? 1 : 0);

			// down pulse is around 55-65us then time between 2  falling edges is
			//   80-93 us for data 0
			//   125-135 us for data 1
			// A safe decission threshold is 110us
			if(pulse_width_us > EDGE_THRESHOLD_MICROSECONDS) data[j]= data[j] | mask;
			mask = mask >> 1;	// next bit
			if(mask==0) 		// next byte
			{
				mask= 0x80; 
				data[++j]=0;
			}
		}
		if(_DEBUG_) for(int k= 0; k<5; k++) 
		{
			printf("%d: ", k);
			//printbinary(data[k]);
			for(uint8_t maskb=0x80; maskb; maskb=maskb>>1) printf((data[k] & maskb)?"1":"0");
			printf("\n");
		}
		uint8_t checksum= (data[0] + data[1] + data[2] + data[3]);
		if(_DEBUG_) printf("checksum %d %d\n",  checksum, data[4]);
		if(checksum != data[4])
		{
			if(_VERBOSE_) ESP_LOGE(TAG, "WRONG CHECKSUM");
			return -1;
		}
		*humidity= ((uint16_t)data[0] << 8) + (uint16_t)data[1];
		*temperature= ((uint16_t)data[2] << 8) + (uint16_t)data[3];
		if(_VERBOSE_) ESP_LOGI(TAG, "temperature %d",  *temperature);
		if(_VERBOSE_) ESP_LOGI(TAG, "humidity    %d",  *humidity);
	}
	else 
	{
		if(_VERBOSE_) ESP_LOGE(TAG, "Wrong pulse count %d != 41", isr_count);
		return -1;
	}
	fflush(stdout);
	return 0;
} // DHTget()


// END OF FILE