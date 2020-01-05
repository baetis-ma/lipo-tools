#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

//wifi sockets etc requirements
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

//wifi requirements
#define EXAMPLE_WIFI_SSID "troutstream"
#define EXAMPLE_WIFI_PASS "password"
#define PORT 80
#include "esp_wifi.h"
#include "../components/wifisetup.h"

//i2c and periferal requirements
#include "driver/i2c.h"
static uint32_t i2c_frequency = 100000;
static i2c_port_t i2c_port = I2C_NUM_0;
static gpio_num_t i2c_gpio_sda = 18;
static gpio_num_t i2c_gpio_scl = 19;
#include "../components/i2c.h"
#include "../components/ssd1306.h"

//tcp_server_task and globals dec
//does server setup and waits for http_request
//    index.html - returns webpag
//    GetData GET - reads http post into rx_buffer
//                  and returns outstring
char outstr[4096];      
char rx_buffer[1024];  
#include "../components/tcp_server_task.h"
    
#define maxlen  32
static void lipo_control_task () {
   int rate_sec = 1;
   int duration_time = 0;

   //from web page control interface
   int lipo_charge = 0;
   int lipo_discharge = 0;
   int lipo_38 = 0;
   float lipo_current = 0.00;
   //outputs to webpage
   float volt_meas1[maxlen] = { 0 };
   float volt_meas2[maxlen] = { 0 };
   float volt_meas3[maxlen] = { 0 };
   float total_charge = 0;
   float adc1 = 1;
   float adc2 = 2;
   float adc3 = 3;
   float adc4 = 4;
   float aout = 3.2;
   int relay1 = 11;
   int relay2 = 12;
   int relay3 = 13;
   int relay4 = 14;

   char tmp[64];
   char *temp;
   while (1) {
       //read rx_data and parse commands
       printf("rx_buffer==>%s\n", rx_buffer);
       temp = strstr(rx_buffer, "charge=");  
         if(temp)sscanf(temp,"charge=%d", &lipo_charge);
       temp = strstr(rx_buffer, "dischg=");  
         if(temp)sscanf(temp,"dischg=%d", &lipo_discharge);
       temp = strstr(rx_buffer, "dischg38=");  
         if(temp)sscanf(temp,"dischg38=%d", &lipo_38);
       temp = strstr(rx_buffer, "current=");  
         if(temp)sscanf(temp,"current=%f", &lipo_current);
       printf ("interpeted  -  %d %d %d %f\n", lipo_charge, lipo_discharge, lipo_38, lipo_current);

       //collect data and populate volt_meas, manage system
       total_charge = total_charge + rate_sec * lipo_current / 3600;
       duration_time = duration_time + rate_sec;
       volt_meas1[1] = 1.11;
       volt_meas2[2] = 2.22;
       volt_meas3[3] = 3.33;

       //construct outstring
       snprintf( outstr, sizeof tmp,"%d,%d,%5.3f,", maxlen, duration_time, total_charge);
       
       snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,%4.2f,", adc1, adc2, adc3, adc4); 
       strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%d,%d,%d,%d,", relay1, relay2, relay3, relay4); strcat (outstr, tmp);  

       for (int a = 0; a < maxlen; a++) {
            snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,", volt_meas1[a], volt_meas2[a], volt_meas3[a]); 
            strcat (outstr, tmp);  
       }
       strcat ( outstr, "EOF\0");
       printf("outstr   ==>%s\n", outstr);
       vTaskDelay(200);
   }
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    wait_for_ip();
    //i2cdetect();
    //ssd1305_init();

    //start tcp server and data collection tasks
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);
    xTaskCreate(lipo_control_task, "lipo_control_task", 4096, NULL, 5, NULL);

}

