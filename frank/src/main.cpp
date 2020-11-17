#include "frank.h"
#include "credentials.h"
#include <arduino.h>
#include <WiFi.h> 
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <AWS_IOT.h>
#include <ArduinoJson.h>

static const char* TAG = "main";

// Motor IO definition
const uint8_t L298_IN1 = 19;
const uint8_t L298_IN2 = 21;
const uint8_t L298_IN3 = 22;
const uint8_t L298_IN4 = 23;
const uint8_t L298_ENA = 18;
const uint8_t L298_ENB = 5;
const uint8_t trackLeftChannel = 0;
const uint8_t trackRightChannel = 1;

const int motorFrequency = 2000;

// trackMove provides structure for controling track behavior, setting a demand for each track and a duration for the action in ms
struct trackMoveSx{
    int32_t demandLeft = 0;
    int32_t demandRight = 0;
    uint32_t durationMs = 0;
} trackMove;

QueueHandle_t xStructTracKMoveQueue = NULL;

// Lamp bar
const uint8_t lampRelay = 22;
QueueHandle_t xLampStateQueue = NULL;

// MQTT Connection
char TOPIC_NAME[] = "#";
StaticJsonDocument<512> jsonBuffer;

AWS_IOT iot;



// Controls motor tracks by processing a struct from a queue
// The struct has a duration component before the motors turn off automatically
void moveTracks(void * params){
    struct trackMoveSx xTrackMove; 
    while(1)
    {
        if( xQueueReceive( xStructTracKMoveQueue,
            &( xTrackMove ),
            portMAX_DELAY ) == pdTRUE)
        {
            ESP_LOGI(TAG, 
                "Tracks left:%i, right:%i, duration:%i",
                xTrackMove.demandLeft, 
                xTrackMove.demandRight,
                xTrackMove.durationMs);

            //Process left motor
            if(xTrackMove.demandLeft >= 0){
                digitalWrite(L298_IN1, HIGH);
                digitalWrite(L298_IN2, LOW);
                ledcWrite(trackLeftChannel, (uint32_t) xTrackMove.demandLeft );
            }
            else{
                digitalWrite(L298_IN1, LOW);
                digitalWrite(L298_IN2, HIGH);
                ledcWrite(trackLeftChannel, (uint32_t) (-1 * xTrackMove.demandLeft));
            }
            
            // Process right motor
            if(xTrackMove.demandRight >= 0){
                digitalWrite(L298_IN3, HIGH);
                digitalWrite(L298_IN4, LOW);
                ledcWrite(trackRightChannel, (uint32_t) xTrackMove.demandRight);
            }
            else{
                digitalWrite(L298_IN3, LOW);
                digitalWrite(L298_IN4, HIGH);
                ledcWrite(trackRightChannel, (uint32_t) (-1 * xTrackMove.demandRight));
            }

            vTaskDelay(xTrackMove.durationMs / portTICK_PERIOD_MS);
            ledcWrite(trackRightChannel, 0);
            ledcWrite(trackLeftChannel, 0);
        }
    }
}

void controlLamp(void * params){
    int xLampState =0;
    while(1){
        if( xQueueReceive( xLampStateQueue,
            &( xLampState ),
            portMAX_DELAY ) == pdTRUE)
        {
            int intensity = ( int ) xLampState;
            if(xLampState >= 1) digitalWrite(lampRelay, HIGH);
            else digitalWrite(lampRelay, LOW);
        }
    }
}

void iotSubActionCallback(char *topicName, int payloadLen, char *payload){
    ESP_LOGI(TAG, "message received: %s", payload);

    // load payload into json object
    DeserializationError err = deserializeJson(jsonBuffer, payload);
    if( err == DeserializationError::Ok ){
        auto message = jsonBuffer["action"].as<char*>();
        ESP_LOGI(TAG, " action is: %s", message);

        if(strcmp(message, "motion") == 0){
            auto demandLeft = jsonBuffer["parameters"]["left_demand"].as<int32_t>();
            auto demandRight = jsonBuffer["parameters"]["right_demand"].as<int32_t>();
            auto duration = jsonBuffer["parameters"]["duration"].as<uint32_t>();

            struct trackMoveSx vTrackMove;
            vTrackMove.demandLeft = demandLeft;
            vTrackMove.demandRight = demandRight;
            vTrackMove.durationMs = duration;
    
            ESP_LOGI( TAG, 
                "motion demand left: %i, right: %i, duration: %i", 
                demandLeft,
                demandRight,
                duration);

            xQueueSendToBack(xStructTracKMoveQueue, (void * ) &vTrackMove, (TickType_t) 0 );
        }
        else if(strcmp(message, "lamp") == 0){
            auto intensity = jsonBuffer["parameters"]["state"].as<int>();
            ESP_LOGI(TAG,"lamp, intensity: %i", intensity);
            xQueueSendToBack(xLampStateQueue, (void * ) &intensity, (TickType_t) 0 );
        }
    }
    else{
        ESP_LOGW(TAG, "Could not decode payload to JSONdocument");
    }
}


void setup(void) {
    Serial.begin(115200);

    //Configure Motor PWM
    ESP_LOGI( TAG, "Configuring Motor", NULL);   
    pinMode(L298_IN1, OUTPUT);
    pinMode(L298_IN2, OUTPUT);
    pinMode(L298_IN3, OUTPUT);
    pinMode(L298_IN4, OUTPUT);
    ledcAttachPin(L298_ENA, trackLeftChannel);
    ledcAttachPin(L298_ENB, trackRightChannel);
    ledcSetup(trackLeftChannel, motorFrequency, 16);
    ledcSetup(trackRightChannel, motorFrequency, 16);

    //Configure Lamp
    ESP_LOGI( TAG, "Configuring Lamp", NULL);   
    pinMode(lampRelay, OUTPUT);
    digitalWrite(lampRelay, LOW);

    //Connect to WiFi
    ESP_LOGI(TAG, "Connecting to wifi");
    WiFi.begin(ssid, pwd);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        ESP_LOGI(TAG, "Connection pending...");
    }

    ESP_LOGI(TAG, "Connected with IP: %s", WiFi.localIP());

    //Connect to broker
    ESP_LOGI( TAG, "Connecting to AWS IoT broker", NULL);
    if( iot.connect(ENDPOINT, CLIENT_ID)== 0)
    {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "Connected to AWS");
        if(0 == iot.subscribe( TOPIC_NAME, iotSubActionCallback))
        {
            ESP_LOGI( TAG, "Subscribed to %s", TOPIC_NAME);
        }
        else
        {
            ESP_LOGI( TAG, "could not subscribe to topic", NULL);
            while(1);
        }
    }
    else
    {
        Serial.println("AWS connection failed, Check the HOST Address");
        while(1);
    }


    ESP_LOGI( TAG, "Creating Queues", NULL);   
    xStructTracKMoveQueue = xQueueCreate( 5, sizeof(trackMove));
    xLampStateQueue = xQueueCreate(5, sizeof( int ));


    ESP_LOGI( TAG, "Launching tasks", NULL);   
    xTaskCreate(moveTracks, "moveTracks", 4*1024, NULL, 10, NULL);
    xTaskCreate(controlLamp, "control Lamp", 1*1024, NULL, 30, NULL);
}

void loop(){

struct trackMoveSx vTrackMove1;
    vTrackMove1.demandLeft = 65536;
    vTrackMove1.demandRight = 65536;
    vTrackMove1.durationMs = 1000;
    
    struct trackMoveSx vTrackMove2;
    vTrackMove2.demandLeft = -65536;
    vTrackMove2.demandRight = -65536;
    vTrackMove2.durationMs = 1000;

    struct trackMoveSx vTrackMove3;
    vTrackMove3.demandLeft = -65536;
    vTrackMove3.demandRight = 65536;
    vTrackMove3.durationMs = 1000;

    struct trackMoveSx vTrackMove4;
    vTrackMove4.demandLeft = 65536;
    vTrackMove4.demandRight = -65536;
    vTrackMove4.durationMs = 1000;

    struct trackMoveSx vTrackMove5;
    vTrackMove5.demandLeft = 65536;
    vTrackMove5.demandRight = 65536;
    vTrackMove5.durationMs = 3000;
    
    ESP_LOGI( TAG, "forward 1 second", NULL);
    xQueueSendToBack(xStructTracKMoveQueue, (void * ) &vTrackMove1, (TickType_t) 0 );
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ESP_LOGI( TAG, "backward 1 second", NULL);
    xQueueSendToBack(xStructTracKMoveQueue, (void * ) &vTrackMove2, (TickType_t) 0 );
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ESP_LOGI( TAG, "left 1 second", NULL);
    xQueueSendToBack(xStructTracKMoveQueue, (void * ) &vTrackMove3, (TickType_t) 0 );
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ESP_LOGI( TAG, "right 1 second", NULL);
    xQueueSendToBack(xStructTracKMoveQueue, (void * ) &vTrackMove4, (TickType_t) 0 );
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ESP_LOGI( TAG, "forward 3 seconds", NULL);
    xQueueSendToBack(xStructTracKMoveQueue, (void * ) &vTrackMove5, (TickType_t) 0 );
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    while(1){
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}