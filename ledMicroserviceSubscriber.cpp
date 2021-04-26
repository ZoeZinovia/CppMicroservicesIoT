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
#include <chrono>
#include <fstream>

//using namespace std;
using namespace rapidjson;
using namespace std::chrono;

#define CLIENTID    "ledSubscriber"
#define TOPIC       "LED"
#define QOS         1

int pin;
volatile MQTTClient_deliveryToken deliveredtoken;
bool led_status;
std::string session_status;
char* ADDRESS;

auto start = high_resolution_clock::now();

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
    payloadptr = (char*)message->payload;
    int len = strlen(payloadptr);
    if(payloadptr[len-2] == '}'){
        payloadptr[len-1] = '\0';
    }

    std::cout << payloadptr << "\n";

    rapidjson::Document document;
    document.Parse(payloadptr);
    if(document.HasMember("Done")){
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        session_status = "Done";
        auto end = high_resolution_clock::now();
        std::chrono::duration<double> timer = end-start;
        std::ofstream outfile;
        outfile.open("piResultsCpp.txt", std::ios_base::app); // append instead of overwrite
        outfile << "LED subscriber runtime = " << timer.count() << "\n";
        return 0;
    } else{
        if(document.HasMember("LED_1")) {
            led_status = (bool) document["LED_1"].GetBool();
            pin = document["GPIO"].GetInt();
            switch_led(pin, led_status);
        }
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char *argv[]){

    ADDRESS = argv[0];
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
    else{
        printf("Connected. Result code %d\n", rc);
    }
    MQTTClient_subscribe(client, TOPIC, QOS);

    while(session_status != "Done"){
    //Do nothing
    }

//    MQTTClient_unsubscribe(client, TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    switch_led(pin, false);
    std::cout << "Led subscriber finished\n";
    return rc;
}