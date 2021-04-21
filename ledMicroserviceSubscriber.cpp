//
// Created by Shani du Plessis on 20/04/2021.
//

extern "C" {
    #include <wiringPi.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "MQTTClient.h"
}
#include <csignal>
#include <iostream>
#include "include/rapidjson/document.h"

#define ADDRESS     "10.35.0.229:1883"
#define CLIENTID    "ledSubscriber"
#define TOPIC       "LED"
#define QOS         1

//using namespace std;
using namespace rapidjson;

bool RUNNING = true;
int pin = 17;

volatile MQTTClient_deliveryToken deliveredtoken;

void switch_led(int pin, bool value){
    pinMode(pin, OUTPUT);
    if (value) {
        digitalWrite(pin, HIGH);
        std::cout << "\nON!\n";
    } else {
        digitalWrite(pin, LOW);
        std::cout << "\nOFF!\n";
    }
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

static rapidjson::Document str_to_json(const char* json) {
    rapidjson::Document document;
    document.Parse(json);
    return std::move(document);
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
//    printf("     topic: %s\n", topicName);
//    printf("   message: ");
    payloadptr = (char*)message->payload;
//    std::string MQTT_message = (std::string)payloadptr;
    std::cout << payloadptr << "\n";
    rapidjson::Document document;
    document.Parse(payloadptr);

    bool led_status = document["LED_1"].GetBool();
    std::cout << led_status << "\n";

    switch_led(pin, true);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

//void exit_program(int s){
//    MQTTClient_unsubscribe(client, TOPIC);
//    MQTTClient_disconnect(client, 10000);
//    MQTTClient_destroy(&client);
//    std::cout << "Program ended\n";
//}

int main(){
//    std::signal(SIGINT, exit_program);
    wiringPiSetupGpio();

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_unsubscribe(client, TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}