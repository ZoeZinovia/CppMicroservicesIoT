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

auto start = high_resolution_clock::now(); // Starting timer

void switch_led(int pin, bool value){ // Function to switch led on and off
    pinMode(pin, OUTPUT);
    if (value) {
        digitalWrite(pin, HIGH);
    } else {
        digitalWrite(pin, LOW);
    }
}

void delivered(void *context, MQTTClient_deliveryToken dt) // Required callback
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) // Callback function for when an MQTT message arrives from the broker
{
    int i;
    char* payloadptr; //payload

    payloadptr = (char*)message->payload; //payload converted to char*
    int len = strlen(payloadptr);
    if(payloadptr[len-2] == '}'){ // Fix for a bug in RapidJson
        payloadptr[len-1] = '\0';
    }

    rapidjson::Document document;
    document.Parse(payloadptr); // Parse string to JSON
    if(document.HasMember("Done")){ // Done message is received from publisher when communication ends. This triggers the end of the session and the end of the timer
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        session_status = "Done";
        auto end = high_resolution_clock::now();
        std::chrono::duration<double> timer = end-start;
        std::cout << "LED subscriber runtime = " << timer.count() << "\n";
        std::ofstream outfile;
        outfile.open("piResultsCpp.txt", std::ios_base::app); // append to the results text file
        outfile << "LED subscriber runtime = " << timer.count() << "\n";
        return 0;
    } else{
        if(document.HasMember("LED_1")) { // If the message is about the LED status, the LED is switch accordingly
            led_status = (bool) document["LED_1"].GetBool();
            pin = document["GPIO"].GetInt();
            switch_led(pin, led_status);
        }
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }
}

void connlost(void *context, char *cause) // Required callback for lost connection
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char *argv[]){
    std::string input = argv[1]; // IP address as command line argument to avoid hard coding
    input.append(":1883"); // Append MQTT port
    char char_input[input.length() + 1];
    strcpy(char_input, input.c_str());
    ADDRESS = char_input;

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

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) //Unsuccessful connection
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    else{ // Successful connection
        printf("Connected. Result code %d\n", rc);
    }
    MQTTClient_subscribe(client, TOPIC, QOS);

    while(session_status != "Done"){ // Continue listening for messages until end of session
    //Do nothing
    }

//    MQTTClient_unsubscribe(client, TOPIC);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    switch_led(pin, false);
    return rc;
}