
#define maxlen  128
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
   float volt_meas1[maxlen] = { 0 };
   float volt_meas2[maxlen] = { 0 };
   float volt_meas3[maxlen] = { 0 };
   float volt_meas4[maxlen] = { 0 };
   float volt_meas5[maxlen] = { 0 };
   float volt_meas6[maxlen] = { 0 };
   float volt_measr[maxlen] = { 0 };
   float volt_measd[maxlen] = { 0 };
   float total_charge = 0;
   float adc1=0, adc2=0, adc3=0, adc4=0, adc5=0, adc6=0, vrload=0, vdrain=0;

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

       //collect data from adcs
       float temp_float;
       tmp_str[0] = 0x40; tmp_str[1] = 0x80;
       i2c_write_block (0x48, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       temp_float = 0.001505 * (256 * tmp_str[0] + tmp_str[1]);
       if (temp_float < 30) adc5 = temp_float;

       tmp_str[0] = 0x50; tmp_str[1] = 0x80;
       i2c_write_block (0x48, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       temp_float = 0.00149 * (256 * tmp_str[0] + tmp_str[1]);
       if (temp_float < 30) adc6 = temp_float;

       tmp_str[0] = 0x60; tmp_str[1] = 0x80;
       i2c_write_block (0x48, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       if (tmp_str[0] < 0x80)temp_float = -0.000549 * (256 * tmp_str[0] + tmp_str[1]);
                   else temp_float = -0.000549 * (0xffff-(256*tmp_str[0] + tmp_str[1]));
       if (temp_float < 30) vdrain = temp_float;

       tmp_str[0] = 0x70; tmp_str[1] = 0x80;
       i2c_write_block (0x48, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       i2c_read (0x48, 0x00, tmp_str, 2);
       if (tmp_str[0] < 0x80)temp_float = -0.0001225 * (256 * tmp_str[0] + tmp_str[1]);
                   else temp_float = 0.0001225 * (0xffff-(256*tmp_str[0] + tmp_str[1]));
       if (temp_float < 30) vrload = temp_float;

       tmp_str[0] = 0x40; tmp_str[1] = 0x80;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       temp_float = 0.000577 * (256 * tmp_str[0] + tmp_str[1]);
       if (temp_float < 30) adc1 = temp_float;

       tmp_str[0] = 0x50; tmp_str[1] = 0x80;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       temp_float = 0.00057 * (256 * tmp_str[0] + tmp_str[1]);
       if (temp_float < 30) adc2 = temp_float;

       tmp_str[0] = 0x60; tmp_str[1] = 0x80;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       temp_float = 0.000889 * (256 * tmp_str[0] + tmp_str[1]);
       if (temp_float < 30) adc3 = temp_float;

       tmp_str[0] = 0x70; tmp_str[1] = 0x80;
       i2c_write_block (0x49, 0x01, tmp_str, 2);
       vTaskDelay(2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       i2c_read (0x49, 0x00, tmp_str, 2);
       temp_float = 0.000889  * (256 * tmp_str[0] + tmp_str[1]);
       if (temp_float < 30) adc4 = temp_float;
       //printf("1 0x%04x\n", (256 * tmp_str[0] + tmp_str[1]));

       //programable current source
       errprop = (int) (propgain * (lipo_current - vrload / source_follower));
       aout = aout + errprop;
       if ( aout + errprop > 0xff ) aout = 0xff;
       if ( aout + errprop < 0x00 ) aout = 0x00;
       //pcf 8591 - no regaddr just cntreg iin i2caddr field - 0x40 turns on dac out
       //         - dacout x00-xff - about 20mV per
       tmp_str[0] = aout;
       i2c_write_block (0x48, 0x40, tmp_str, 1); 

    printf(" %6.2f %6.2f %6.2f %6.2f  %6.2f %6.2f %6.2f %6.2f   dac = 0x%02x\n", adc6, adc5, adc4, adc3, adc2, adc1, vrload, vdrain, aout);
    //printf ("errprop=%4d adc4=%5.3f  progI = %5.3f sourceI = %5.3f gateV = %5.3f  aout =  0x%02x\n", 
    //             errprop, adc4, lipo_current, adc4 / source_follower, adc3, aout); 

       samples++;
       if ( samples % samples_per == 0){
           volt_meas1[len] = adc1 - vdrain;
           volt_meas2[len] = adc2 - adc1;
           volt_meas3[len] = adc3 - adc2;
           volt_meas4[len] = adc4 - adc3;
           volt_meas5[len] = adc5 - adc4;
           volt_meas6[len] = adc6 - adc5;
           volt_measr[len] = vrload;
           volt_measd[len] = vdrain;
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
              volt_meas4[x] = (volt_meas4[2 * x] + volt_meas4[2 * x + 1]) / 2;  
              volt_meas5[x] = (volt_meas5[2 * x] + volt_meas5[2 * x + 1]) / 2;  
              volt_meas6[x] = (volt_meas6[2 * x] + volt_meas6[2 * x + 1]) / 2;  
          }
          len = maxlen / 2;
       }

       //construct outstr - outstr is gloabal that will be forwarded by tcp_server_task
       snprintf( outstr, sizeof tmp,"%d,%d,%d,%d,%5.3f,", samples, maxlen, len, len * samples_per, total_charge);
       snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,", lipo_current, lipo_loadr); strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%d,%d,%d,", lipo_charge, lipo_discharge, lipo_38); strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,%4.2f,", adc1, adc2, adc3, adc4); strcat (outstr, tmp);  
       snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,%4.2f,", adc5, adc6, vrload, vdrain); strcat (outstr, tmp);  
       for (int a = 0; a < maxlen; a++) {
            snprintf( tmp, sizeof tmp,"%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,%4.2f,", 
                      volt_meas1[a], volt_meas2[a], volt_meas3[a], volt_meas4[a], 
                      volt_meas5[a], volt_meas6[a], volt_measr[a], volt_measd[a]); 
            strcat (outstr, tmp);  
       }
       strcat ( outstr, "EOF\0");
       //printf("outstr   ==>%s\n", outstr);
       vTaskDelay(100);
   }
}

