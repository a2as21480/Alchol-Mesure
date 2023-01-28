/*
	Button.h - Library for Button Input
	Created by Hyojun Lim, December 11, 2021, Mulgeum Highschool 20931
	Copyright(c) All Rights Reserved
	
	Project 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
	Last Modify 2021.12.11
*/

#ifndef __BUTTON_H__
#define __BUTTON_H__

#include "Arduino.h"



class Button {
	private:
		uint8_t _Bpin;	

	public:
		Button(uint8_t Bpin);
		bool pressed();
};


#endif