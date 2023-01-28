/*
 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
 객체 지향 프로그래밍 구현 버전
 Copyright(C) 2021 mulgeum 20931 임효준

 감도를 설정하기 따라 값이 다 다르게 나오니, 깨끗한 공기에서 저항에 대한 현 저항의 비율을 측정
 시간이 지남에 따라 센서가 가열되며 저항값이 올라 센서값이 미묘하게 오르기에 영점 버튼 추가함
 반드시 기다려서 비율이 1로 돌아간 다음 측정

 
 알고리즘
 예열(LED RED) ->R0 측정(LED BLUE) -> 저항 비 측정(LED GREEN) ->LCD, Serial print ->버튼 입력 확인 -> 다시 저항비 측정으로

 1.만약 버튼을 버튼을 3초이상 누르고 있다면 -> R0 리셋
 2.만약 버튼을 눌럿다 뗀후 1초 안에 2번 누른다면 -> 일시 중지(값 기록용)
 1도 2도 아니면 -> LCD 초기화(접점 불량으로 LCD 표시 안된 문제 해결)

 만약 일시 중지 상태에서 다시 버튼이 입력되면 퓨즈 해제
 

 이후 실험값을 구하면 미리 만들어 둔 함수에 집어넣어, 농도 구하는 거 추가
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); //접근 주소 0x27 or 0x3F, 가로 16, 세로 2

#define control_button 10 //컨트롤 버튼

byte R0char[8] = {
  B11000,
  B10100,
  B11000,
  B10100,
  B00010,
  B00101,
  B00101,
  B00010,
};

enum LED {
    LED_R =5,
    LED_G,
    LED_B
};


void _warmUp(int sec) {
    //예열 
    for (int i = 0; i < sec; i++) { //안내 메세지
    Serial.println("Wait for " + String(sec - i) + " Sec");

    lcd.setCursor(0, 0);
    lcd.print("Wait for " + String(sec - i) + " Sec  "); //t 값을 char로 변환시켜서 프린트, 뒤에 공백 문자를 추가해서 자릿수 바뀔때, 뒤에 남은 문자 제거
    lcd.setCursor(0, 1);
    lcd.print("Need to Heating!");
    delay(1000);
    
    }
    lcd.clear();
};


float _avergeR0(int pin) {
    int sum = 0;
    int *psum = &sum;
    for (int i = 0; i < 10; i++) {
    (*psum) += 1023 - analogRead(pin); //복합 대입 연산자 A+=B는 A= A+B, +가 먼저 되고, 그다음 =된다고 생각
    }
  return (*psum)/10;
};

float _RRate(int val, float R0) {
    return (float)( 1023.0 - val )/R0;
}

float _Mapping(float R) {
     //실험 데이터의 순서쌍 (Rs/R0, 농도) (Rs/R0)작은 것 부터 입력
     const float _Data[10][2] = {};
     float (*Data)[2] = _Data; //배열을 가르키는 포인터 선언
    
     int i = 0;
    
     //구간은 하나씩 대입해보며 해당하는 구간의 범위 찾기
     for (; i < sizeof(_Data)/sizeof(_Data[0]); i++) {
    
      //구간 확인
       if (   *(Data[i] + 0) <= R  &&  R <= *(Data[i + 1] + 0)   ) {
             break;
             }
        }
     //해당 구간의 기울기 계산
     float mean = ( *(Data[i + 1] + 1) - * (Data[i] + 1) ) / ( *(Data[i + 1] + 0) - * (Data[i] + 0) );
         
     //기울기를 토대로 그때의 일차함수에 데이터값 대입
     return (float)mean * ( R - *(Data[i] + 0) ) + *(Data[i] + 1);
}

enum state{
     pause = 0,
     LCD_reset,
     R0_reset
};
    
int Button_state() {

    enum state state;
    unsigned long t0 = 0;
    bool startIN = false;

    if( digitalRead(control_button) == HIGH ) {
        t0 = millis();
        startIN = true;

        //버튼을 누르고 있는 동안 누른 시간이 3초를 넘기면
        while( digitalRead(control_button) ) {
            if(millis() - t0 >= 3000) {
                state = R0_reset;
                return state;
            }
        }
    }

    t0 = millis(); //t0 초기화
    //버튼이 한 번 눌럿다 떼진 상태에서 1초 안에 다시 버튼이 눌릴 때
    while( millis() - t0 <= 1000 && state != R0_reset) {
        if( digitalRead(control_button) == HIGH ) {
            state = pause;
            return state;
        }
    }

    //처음의 입력만 있었다면
    if( startIN = true && state != R0_reset && state != pause ) {
        state = LCD_reset;
        return state;
    } 
    return -1; //오류
}

//MQ3 클래스에 해당하는 구조체 선언
struct MQ3 {
    int value; //센서로 읽어들인 값
    int sensorPin; //센서핀 설정
    float R0; //기본 저항
    float RRate; //저항비
    void (*warmUp)(int); //예열
    
    struct Cacul {
        float (*avergeR0)(int); //평균 R0 측정
        float (*RRate)(int, float); //저항비 Rs/R0 측정
        float (*Mapping)(float); //현 저항비로 농도값 계산
    }Cacul;
};

//구조체에 값 할당
struct MQ3 MQ3 {
    .value = 0,
    .sensorPin = A0,
    .R0 = 0.0,
    .RRate = 0.0,
    .warmUp = _warmUp,
    {
        .avergeR0 = _avergeR0,
        .RRate = _RRate,
        .Mapping = _Mapping
    }
};

void setup() {
  //시리얼 준비
  Serial.begin(9600);

  //핀 준비
  for(int i=0; i<3; i++) {
    pinMode(LED_R+i, OUTPUT);
  }
  pinMode(control_button, INPUT_PULLUP); //내장 풀업 저항 사용

  //lcd 준비
  lcd.init(); //lcd 초기화
  lcd.backlight(); //백라이트 켜기

  //lcd 문자 준비
  lcd.createChar(1, R0char);

  //예열
  digitalWrite(LED_R, HIGH); //상태 진입 안내
  MQ3.warmUp(90); //90초 예열
}

void loop() {
    //상태 진입 안내
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, HIGH);
    
    MQ3.R0 = MQ3.Cacul.avergeR0(MQ3.sensorPin);
    
    //R0 안내 R0
    Serial.println("R0:" + String(MQ3.R0, 7));

    lcd.setCursor(0, 0);
    lcd.print("Mesured R0!");
    lcd.setCursor(0, 1);
    lcd.print( "R0:" + String(MQ3.R0, 7)); //R0값 안내
    delay(1100);

    //백라이트 깜빡거리기
    lcd.noBacklight();
    delay(500);
    lcd.backlight();
    delay(500);
    lcd.clear();

    //측정 상태로 진입 안내 LED 표시
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, LOW);

    while(1) {
      MQ3.value = analogRead(A0);
      MQ3.RRate = MQ3.Cacul.RRate(MQ3.value, MQ3.R0); //Rs/R0 측정
      
      //Rs/R0 값 출력
      Serial.println( "Rs/R0:" + String(MQ3.RRate, 7) + ", R0:" + String(MQ3.R0, 7) + ", Raw:" + String(MQ3.value));

      lcd.setCursor(0, 0);
      lcd.print( "Rs/R0:" + String(MQ3.RRate, 7) );
      lcd.setCursor(0, 1);
      lcd.write(byte(1)); //R0 출력
      lcd.print( ":" + String(MQ3.R0) + ",");
      lcd.print( "Raw:" + String(MQ3.value) + "  " ); //뒤에 공백 문자를 추가해서 자릿수 바뀔때, 뒤에 남은 문자 제거

     /*
      버튼 입력시
      */
     enum state state;
     state = Button_state(); //버튼 상태 판단
     
     switch(state) {
        case pause:
            while(!digitalRead(control_button)); //버튼이 다시 입력되기 전까지 일시 중지
            break;
            
        case LCD_reset:
            Serial.println("initializing");
            lcd.clear(); //초기화가 자연스럽게 보이려고 추가
            lcd.init(); //병에 탈부착 하다 접점 때문에 LCD 에러날 때 대처를 위해 넣음
            lcd.createChar(1, R0char);
            break;
     }

     if(state = R0_reset) {
        break;
     }
     
     delay(10); //과부화 걸려서 프리징 되는거 방지용
     }
    
}
