/*
	Alchol_Concentration.cpp - Library for Alchol_Concentration Project
	Created by Hyojun Lim, December 11, 2021, Mulgeum Highschool 20931
	Copyright(c) All Rights Reserved
	
	Project 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
	Last Modify 2021.12.11
	
*/

enum state {
    R0_reset =1,
    pause,
    LCD_reset
}state;

void warmUp(uint8_t sec) {
	for(int i=0; i<sec; i++) {
		String message = "wait for " + String(sec-i) + "Sec    ";
		Serial.println(message);
		lcd.
		