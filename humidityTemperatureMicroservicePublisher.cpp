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

#define CLIENTID    "ExampleClientPub"
#define TOPIC_T       "Temperature"
#define TOPIC_H       "Humidity"
#define QOS         1
#define TIMEOUT     10000L
#define DHT_PIN		12	/* GPIO-22 */

using namespace rapidjson;
using namespace std::chrono;

char* ADDRESS;
const char* topic_humidity = "Humidity";
const char* topic_temperature = "Temperature";
std::string PAYLOAD =      "Hello World!";

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
    printf("Waiting for up to %d seconds for publication of message.\n",
           (int) (TIMEOUT / 1000));
    int rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    printf("Message with delivery token %d delivered\n", token);
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
    double humidity = 0;
    double temperature = 0;
    int count = 0;
    while(count <= 2) {
        if(count == 2){
            rapidjson::Document document_done;
            document_done.SetObject();
            rapidjson::Document::AllocatorType& allocator1 = document_done.GetAllocator();
            document_done.AddMember("Done", true, allocator1);
            std::string pub_message_done = json_to_string(document_done);
            rc = publish_message(pub_message_done, TOPIC_T, client);
            rc = publish_message(pub_message_done, TOPIC_H, client);
        }

        else {
            /* pull pin down for 18 milliseconds */
            pinMode( DHT_PIN, OUTPUT );
            digitalWrite( DHT_PIN, LOW );
            delay( 18 );

            /* prepare to read the pin */
            pinMode( DHT_PIN, INPUT );
            digitalRead()
            //Create JSON DOM document object for humidity
            rapidjson::Document document_humidity;
            document_humidity.SetObject();
            rapidjson::Document::AllocatorType &allocator2 = document_humidity.GetAllocator();
            document_humidity.AddMember("Humidity", 88, allocator2);
            document_humidity.AddMember("Unit", "%", allocator2);

            //Create JSON DOM document object for temperature
            rapidjson::Document document_temperature;
            document_temperature.SetObject();
            rapidjson::Document::AllocatorType &allocator3 = document_temperature.GetAllocator();
            document_temperature.AddMember("Temp", 12, allocator3);
            document_temperature.AddMember("Unit", "C", allocator3);
            try {
                std::string pub_message_humidity = json_to_string(document_humidity);
                rc = publish_message(pub_message_humidity, TOPIC_H, client);
                std::string pub_message_temperature = json_to_string(document_temperature);
                rc = publish_message(pub_message_temperature, TOPIC_T, client);
            } catch (const std::exception &exc) {
                // catch anything thrown within try block that derives from std::exception
                std::cerr << exc.what();
            }
        }

        count = count + 1;
    }
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    auto end = high_resolution_clock::now(); // Starting timer
    std::chrono::duration<double> timer = end-start;
    std::ofstream outfile;
    outfile.open("piResultsCpp.txt", std::ios_base::app); // append to the results text file
    outfile << "Humidity and temperature publisher runtime = " << timer.count() << "\n";
    std::cout << "Humidity and temperature runtime = " << timer.count() << "\n";
    return rc;
}