/*
	Alchol_Concentration.h - Library for Alchol_Concentration Project
	Created by Hyojun Lim, December 11, 2021, Mulgeum Highschool 20931
	Copyright(c) All Rights Reserved
	
	Project 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
	Last Modify 2021.12.11
	
*/

#ifndef __Alchol_h__
#define __Alchol_h__

#include "Arduino.h"
enum state;

void warmUp(uint8_t sec);
void anounceR0(int R0);

class Button {
	private:
		uint8_t _Bpin;
		
	public:
		Button(uint8_t Bpin) {
			Bpin = _Bpin;
		}
		
		bool pressed();
		int state();
}

void LCD_scroll();




#endif