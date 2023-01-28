/*
 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
 객체 지향 프로그래밍 구현 버전
 Copyright(C) 2021 mulgeum 20931 임효준

 알고리즘
예열(LED RED) ->R0 측정(LED BLUE) -> 저항 비 측정(LED GREEN) ->LCD, Serial print -> 버튼이 눌렸는가?

버튼이 눌리지 않았다면 -> 저항비 측정으로
버튼이 1초 이상 눌렸다 -> R0 다시 측정
버튼이 1초 이상 눌리지 않고, 버튼이 때진 상태에서 0.2 초 안에 다른 버튼 입력이 있다면 -> pause(LED 주황)
버튼이 1초 이상 눌리지 않고, 처음의 입력만 있다면 -> LCD 리셋

시간이 지나면서 센서가 가열되고, 그에 따라 센서의 저항이 변하게 된다. 이러한 문제를 상쇄하기 위해 R0 리셋 기능 추가

사용 설명서
그림(첨부) 대로 연결하였다면
1.예열 모드 (LED red)
2.R0 측정 (LED blue)
3.측정 모드 (LED green)
측정 모드에서 버튼을 한 번 누르게 되면 LCD가 리셋됩니다.
이는 접점 문제로 LCD에 에러가 발생할 경우 리셋하기 위한 조치입니다.

버튼을 짧게 두번 누르면 퓨즈 모드가 실행되며, LED가 노란색으로 표시됩니다.
이때 버튼을 한 번 더 누르는 것으로 퓨즈 모드를 빠져나올 수 있습니다.

버튼을 1초 이상 누르게 되면, R0값이 현재의 값으로 재설정 됩니다.
 
 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); //접근 주소 0x27 or 0x3F, 가로 16, 세로 2
#define button 10

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

enum state {
    R0_reset =1,
    pause,
    LCD_reset
}state;

enum LED {
    LED_R =5,
    LED_G,
    LED_B
};

void warmUp(uint8_t sec) {
    digitalWrite(LED_R, HIGH); // 상태 진입 안내
    
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

void anounceR0(float R0) {
    //상태 진입 안내
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, LOW);
    digitalWrite(LED_B, HIGH);
    
    
    //R0 안내 R0
    Serial.println("R0:" + String(R0, 7));

    lcd.setCursor(0, 0);
    lcd.print("Mesured R0!");
    lcd.setCursor(0, 1);
    lcd.print( "R0:" + String(R0, 7)); //R0값 안내
    delay(1100);

    //백라이트 깜빡거리기
    lcd.noBacklight();
    delay(500);
    lcd.backlight();
    delay(500);
    lcd.clear();
};

float averge(uint8_t num) {
    unsigned int val = 0;
    for(int i; i<num; i++) {
         val += analogRead(A0);
    }
    return (float)val/num;
};

float Mapping(float rate) {
    int i=0;
    float arr[10][2];
    
    //해당하는 구간 찾기
    for(; i< (sizeof(arr)/sizeof(arr[0])-1); i++) {
         if(arr[i][0] <= rate && rate <= arr[i+1][0]) break;
    }
    
    //기울기 계산
    float mean = (arr[i+1][1] - arr[i][1])/(arr[i+1][0] - arr[i][0]);
    return (float)mean * (rate - arr[i][0]) + arr[i][1]; //구한 기울기를 토대로 일차함수에다가 값을 대입해서 농도값 계산
};

void INIT_INT0() {
    EIMSK |= (1 << INT0);           //INT0 인터럽트 활성화, EIMSK 레지스터 0번 비트를 1로
    EICRA |= 0x03;                  //Falling 일때 인터럽트 발생
    sei();                          //전역적인 인터럽트 발생 활성화
};

bool button_pressed() {
    //채터링 현상으로 버튼이 눌린 횟수를 확인하려면 짧은 간격에 두 번 검사해서 정말로 눌린 건지 확인해야함
    if(!digitalRead(button) ) { //버튼을 눌렀으면
        delay(10);
        if(!digitalRead(button) ) return true;
    }
    return false;
};

volatile unsigned int count = 0;
volatile unsigned long t0 = 0;
volatile bool flag = false, flag_pause = false; 

void flag_init() {
    count = 0;
    t0 = 0;
    flag = false, flag_pause = false;
};

int button_state() {
    //버튼 상태 판단 함수
    Serial.println(flag);
    if(!flag) return 0;
            
    if(flag_pause) {
        flag_init();
        return pause;
    }

    while(flag && button_pressed() ) {
        if(millis() - t0 > 1000 ) {
            flag_init();
            return R0_reset;
        }
    }
    
    if(flag) return LCD_reset;
};



ISR(INT0_vect) {
    Serial.println("interupt");
    count +=1;
    switch(count) {
        case 1:
            flag = true;
            t0 = millis();
            break;

        case 2: 
            if(millis() - t0 < 200) {flag_pause = true;
            Serial.println("twice"); }
            break;      
    }
    Serial.println(count);
};

void LCD_scroll(String message) {
    //두번 째열에 문자열을 스크롤, 도중에 버튼 입력이 있으면 종료
    int n=0;
    while(1) {
        for(int i=0; i<16; i++) {
            int s = (i+n)%message.length();
            lcd.print(message.charAt(s) );
            if(button_pressed() ) return 0;
        }
        n+=1;
        n%=message.length();

        unsigned long t0 =millis();
        while(millis() - t0 <500) {
            if(button_pressed() ) return 0;
        }
    }
}

float R0 =0.0;
float rate = 0.0;

void setup() {
    Serial.begin(9600);
    
    //핀 준비
    for(int i=0; i<3; i++) {
    pinMode(LED_R+i, OUTPUT);
    }
    pinMode(button, INPUT_PULLUP);
    
    //LCD 준비 
    lcd.init(); //lcd 초기화
    lcd.backlight(); //백라이트 켜기
    lcd.createChar(1, R0char);

    warmUp(5); //예열
    R0 = 1023.0 - averge(10);
    anounceR0(R0);

    INIT_INT0();
}

void loop() {
    //측정 모드
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, LOW);

    float val = 1023 - averge(10);
    int value = analogRead(A0);
    rate = (float)val/R0;
    String info = "Rs/R0:" + String(rate, 7) + ", R0:" + String(R0, 7) + ", Raw:" + String(value)+"  ";
    
    Serial.println(info);
    lcd.setCursor(0, 0);
    lcd.print( "Rs/R0:" + String(rate, 7) );
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); //R0 출력
    lcd.print( ":" + String(R0) + ",");
    lcd.print( "Raw:" + String(value) + "  " ); //뒤에 공백 문자를 추가해서 자릿수 바뀔때, 뒤에 남은 문자 제거

    if(flag) state = button_state();
    switch(state) {
      case R0_reset:
        Serial.println("R0_reset");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("R0_reset");
        //상태 진입 안내
        digitalWrite(LED_R, LOW);
        digitalWrite(LED_G, LOW);
        digitalWrite(LED_B, HIGH);
        delay(1000);

        R0 = 1023.0 - averge(10);
        anounceR0(R0);
        break;

      case pause:
        Serial.println("pause");
        digitalWrite(LED_R, HIGH);
        digitalWrite(LED_G, HIGH);
        digitalWrite(LED_B, LOW);
        
        float concentration = 0.0;
        //concentration = Mapping(rate);
        delay(1000);
        Serial.println("Alchol Concentration: " + String(concentration) );
        LCD_scroll(info);
        break;

      case LCD_reset:
        Serial.println("LCD reset");
        lcd.init(); //lcd 초기화
        lcd.backlight(); //백라이트 켜기
        lcd.createChar(1, R0char);
        lcd.noBacklight();
        delay(500);
        lcd.backlight();
        break;
    };
    delay(100); //과부화로 인한 프리징 방지, 숫자가 빠르게 바뀌면 읽기 어려움
}
