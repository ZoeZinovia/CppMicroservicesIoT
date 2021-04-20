//
// Created by Shani du Plessis on 20/04/2021.
//

extern "C" {
#include <wiringPi.h>
}
#include <csignal>
#include <iostream>
#include "mqtt/async_client.h"

const std::string ADDRESS("10.35.0.229:1883");
const std::string CLIENTID("AsyncPublisher");
const std::string TOPIC("Temperature");

//using namespace std;

bool RUNNING = true;

void switch_led(int pin){
    int status = digitalRead(pin);
    pinMode(pin, OUTPUT);
    if(status == 0) {
        digitalWrite(pin, HIGH);
        std::cout << "\nON!\n";
    } else{
        digitalWrite(pin, LOW);
        std::cout << "\nOFF!\n";
    }
}

void exit_program(int s){
    RUNNING = false;
}

int main(){
    std::signal(SIGINT, exit_program);

    wiringPiSetupGpio();

    std::cout << "\nControlling the GPIO pins with wiringPi\n";

    int pin = 17;

    int time = 500;
    switch_led(pin);

    mqtt::async_client client(ADDRESS, CLIENTID);

    std::cout << "Program ended\n";

}