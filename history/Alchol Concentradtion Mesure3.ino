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
#define button 10 //컨트롤 버튼

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


float _mesureR0(int pin) {
    int sum = 0;
    int *psum = &sum;
    for (int i = 0; i < 10; i++) {
    (*psum) += 1023 - analogRead(pin); //복합 대입 연산자 A+=B는 A= A+B, +가 먼저 되고, 그다음 =된다고 생각
    }
  return (*psum)/10;
};

float _Rrate(int val, float R0) {
    return (float)( 1023.0 - val )/R0;
};

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
};

void _anounceR0(float R0) {
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

bool button_pressed() {
    //채터링 현상으로 버튼이 눌린 횟수를 확인하려면 짧은 간격에 두 번 검사해서 정말로 눌린 건지 확인해야함
    if(!digitalRead(button) ) { //버튼을 눌렀으면
        delay(10);
        if(!digitalRead(button) ) return true;
    }
    return false;
};

bool button_depressed() {
    //버튼이 눌린 게 아니라면, 채터링 중이거나 정말로 안 눌렸거나 임을 유의!
    if(digitalRead(button) ) { //버튼을 눌렀으면
        delay(10);
        if(digitalRead(button) ) return true;
    }
    return false;
};

enum state {
    R0_reset =1,
    pause,
    LCD_reset
};

int button_state() {
    /*
     버튼을 1초 이상 꾹 누른다면 R0 리셋
     버튼을 1초 안으로 누르고, 0.2초 내에 추가 버튼 입력이 있으면 pause
     없으면 LCD reset
     */
    unsigned long t0, t1;
    enum state state;
    bool flag = 0;
    if(button_pressed() ) {
        t0 = millis();
        flag = 1;
    }
    else { //버튼 입력이 없는 경우
        return -1;
    } 
   
    while(button_pressed()&flag ) {
        //버튼이 눌러진 시간이 1초가 넘으면 R0 리셋으로 판단
        if(millis() - t0 >1000) {
            return R0_reset;
        }
    }

    
    //버튼이 눌렀다 떼진 경우
    if(button_depressed() & flag ) {
        t1 =millis();
    }
    
    //0.2 초 내에 추가 버튼 입력이 있으면 pause로 판단, 없으면 LCD_reset
    while(millis() - t1 < 200) {
        if(button_pressed() & flag) {
            return pause;
        }
    }
    return LCD_reset;
}

void LCD_scroll(String message) {
    //두번 째열에 문자열을 스크롤, 도중에 버튼 입력이 있으면 종료
    Serial.println(message);
    unsigned int len = message.length();
    int n=0;
    
    while(1) {       
        lcd.setCursor(0,1);
        for(int i=0; i<16; i++) {
            lcd.print(message.charAt((i+n)%len ) );
            if(button_pressed() ) return 0;
        }
        n+=1;
        n%=len;

        unsigned long t0 =millis();
        while(millis() - t0 <500) {
            if(button_pressed() ) return 0;
        }
    }
}

struct Sensor {
  float R0;
  float rate;

  void (*warmUp)(int); //예열
  void (*anounceR0)(float);

  struct Cacul {
    float (*mesureR0)(int); //평균 R0 측정
    float (*Rrate)(int, float); //저항비 Rs/R0 측정
    float (*Mapping)(float); //현 저항비로 농도값 계산
  }Cacul;
};

//매개변수 MQ3로 하여 구초제에 값 할당
struct Sensor MQ3 {
  .R0 = 0.0,
  .rate = 0.0,

  .warmUp = _warmUp,
  .anounceR0 =_anounceR0,
  {
    .mesureR0 = _mesureR0,
    .Rrate = _Rrate,
    .Mapping = _Mapping
  }
};

void setup() {
    Serial.begin(9600);
    
    //핀 준비
    for(int i=0; i<3; i++) {
    pinMode(LED_R+i, OUTPUT);
    }
    pinMode(button, INPUT); //내장 풀업 저항 믿지 말자, 내장 풀업 쓰니까 버튼 눌러도, 눌러졌다고 인식 안됨

    //LCD 준비 
    lcd.init(); //lcd 초기화
    lcd.backlight(); //백라이트 켜기
    lcd.createChar(1, R0char);
    
    MQ3.warmUp(5); //90초 예열
    MQ3.R0 = MQ3.Cacul.mesureR0(A0);
    MQ3.anounceR0(MQ3.R0);
}

void loop() {
    //측정 모드
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, LOW);
    
    int value = analogRead(A0);
    MQ3.rate = MQ3.Cacul.Rrate(value, MQ3.R0);
    String info = "Rs/R0:" + String(MQ3.rate, 7) + ", R0:" + String(MQ3.R0, 7) + ", Raw:" + String(value) + "  ";

    //Rs/R0 값 출력
    Serial.println(info);
    lcd.setCursor(0, 0);
    lcd.print( "Rs/R0:" + String(MQ3.rate, 7) );
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); //R0 출력
    lcd.print( ":" + String(MQ3.R0) + ",");
    lcd.print( "Raw:" + String(value) + "  " ); //뒤에 공백 문자를 추가해서 자릿수 바뀔때, 뒤에 남은 문자 제거

    //버튼 입력시 작동
    enum state state;
    state = button_state();

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
        delay(2000);

        MQ3.R0 = MQ3.Cacul.mesureR0(A0);
        MQ3.anounceR0(MQ3.R0);
        break;
        
      case pause:
        Serial.println("pause");
        digitalWrite(LED_R, HIGH);
        digitalWrite(LED_G, HIGH);
        digitalWrite(LED_B, LOW);
        
        float concentration = 0.0;
        //concentration = MQ3.Cacul.Mapping(MQ3.Rrate);
        delay(1000);
        Serial.println("Alchol Concentration: " + String(concentration) );
        //LCD 첫줄에는  알콜 농도를, 둘째줄은 스크롤 하면서 정보를 표시
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(String(concentration) + "%");
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
