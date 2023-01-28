/*
	Sensor.cpp - Library for Arduino Sensor
	Created by Hyojun Lim, December 11, 2021, Mulgeum Highschool 20931
	Copyright(c) All Rights Reserved
*/

Sensor::Sensor(uint8_t pin) {
	pin = _pin;
};

uint8_t Sensor::averge(uint8_t num) {
	int val = 0;
	for(int i; i<num; i++) {
		val += analogRead(_pin);
	}
	val /= num;
	return val;
};

float Sensor::Mapping(float rate, float *arr) {
	int i=0;
    
    //해당하는 구간 찾기
    for(; i< (sizeof(arr)/sizeof(arr[0])-1); i++) {
        if(arr[i][0] <= rate && rate <= arr[i+1][0]) break;
    }
    
    //기울기 계산
    float mean = (arr[i+1][1] - arr[i][1])/(arr[i+1][0] - arr[i][0]);
    return (float)mean * (rate - arr[i][0]) + arr[i][1]; //구한 기울기를 토대로 일차함수에다가 값을 대입해서 농도값 계산
};