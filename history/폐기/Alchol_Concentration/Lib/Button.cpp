/*
	Button.cpp - Library for Button Input
	Created by Hyojun Lim, December 11, 2021, Mulgeum Highschool 20931
	Copyright(c) All Rights Reserved
	
	Project 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
	Last Modify 2021.12.11
*/

Button::Button(uint8_t Bpin) {
	Bpin = _Bpin;
    pinMode(_Bpin, INPUT_PULLUP);
    EIMSK |= (1 << INT0);           //INT0 인터럽트 활성화, EIMSK 레지스터 0번 비트를 1로
    EICRA |= 0x10;                  //Falling 일때 인터럽트 발생
    sei();                          //전역적인 인터럽트 발생 활성화
};


bool Button::pressed() {
	//채터링 현상으로 버튼이 눌린 횟수를 확인하려면 짧은 간격에 두 번 검사해서 정말로 눌린 건지 확인해야함
    if(!digitalRead(_Bpin) ) { //버튼을 눌렀으면
        delay(10);
        if(!digitalRead(_Bpin) ) return true;
    }
    return false;
};

