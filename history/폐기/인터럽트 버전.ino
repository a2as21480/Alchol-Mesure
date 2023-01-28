/*
 2021 화학 실험 MQ-3 센서를 이용한 알콜농도 측정장치 만들기
 Copyright(c) 2021 20931 임효준
 객체 지향 + 인터럽트 방식
 
  알고리즘
예열(LED RED) ->R0 측정(LED BLUE) -> 저항 비 측정(LED GREEN)(정수로 바꿔서 계산 후 다시 돌려주기) -> 인터럽트 요청 ->LCD, Serial print -> 저항비 측정으로

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
#define DEBUG
#define button 2 //컨트롤 버튼
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2); //접근 주소 0x27 or 0x3F, 가로 16, 세로 2
#include <Sensor.h> //자작 라이브러리
Sensor MQ3(A0);

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

float R0 =0.0, rate =0.0;

void warmUp(int sec) {
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

bool button_pressed() {
    //채터링 현상으로 버튼이 눌린 횟수를 확인하려면 짧은 간격에 두 번 검사해서 정말로 눌린 건지 확인해야함
    if(!digitalRead(button) ) { //버튼을 눌렀으면
        delay(10);
        if(!digitalRead(button) ) return true;
    }
    return false;
};


enum state {
    R0_reset =1,
    pause,
    LCD_reset
}state;

volatile unsigned long t0 = 0, t1 = 0;
volatile unsigned int count = 0;
volatile bool flag = false, once = false, flag_R0reset = false, flag_pause =false;
ISR(INT0_vect) {
    //Interupt Service Routine 인터럽트 0번(D2)핀에 대한 인터럽트 발생 시 작동
    //버튼이 1초 이상 꾹 눌리고 있다면 R0 리셋
    //버튼이 한 번 눌리고 0.5초 안에 또 눌리면 pause
    //버튼이 한 번 눌리고 0.5초가 지났는데도 입력이 없으면 LCD 리셋
    count+=1;
    switch(count) {
        case 1:
            if(button_pressed() ) {
                t0 = millis();
                flag = true;
            }
            else count -=1;
            break;
                
        case 2:
            t1 = millis();
            once = true;
            break;

        case 3:
            if(millis() - t1 <= 500) flag_pause = true;
            break;
    }

    #ifdef DEBUG
    Serial.println(count);
    #endif
}

void INIT_INT0() {
    EIMSK |= (1 << INT0); //INT0 인터럽트 활성화, EIMSK 레지스터 0번 비트를 1로
    EICRA |= 0x01; //Change 일때 인터럽트 발생
    sei(); //전역적인 인터럽트 발생 활성화
}

int button_state() {
    if(!flag) return 0;
    else if(flag_R0reset) return R0_reset;
    else if(flag_pause) return pause;
    else if(once && millis()-t1 > 500) return LCD_reset;
    else return 0;
}

void INIT_button_state() {
    t0 = 0, t1 = 0;
    count = 0;
    flag = false, once = false, flag_R0reset = false, flag_pause =false;
}

void LCD_scroll(float concent, String info) {
    //LCD 첫줄에는  알콜 농도를, 둘째줄은 스크롤 하면서 정보를 표시
    cli(); //인터럽트 발생 금지
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(String(concent) + "%");
    char *anounce = malloc(sizeof(info) ); //메모리 할당
    int n = 0;
    strcpy(anounce, info.c_str() ); //문자열을 char 배열에 할당
    delay(1000);

    while(1) {
        lcd.setCursor(0,1);

        for(int i=0; i<16; i++) {
            lcd.print(anounce[(i+n)%info.length() ]);
            if(button_pressed()) {
                lcd.clear();
                sei();
                free(anounce); //메모리 해제
                return 0;
            }
        }

        unsigned long st = millis();
        while(millis() - st <= 500) {
            if(button_pressed()) {
                lcd.clear();
                sei();
                free(anounce); //메모리 해제
                return 0;
            }
        }
        n+=1; //실행되면서 n이 하나씩 늘어남
        n= n%(info.length() ); //나머지 연산을 통해, 배열이 끝부분이 출력이 된 다음 다시 처음 부분이 출력될 수 있도록 함
    }
    free(anounce);
    sei();
    return 0;
}

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

   warmUp(5); //90초 예열
   R0 = MQ3.avergeR(10); //10번 측정 한 저항값의 평균
   anounceR0(R0); //R0값 안내
   INIT_INT0();
}

void loop() {
    //측정 모드
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_G, HIGH);
    digitalWrite(LED_B, LOW);

    int value = MQ3.avergeR(10);
    float rate = (float)value/R0;
    String info = "Rs/R0:" + String(rate, 7) + ", R0:" + String(R0, 7) + ", Raw:" + String(value) + "  "; //스크롤시 자연스럽게 보이기 위해 공백 추가

    //Rs/R0 값 출력
    Serial.println(info);
    lcd.setCursor(0, 0);
    lcd.print( "Rs/R0:" + String(rate, 7) );
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); //R0 출력
    lcd.print( ":" + String(R0) + ",");
    lcd.print( "Raw:" + String(value) + "  " ); //뒤에 공백 문자를 추가해서 자릿수 바뀔때, 뒤에 남은 문자 제거

    while(button_pressed()&flag ) {
        if(millis() - t0 >1000 ) {
            flag_R0reset = true;
            break;
        }
    }
    state = button_state();
    switch(state) {
        case R0_reset:
            Serial.println("R0_reset");
            INIT_button_state();
            delay(1000);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("R0_reset");
            //상태 진입 안내
            digitalWrite(LED_R, LOW);
            digitalWrite(LED_G, LOW);
            digitalWrite(LED_B, HIGH);
            delay(2000);

            R0 = MQ3.avergeR(100);
            anounceR0(R0);
            break;

        case pause:
            Serial.println("pause");
            INIT_button_state();
            delay(1000);
            digitalWrite(LED_R, HIGH);
            digitalWrite(LED_G, HIGH);
            digitalWrite(LED_B, LOW);
        
            float concentration = 0.0;
            float data[10][2]; //실험 데이터 입력
            //concentration = MQ3.Mapping(rate, data);
            Serial.println("Alchol Concentration: " + String(concentration) );
            LCD_scroll(concentration, info);
            break;

        case LCD_reset:
            Serial.println("LCD reset");
            INIT_button_state();
            delay(1000);
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
