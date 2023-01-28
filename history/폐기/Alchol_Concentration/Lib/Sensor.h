/*
	Sensor.h - Library for Arduino Sensor
	Created by Hyojun Lim, December 11, 2021, Mulgeum Highschool 20931
	Copyright(c) All Rights Reserved
	
	Project 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
	Last Modify 2021.12.11
	
	평균 값을 구하는 averge와 Mapping 지원
*/

#ifndef __Sensor_h__
#define __Sensor_h__

#include "Arduino.h"

class Sensor {
	private:
		uint8_t _pin;
		
	public:
		Sensor(uint8_t pin);
		uint8_t averge(uint8_t num);
		float Mapping(float rate, float *arr);
		
	
};

#endif