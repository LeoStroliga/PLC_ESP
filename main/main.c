#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"


// Input GPIOs (connected to optocoupler outputs)
#define OPTO_X20 GPIO_NUM_26    // gornji senzor
#define OPTO_X21 GPIO_NUM_27    // donji senzor
#define OPTO_X18 GPIO_NUM_32    // desni senzor
#define OPTO_X19 GPIO_NUM_33    // livi senzor

// Output GPIOs (connected to relays)
#define OUTPUT_X7 GPIO_NUM_4   // spušta glavu
#define OUTPUT_X6 GPIO_NUM_5   // diže glavu

// Pulse duration in milliseconds
#define PULSE_DURATION_MS 400

// Direction flag
static int direction = 1; //1 = up, 0 = down


// Function to cahnge direction ensuring it doesn't change direction twice reverting itself
static void change_direction (int* pins){
    
    if (pins[0] == 1) {
                    //printf("Mijenjaj smjer → dolje\n");
                    direction = 0;
                    gpio_set_level(OUTPUT_X6, 1);
    } else if (pins[1] == 1) {
                   // printf("Mijenjaj smjer → gore\n");
                    direction = 1;
                    gpio_set_level(OUTPUT_X6, 0);
                }
   
    vTaskDelay(1000); //Delay to ensure change doesn't happen twice during sensor input change
    vTaskDelete(NULL);
}

static void input_simulation_task(void *arg) {
    int last_state[4] = {1, 1, 1, 1};
     int pins[] = {
            gpio_get_level(OPTO_X20),
            gpio_get_level(OPTO_X21),
            gpio_get_level(OPTO_X18),
            gpio_get_level(OPTO_X19)
        };
 
    while (1) {

        pins[0] = gpio_get_level(OPTO_X20);
        pins[1] =  gpio_get_level(OPTO_X21);
        pins[2] =  gpio_get_level(OPTO_X18),
        pins[3] =  gpio_get_level(OPTO_X19);
           
        

        for (int i = 0; i < 4; i++) {
            if (pins[i] != last_state[i]) {
                last_state[i] = pins[i];
                //ESP_LOGI("OPTO_IN", "Input %d changed to %s", i + 1, pins[i] ? "HIGH" : "LOW");
               
                if (i == 0 || i == 1){
                    change_direction(pins);
                }

                if ((i == 2 || i == 3) && pins[i] == 1) {
                    if (direction == 0) {       //Case when it goes down esp simulates input each time separately
                      //  printf("Idi dolje\n");
                      gpio_set_level(OUTPUT_X6, 1);
                        gpio_set_level(OUTPUT_X7, 0);
                        vTaskDelay(pdMS_TO_TICKS(PULSE_DURATION_MS));
                        gpio_set_level(OUTPUT_X7, 1);
                    } else {                    //Case when it goes up it simulates input all the time, continuously
                       // printf("Idi gore\n");
                        gpio_set_level(OUTPUT_X7, 1);
                        gpio_set_level(OUTPUT_X6, 0);
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100)); // Pause interval
    }
}

void app_main(void) {

    
    // Configure output pins
    gpio_config_t output_conf = {
        .pin_bit_mask = (1ULL << OUTPUT_X6) | (1ULL << OUTPUT_X7),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&output_conf);

    // Output starts upward
    gpio_set_level(OUTPUT_X6, 0);
    gpio_set_level(OUTPUT_X7, 1);
    

    // Configure input pins
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << OPTO_X20) |
                        (1ULL << OPTO_X21) |
                        (1ULL << OPTO_X18) |
                        (1ULL << OPTO_X19),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&input_conf);

    // Start the main input simulation task
    xTaskCreate(input_simulation_task, "input_simulation_task", 2048, NULL, 10, NULL);
}
