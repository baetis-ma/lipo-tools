static void tcp_server_task(void *pvParameters)
{
    char addr_str[16];
    int addr_family;
    int ip_protocol;
    char *temp;

    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
    listen(listen_sock, 1);

    struct sockaddr_in sourceAddr;
    uint addrLen = sizeof(sourceAddr);
    while (1) {
        int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
        if (sock < 0) { ESP_LOGE("", "Unable to accept connection: errno %d", errno); break; }
        //ESP_LOGI("", "Socket accepted");
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        // Error occured during receiving
        if (len <= 0) { ESP_LOGE("", "recv failed: errno %d", errno); break; }
        else {
            //prints client source ip and http request packet to esp monitor
            inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, 
                          addr_str, sizeof(addr_str) - 1);
            //ESP_LOGI("---  ", "Received %d bytes from %s:", len, addr_str);
            //ESP_LOGI("", "HTTP REQUEST Packet\n%s", rx_buffer);
            //parse request
            char http_type[8];
            char temp_str[16];
            sscanf(rx_buffer, "%s/", http_type);
            //ESP_LOGI("", "request type %s  ", http_type);
            temp = strstr(rx_buffer, "?");    
            if(temp){
               memcpy(temp_str, rx_buffer+strlen(http_type)+2, 
                                strlen(rx_buffer)-strlen(temp));
               temp_str[strlen(temp_str)] = '\0';
            } else {
               sscanf(rx_buffer+strlen(http_type)+2, "%s", temp_str);
            }
            //return html page to socket
            if (strcmp("index.html", temp_str) ==0 || strcmp("HTTP/1.1",temp_str) ==0 ){
                extern const char index_html_start[] asm("_binary_index_html_start");
                extern const char index_html_end[] asm("_binary_index_html_end");
                int pkt_buf_size = 1500;
                int pkt_end = pkt_buf_size;
                int html_len =  strlen(index_html_start) - strlen(index_html_end);
                for( int pkt_ptr = 0; pkt_ptr < html_len; pkt_ptr = pkt_ptr + pkt_buf_size){
                    if ((html_len - pkt_ptr) < pkt_buf_size) pkt_end = html_len - pkt_ptr;
                        //ESP_LOGI("", "pkt_ptr %d pkt_end %d", pkt_ptr,pkt_end );
                        send(sock, index_html_start + pkt_ptr, pkt_end, 0);
                    }
            } else 
            //parse datagram, rcv data from webpage or return collected data
            if (strcmp("GET", http_type) ==0 || strcmp("getData",temp_str) ==0 ){
                //ESP_LOGI("","sizeof=%d  %s",strlen(outstr), outstr);
                send(sock, outstr, sizeof outstr, 0);
                
            //resource not found 
            } else {
                char temp404[] = "<html><h1>Not Found 404</html>";
                send(sock, temp404, sizeof(temp404), 0);
                //ESP_LOGI("", "404");
            }
            vTaskDelay(50/ portTICK_RATE_MS); //waits for buffer to purge
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

