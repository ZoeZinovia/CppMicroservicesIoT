////
//// Created by Shani du Plessis on 21/04/2021.
////

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
#include "include/rapidjson/stringbuffer.h"
#include "include/rapidjson/prettywriter.h"
#include "include/rapidjson/writer.h"
#include <chrono>
#include <fstream>
#include <typeinfo>

// Pi variables
#define PIN 17

// MQTT variables
#define CLIENTID    "pir_client"
#define TOPIC       "PIR"
#define QOS         0
#define TIMEOUT     10000L

char* ADDRESS;

// RapidJson variables
using namespace rapidjson;
using namespace std::chrono;

int publish_message(std::string str_message, const char *topic, MQTTClient client){
    // Initializing components for MQTT publisher
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    // Updating values of pubmsg object
    char *message = new char[str_message.length() + 1];
    strcpy(message, str_message.c_str());
    pubmsg.payload = message;
    pubmsg.payloadlen = (int)std::strlen(message);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client, topic, &pubmsg, &token); // Publish the message
    int rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    return rc;
}

std::string json_to_string(const rapidjson::Document& doc){
    //Serialize JSON to string for the message
    rapidjson::StringBuffer string_buffer;
    string_buffer.Clear();
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(string_buffer);
    doc.Accept(writer);
    return std::string(string_buffer.GetString());
}


int main(int argc, char* argv[])
{
    auto start = high_resolution_clock::now(); // Starting timer

    std::string input = argv[1]; // IP address as command line argument to avoid hard coding
    input.append(":1883"); // Append MQTT port
    char char_input[input.length() + 1];
    strcpy(char_input, input.c_str());
    ADDRESS = char_input;

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    } else{
        printf("Connected. Result code %d\n", rc);
    }

    wiringPiSetup(); // Required for wiringPi
    pinMode(PIN, INPUT);
    bool motion = false;
    int count = 0;
    int numIterations = 10000;
    while(count <= numIterations) {
        if(count == numIterations){
            rapidjson::Document document_done;
            document_done.SetObject();
            rapidjson::Document::AllocatorType& allocator1 = document_done.GetAllocator();
            document_done.AddMember("Done", true, allocator1);
            std::string pub_message_done = json_to_string(document_done);
            rc = publish_message(pub_message_done, TOPIC, client);
        }
        else {
            motion = digitalRead(PIN);
            //Create JSON DOM document object for humidity
            rapidjson::Document document_pir;
            document_pir.SetObject();
            rapidjson::Document::AllocatorType &allocator2 = document_pir.GetAllocator();
            document_pir.AddMember("PIR", motion, allocator2);
            try {
                std::string pub_message_pir = json_to_string(document_pir);
                rc = publish_message(pub_message_pir, TOPIC, client);
            } catch (const std::exception &exc) {
                // catch anything thrown within try block that derives from std::exception
                std::cerr << exc.what();
            }
        }
        count = count + 1;
    }

    // End of loop. Stop MQTT and calculate runtime
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    auto end = high_resolution_clock::now();
    std::chrono::duration<double> timer = end-start;
    std::ofstream outfile;
    outfile.open("piResultsCpp.txt", std::ios_base::app); // append to the results text file
    outfile << "PIR publisher runtime = " << timer.count() << "\n";
    std::cout << "PIR runtime = " << timer.count() << "\n";
    return rc;
}

