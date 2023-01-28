/*
2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
Copyright(c) 2021 20931 임효준

참고(영문)
https://www.arduino.cc/en/Hacking/LibraryTutorial#.Uws0tPl_t58
*/
#ifndef Sensor_h
#define Sensor_h

#include "Arduino.h"

class Sensor {
    private:
    //private method
    int sensorpin;
    
    public:
    //public method
    Sensor(uint8_t pin); //생성자, 반드시 클래스와 이름이 같아야됨, 쉽게 말하면 SoftwareSerial mySerial(TX, RX) 만들어주는 것임
    
    unsigned int avergeR(unsigned int num);
    float Mapping(float R, float data[][2]);
};
#endif