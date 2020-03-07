#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "driver/gpio.h"

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
#include "./periferals/wifisetup.c"

//i2c and periferal requirements
#include "driver/i2c.h"
#define I2C_SDA_IO    5
#define I2C_SCL_IO    4
#include "./periferals/i2c.c"
#include "./periferals/ssd1306.c"

//tcp_server_task and globals declare
//does server setup and waits for http_request
//    index.html - returns webpage
//    GetData GET - reads http post into rx_buffer
//                  and returns outstr
char outstr[6096];      
char rx_buffer[1024];  
#include "./tasks/tcp_server_task.c"
    
#include "./tasks/lipo_control_task.c"
#include "./tasks/buttons_task.c"

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();  //in wifisetup.h
    wait_for_ip();      //in wifisetup.h

    i2c_init();
    i2c_detect();       //in i2c.h
    ssd1305_init();     //in i2c.h
    ssd1305_blank(0xcc);//in i2c.h
    vTaskDelay(50);

    char disp_str[128] = "4   Li-Ion|||4  Battery||4  Charger||1  Any SWx for MENU";
    ssd1305_text(disp_str);

    //start tcp server and data collection tasks
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 4, NULL);          //seperate .h
    xTaskCreate(lipo_control_task, "lipo_control_task", 8192, NULL, 5, NULL); //in top level .c
    xTaskCreate(buttons_task, "buttons_task", 8192, NULL, 6, NULL);

}

