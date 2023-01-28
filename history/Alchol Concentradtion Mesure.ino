/*
  2021 화학 실험 MQ-3 센서를 이용한 용액속 알콜농도 측정장치 만들기
  제작 2021 20931 임효준
  https://pkr7098.tistory.com/135 참고

  아두이노 ADC는 10비트 해상도이니 1당 약 0.005V에 해당 5/1024

  감도를 설정하기에 따라 ADC값이 다 다르게 나오니, 알콜없을 때에 대한 저항값의 비율로 측정
  알콜을 만나면 저항값이 떨어지니, Rs/R0는 감소하고, Raw값은 상승한다.

  문제: 측정후 다시 원상태로 돌아가기 까지 오래 걸림, 시간이 지나면 영점 이탈 문제


  알고리즘
  예열(LED RED) ->R0 측정(LED BLUE) -> 저항 비 측정(LED GREEN) ->LCD, Serial print ->다시 저항비 측정으로

  만약 버튼을 누른다면
  R0 다시 측정
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27, 16, 2); //접근 주소 0x27 or 0x3F, 가로 16, 세로 2

const int DELAY_s = 90; //90초 예열 시간 설정
const int LED_R = 5, LED_G = 6, LED_B = 7;
#define ZERO_button 10 //영점 버튼

float R0 = 0.0;
float *pR0 = &R0; //R0의 시작 메모리 주소를 저장하는 포인터 선언 및 초기화

/*
  byte Rschar[8] = {
  B11000,
  B10100,
  B11011,
  B10100,
  B00011,
  B00000,
  B00011,
  B00000,
  };
*/

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

/*
  byte Rwchar[8] = {
  B11000,
  B10100,
  B11000,
  B10101,
  B00000,
  B10101,
  B10101,
  B01010,
  };
*/

void setup() {

  //시리얼 준비
  Serial.begin(9600);

  //핀 준비
  pinMode(LED_R, OUTPUT); //DDRB |= 0x01 << LED_R
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(ZERO_button, INPUT_PULLUP); //내장 풀업 저항 사용

  //lcd 준비
  lcd.init(); //lcd 초기화
  lcd.backlight(); //백라이트 켜기

  //lcd 문자 준비
  lcd.createChar(1, R0char);

  //예열
  digitalWrite(LED_R, HIGH);

  for (int i = 0; i < DELAY_s; i++) { //안내 메세지
    Serial.println("Wait for " + String(DELAY_s - i) + " Sec");

    lcd.setCursor(0, 0);
    lcd.print("Wait for " + String(DELAY_s - i) + " Sec"); //DELAY_s 값을 char로 변환시켜서 프린트
    lcd.setCursor(0, 1);
    lcd.print("Need to Heating!");
    delay(1000);

    //자릿수가 바뀔때 뒤에 Sec가 앞으로 한 칸 밀리면서 Secc로 표기되는 거 방지
    if (DELAY_s - i == 10) {
      lcd.clear();
    }
  }
  lcd.clear();

}

void loop() {

  //R0 측정, 10번 측정한 후 평균을 취한다
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_B, HIGH);
  int sum = 0;
  int *psum = &sum;

  for (int i = 0; i < 10; i++) {
    (*psum) += (1023 - analogRead(A0)); //복합 대입 연산자 A+=B는 A= A+B, +가 먼저 되고, 그다음 =된다고 생각
  }
  *pR0 = (float)(*psum) / 10; //자료형 변환 중요

  //R0 안내 R0
  Serial.println("R0:" + String(*pR0));

  lcd.setCursor(0, 0);
  lcd.print("Mesured R0!");
  lcd.setCursor(0, 1);
  lcd.print( "R0:" + String(*pR0)); //R0값 안내
  delay(1100);

  //백라이트 깜빡거리기
  lcd.noBacklight();
  delay(500);
  lcd.backlight();
  delay(500);
  lcd.clear();

  //측정 상태로 진입 안내 LED 표시
  digitalWrite(LED_R, LOW);
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_B, LOW);


  while (1) {
    int value = analogRead(A0);
    int *p_value = &value;

    //저항비 Rs/R0 계산
    float R = (float)( 1023.0 - (*p_value) ) / (*pR0); //자료형 변환 중요
    float *pR = &R;

    //값 출력
    Serial.println( "Rs/R0:" + String(*pR) + ", R0:" + String(*pR0) + ", Raw:" + String(*p_value));

    lcd.setCursor(0, 0);
    lcd.print( "Rs/R0:" + String(*pR) );
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); //R0 출력
    lcd.print( ":" + String(*pR0) + ",");
    lcd.print( "Raw:" + String(*p_value) );

    if ( digitalRead(ZERO_button) == LOW ) { //풀업 저항이니
      lcd.clear();
      break;
    }

  }

}

//Rs/R0를 입력받아 용액의 농도로 변환해주는 함수
float Mapping_Concentration(float *a)
{

  //실험 데이터의 순서쌍 (Rs/R0, 농도) (Rs/R0)작은 것 부터 입력
  const float _Data[10][2] = {};
  float (*Data)[2] = _Data; //배열을 가르키는 포인터 선언

  //(농도)/ (Rs/R0) 기울기
  float _mean = 0;
  float *mean = &_mean;

  int i = 0;

  //구간은 하나씩 대입해보며 해당하는 구간의 범위 찾기
  for (; i < sizeof(_Data)/sizeof(_Data[0]); i++) {

    //구간 확인
    if (   *(Data[i] + 0) <= *a  &&  *a <= *(Data[i + 1] + 0)   ) {
      break;
      }
  }
  //해당 구간의 기울기 계산
  *mean = ( *(Data[i + 1] + 1) - * (Data[i] + 1) ) / ( *(Data[i + 1] + 0) - * (Data[i] + 0) );
      
  //기울기를 토대로 그때의 일차함수에 데이터값 대입
  return (float)(*mean) * ( *a - * (Data[i] + 0) ) + *(Data[i] + 1);
}

