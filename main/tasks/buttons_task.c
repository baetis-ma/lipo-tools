
#define GPIO_INPUT_SW_1     13
#define GPIO_INPUT_SW_2     12
#define GPIO_INPUT_SW_3     15
#define GPIO_INPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_SW_1) | (1ULL<<GPIO_INPUT_SW_2) | (1ULL<<GPIO_INPUT_SW_3))
void buttons_task () {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    while (1) {
        printf(" %d %d %d\n", gpio_get_level(GPIO_INPUT_SW_1), gpio_get_level(GPIO_INPUT_SW_2), gpio_get_level(GPIO_INPUT_SW_3));
        vTaskDelay(100);
    }

}

