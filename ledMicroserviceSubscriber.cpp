//
// Created by Shani du Plessis on 20/04/2021.
//

extern "C" {
#include <wiringPi.h>
}
#include <csignal>
#include <iostream>

//using namespace std;

bool RUNNING = true;

void blink_led(int pin, int time){
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    std::cout << "ON!";
    delay(time);
    digitalWrite(pin, LOW);
    std::cout << "OFF!";
    delay(time);
}

void exit_program(int s){
    RUNNING = false;
}

int main(){
    std::signal(SIGINT, exit_program);

    wiringPiSetupGpio();

    std::cout << "Controlling the GPIO pins with wiringPi\n";

    int pin = 17;

    int time = 500;
    while(RUNNING){
        blink_led(pin, time);
    }

    std::cout << "Program ended";

}