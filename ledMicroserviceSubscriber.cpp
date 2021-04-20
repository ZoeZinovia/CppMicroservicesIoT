//
// Created by Shani du Plessis on 20/04/2021.
//

#include <iostream>
#include <wiringPi.h>
#include <csignal>

bool RUNNING = true;

void blink_led(int pin, int time){
    digitalWrite(pin, HIGH);
    delay(time);
    digitalWrite(pin, LOW);
    delay(time);
}

void exit_program(int s){
    RUNNING = false;
}

int main(){
    std::signal(SIGINT, exit_program);

    wiringPiSetupGpio();

    std::cout << "Controlling the GPIO pins with wiringPi\n";

    int pin = 4;

    int time = 1000;
    while(RUNNING){
        blink_led(pin, time);
    }

    std::cout << "Program ended";

}