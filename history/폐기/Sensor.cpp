#include "Arduino.h"
#include "Sensor.h"

Sensor::Sensor(uint8_t pin) {
    //생성자, 반드시 클래스와 이름이 같아야됨, 쉽게 말하면 SoftwareSerial mySerial(TX, RX) 만들어주는 것임
	//Sensor::이건 Sensor 라는 클래스에 속한다는 뜻
    pin=sensorpin; //다른 곳에서도 사용하기 위해서
};

unsigned int Sensor::avergeR(unsigned int num) {
    //입력한 횟수 만큼 센서 값을 측정하여 평균 값을 반환
	unsigned long long sum = 0;
    for(int i=0; i<num; i++) {
        sum += (1023 - analogRead(sensorpin) );  
    };
	return sum/num;
};

float Sensor::Mapping(float R, float data[][2]) {
    //데이터를 저장한 배열, 현재 저항 값인 (Rs/R0, 농도) 를 가지고 내삽법으로 농도 값으로 변환해주는 함수
    int i=0;
    
    //해당하는 구간 찾기
    for(; i< (sizeof(data)/sizeof(data[0])-1); i++) {
        if(data[i][0] <= R && R <= data[i+1][0]) break;
    }
    
    //기울기 계산
    float mean = (data[i+1][1] - data[i][1])/(data[i+1][0] - data[i][0]);
    return (float)mean * (R - data[i][0]) + data[i][1]; //구한 기울기를 토대로 일차함수에다가 값을 대입해서 농도값 계산
}; 