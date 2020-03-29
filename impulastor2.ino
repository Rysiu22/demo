/*
czas pisania
2020.03.28 - 2,5h
2020.03.29 - 1,5h
*/
// https://github.com/z3t0/Arduino-IRremote
#include <IRremote.h>

// https://github.com/rocketscream/Low-Power
//#include <LowPower.h>

// ustawienie odczytu podczerwieni
#define irPin A0
IRrecv irrecv(irPin);
decode_results results;

// konfiguracja pinów enkoderów
int encoder1_pin_a = 2; //D2 pin pod przerwanie
int encoder1_pin_b = 4;
int encoder2_pin_a = 3; //D3 pin pod przerwanie
int encoder2_pin_b = 5;

// konfiguracja pinów wyjciowych
int pin_wyjscia[] = {6,7,8,9,10,11,12,13};
//int pin_wyjscia[] = {A0,A1,A2,A3,A4,A5,A6,A7};
int pin_wyjscia_sizeof = (sizeof(pin_wyjscia)/sizeof(pin_wyjscia[0])); // nie zmieniać, samemu oblicza prawidłową wartość

// przechowanie liczników
int encoder1_licznik = -1;
int encoder2_licznik = -1;
unsigned long time1 = 0;
unsigned long time2 = 0;

// funkcje wywoływane podczas przerwania
void encoder1_zlicz()
{
  if((millis() - time1) > 50)
  {
    static int kierunek = 1;
    
    // zliczanie impulsów
    if(digitalRead(encoder1_pin_b))
      encoder1_licznik -= kierunek;
    else
      encoder1_licznik += kierunek;

    // zapętlanie ze zmianą kierunku
    if(encoder1_licznik >= (pin_wyjscia_sizeof-1))
    {
      kierunek = -kierunek;
      encoder1_licznik = (pin_wyjscia_sizeof-1);
    }
    if(encoder1_licznik <= 0)
    {
      kierunek = -kierunek;
      encoder1_licznik = 0;
    }
  
    // ustawienie wyjść
    ustaw_wyjscie(encoder1_licznik);

    Serial.print("A");
    Serial.println(encoder1_licznik);
  }
  time1 = millis();
}

void encoder2_zlicz()
{
  if((millis() - time2) > 50)
  {
    static int kierunek = 1;
    // zliczanie impulsów
    if(digitalRead(encoder2_pin_b))
      encoder2_licznik -= kierunek;
    else
      encoder2_licznik += kierunek;
  
    // zapętlanie ze zmianą kierunku
    if(encoder2_licznik >= (pin_wyjscia_sizeof-1))
    {
      kierunek = -kierunek;
      encoder2_licznik = (pin_wyjscia_sizeof-1);
    }
    if(encoder2_licznik <= 0)
    {
      kierunek = -kierunek;
      encoder2_licznik = 0;
    }
  
    // ustawienie wyjść
    ustaw_wyjscie(encoder2_licznik);
  
    Serial.print("B");
    Serial.println(encoder2_licznik);
  }
  time2 = millis();
}

void ustaw_wyjscie(int kod)
{
  for(int i = 0; i < pin_wyjscia_sizeof; i++)
  {
    if(kod == i)
    {
      digitalWrite(pin_wyjscia[i], HIGH);
    }
    else
    {
      digitalWrite(pin_wyjscia[i], LOW);
    }
  }
}

// reset programowy
void(* resetFunc) (void) = 0;//declare reset function at address 0

// polecenia dla podczerwieni
void command(decode_results results)
{    
  switch(results.value)
  {
    // RC6 philips
    case 0xC:
    case 0x1000C:
    // NEC TV
    case 0xFD00FF:
      resetFunc(); //call reset
      break;

    // RC6 philips
    case 0x5B:
    case 0x1005B:
    // NEC TV
    case 0xFD48B7:
      if(++encoder1_licznik >= (pin_wyjscia_sizeof-1))
        encoder1_licznik = (pin_wyjscia_sizeof-1);
      ustaw_wyjscie(encoder1_licznik);
      break;

    // RC6 philips
    case 0x5A:
    case 0x1005A:
    // NEC TV
    case 0xFD8877:
      if(--encoder1_licznik <= 0)
        encoder1_licznik = 0;
      ustaw_wyjscie(encoder1_licznik);
      break;

    case 0xFFFFFFFF:
      // powtórz ostatnie polecenie
      break;

    default:
      break;
  }
};

// info o kompilacji wgrywanego projektu
void display_Running_Sketch (void)
{
  Serial.print("# Arduino Sketch: (");
  Serial.print(__DATE__);
  Serial.print(" at ");
  Serial.print(__TIME__);
  Serial.print(") ");
  Serial.println(__FILE__);
}

void setup() {
  // konfiguracja pinów dla enkoderów, z wewnętrznym podciągnięciem do Vcc
  pinMode(encoder1_pin_a,INPUT_PULLUP);
  pinMode(encoder1_pin_b,INPUT_PULLUP);
  pinMode(encoder2_pin_a,INPUT_PULLUP);
  pinMode(encoder2_pin_b,INPUT_PULLUP);

  // ustatawienie i konfiguracja pinów wyjściowych
  for(int i = 0; i < (sizeof(pin_wyjscia)/sizeof(pin_wyjscia[0])); i++)
  {
    digitalWrite(pin_wyjscia[i], LOW);
    pinMode(pin_wyjscia[i],OUTPUT);
  }

  //dodanie przerwania, funkcja wywoływana po wykryciu zbocza opadającego
  attachInterrupt(digitalPinToInterrupt(encoder1_pin_a), encoder1_zlicz, FALLING);
  attachInterrupt(digitalPinToInterrupt(encoder2_pin_a), encoder2_zlicz, FALLING);

  Serial.begin(9600);
  delay(100);
  display_Running_Sketch();

  irrecv.enableIRIn();

  time1 = millis();
  time2 = millis();
}

void loop() {

  while(Serial.available() > 0) // Don't read unless
  {
    Serial.println(Serial.readString());
  }

   if (irrecv.decode(&results)) 
   {
      if (results.decode_type == NEC) {
        Serial.print("NEC: ");
      } else if (results.decode_type == SONY) {
        Serial.print("SONY: ");
      } else if (results.decode_type == RC5) {
        Serial.print("RC5: ");
      } else if (results.decode_type == RC6) {
        Serial.print("RC6: ");
      } else if (results.decode_type == UNKNOWN) {
        Serial.print("UNKNOWN: ");
      }

      Serial.print("0x");
      Serial.println(results.value, HEX);

      command(results);

      delay(250);
      //LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
      irrecv.resume();
   }

   delay(50);
  //LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
}
