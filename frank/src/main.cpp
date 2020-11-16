#include "frank.h"
#include <arduino.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char* TAG = "main";



// trackMove provides structure for controling track behavior, setting a demand for each track and a duration for the action in ms
struct trackMove{
    int demandLeft = 0;
    int demandRight = 0;
    int durationMs = 0;
};

QueueHandle_t xStructTracKMoveQueue = NULL;

void moveTracks(void * params){
    
}


void setup(void) {
    Serial.begin(115200);
  
    ESP_LOGI( TAG, "Creating Queues", NULL);
    xStructTracKMoveQueue = xQueueCreate(
        5,
        sizeof(trackMove),
         )



}

void loop(){

}