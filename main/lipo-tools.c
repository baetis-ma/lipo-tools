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

//tcp_server_task and globals declare
//does server setup and waits for http_request
//    index.html - returns webpage
//    GetData GET - reads http post into rx_buffer
//                  and returns outstr
char outstr[4096];      
char rx_buffer[1024];  
#include "../components/tcp_server_task.h"
    
#define maxlen  256
static void lipo_control_task () {
   int len = 0;
   int samples = 0;
   int samples_per = 1;
   //from web page control interface
   int lipo_charge = 0;
   int lipo_discharge = 0;
   int lipo_38 = 0;
   float lipo_current = 0.00;
   float lipo_loadr = 0.00;
   //outputs to webpage
   int relay1, relay2, relay3, relay4;
   float volt_meas1[maxlen] = { 0 };
   float volt_meas2[maxlen] = { 0 };
   float volt_meas3[maxlen] = { 0 };
   float total_charge = 0;
   float adc1, adc2, adc3, adc4;
   int aout = 0x50;

   char tmp[64];
   uint8_t tmp_str[2];
   char *temp;
   int lipo_strobe = 0;
   int lipo_samplecnt = 0;
   int lipo_temp;
   float lipo_ftemp;
   float source_follower = 0.51;
   float propgain = 0.8;
   int errprop;
   while (1) {
       //read rx_data and parse commands - command iand state interface to webpage 
       //printf("rx_buffer==>%s\n", rx_buffer);
       temp = strstr(rx_buffer, "strobe=");  
          if(temp){sscanf(temp,"strobe=%d", &lipo_temp);
             if(lipo_temp >= 0) lipo_strobe = lipo_temp;}
       temp = strstr(rx_buffer, "samplecnt=");  
          if(temp){sscanf(temp,"samplecnt=%f", &lipo_ftemp);
             lipo_samplecnt = lipo_ftemp;}
       temp = strstr(rx_buffer, "charge=");  
          if(temp){sscanf(temp,"charge=%d", &lipo_temp);
             if(lipo_strobe == 1) lipo_charge = lipo_temp;}
       temp = strstr(rx_buffer, "dischg=");  
          if(temp){sscanf(temp,"dischg=%d", &lipo_temp);
             if(lipo_strobe == 1) lipo_discharge = lipo_temp;}
       temp = strstr(rx_buffer, "dischg38=");  
          if(temp){sscanf(temp,"dischg38=%d", &lipo_temp);
             if(lipo_strobe == 1) lipo_38 = lipo_temp;}
       temp = strstr(rx_buffer, "current=");  
          if(temp){sscanf(temp,"current=%f", &lipo_ftemp);
             if(lipo_samplecnt > 0) lipo_current = lipo_ftemp;}
       temp = strstr(rx_buffer, "loadr=");  
          if(temp){sscanf(temp,"loadr=%f", &lipo_ftemp);
             if(lipo_samplecnt > 0) lipo_loadr = lipo_ftemp; }
       lipo_strobe = 0;
       //printf("cmp=%3d curr=%5.2f  lres=%5.3f\n", samples, lipo_current, lipo_loadr);

       //collect data and populate graph data frame, manage system
       relay2 = 0; 
       if (lipo_charge == 1)relay1 = 1; else relay1 = 0;
       if (lipo_discharge == 1)relay4 = 1; else relay4 = 0;
       if (lipo_38 == 1)relay3 = 1; else relay3 = 0;

       //collect data from adcs
       tmp_str[0] = 0x40; tmp_str[1] = 0x00;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       adc1 = 0.00059 * (256 * tmp_str[0] + tmp_str[1]);
       if (adc1 > 20) adc1 = 0.00;

       tmp_str[0] = 0x50; tmp_str[1] = 0x00;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       adc2 = 0.00059 * (256 * tmp_str[0] + tmp_str[1]);
       if (adc2 > 20) adc2 = 0.00;

       tmp_str[0] = 0x60; tmp_str[1] = 0x00;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       adc3 = 0.00059 * (256 * tmp_str[0] + tmp_str[1]);
       if (adc3 > 20) adc3 = 0.00;

       tmp_str[0] = 0x70; tmp_str[1] = 0x00;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       adc4 = 0.000189 * (256 * tmp_str[0] + tmp_str[1]);
       if (adc4 > 6) adc4 = 0.00;

       //programable current source
       errprop = (int) (propgain * (source_follower / 0.02) * 
                        (lipo_current - adc4 / source_follower));
       aout = aout + errprop;
       if ( aout + errprop > 0xff ) aout = 0xff;
       if ( aout + errprop < 0x00 ) aout = 0x00;
       //pcf 8591 - no regaddr just cntreg iin i2caddr field - 0x40 turns on dac out
       //         - dacout x00-xff - about 20mV per
       tmp_str[0] = aout;
       i2c_write_block (0x48, 0x40, tmp_str, 1); 

    printf(" %5.3f %5.3f %5.3f %5.3f    dac = 0x%02x\n", adc1, adc2, adc3, adc4, aout);
    //printf ("errprop=%4d adc4=%5.3f  progI = %5.3f sourceI = %5.3f gateV = %5.3f  aout =  0x%02x\n", 
    //             errprop, adc4, lipo_current, adc4 / source_follower, adc3, aout); 

       samples++;
       if ( samples % samples_per == 0){
           volt_meas1[len] = adc2 - adc3;
           volt_meas2[len] = adc3 - adc4;
           volt_meas3[len] = adc4;
           len++;
       }
       total_charge = total_charge + 1 * lipo_current / 3600;
       //total_charge = total_charge + 1 * (adc2 / 20) / 3600;

       //if (graph at max samples) average data into first half of space and 
       //   in future take half as many samples - iterative
       if ( len == maxlen ){
          samples_per = 2 * samples_per;
          for (int x = 0; x < maxlen / 2; x++){
              volt_meas1[x] = (volt_meas1[2 * x] + volt_meas1[2 * x + 1]) / 2;  
              volt_meas2[x] = (volt_meas2[2 * x] + volt_meas2[2 * x + 1]) / 2;  
              volt_meas3[x] = (volt_meas3[2 * x] + volt_meas3[2 * x + 1]) / 2;  
          }
          len = maxlen / 2;
       }

       //construct outstr - outstr is gloabal that will be forwarded by tcp_server_task
       snprintf( outstr, sizeof tmp,"%d,%d,%d,%d,%5.3f,", samples, maxlen, len, len * samples_per, total_charge);
       snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,", lipo_current, lipo_loadr); strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%d,%d,%d,", lipo_charge, lipo_discharge, lipo_38); strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,%4.2f,", adc1, adc2, adc3, adc4); strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%d,%d,%d,%d,", relay1, relay2, relay3, relay4); strcat (outstr, tmp);  
       for (int a = 0; a < maxlen; a++) {
            snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,", volt_meas1[a], volt_meas2[a], volt_meas3[a]); 
            strcat (outstr, tmp);  
       }
       strcat ( outstr, "EOF\0");
       //printf("outstr   ==>%s\n", outstr);
       vTaskDelay(100);
   }
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();  //in wifisetup.h
    wait_for_ip();      //in wifisetup.h
    i2cdetect();        //in i2c.h
    //ssd1305_init();     //in i2c.h

    //start tcp server and data collection tasks
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);          //seperate .h
    xTaskCreate(lipo_control_task, "lipo_control_task", 8192, NULL, 5, NULL); //in top level .c

}

