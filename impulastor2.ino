/*
czas pisania
2020.03.28 - 2,5h
2020.03.29 - 1,5h
2020.04.03 - 2,5h
*/
// https://github.com/z3t0/Arduino-IRremote
#include <IRremote.h>

// https://github.com/rocketscream/Low-Power
//#include <LowPower.h>

// ustawienie odczytu podczerwieni
#define irPin A0
IRrecv irrecv(irPin);
decode_results results;

// pin włącz/wyłącz
bool stan_wylaczenia = true;
bool stan_pin_on_off_switch = HIGH;
int pin_on_off_switch = A4;
int pin_on_of_wyjscie = A5;

// konfiguracja pinów enkoderów
int encoder1_pin_a = 2; //D2 pin pod przerwanie
int encoder1_pin_b = 4;
int encoder2_pin_a = 3; //D3 pin pod przerwanie
int encoder2_pin_b = 5;

// konfiguracja pinów wyjciowych
int pin_wyjscia1[] = {6,7,8,9,10,11,12,13};
int pin_wyjscia2[] = {A1,A2,A3};

// nie zmieniać, samemu oblicza prawidłową wartość
int pin_wyjscia1_sizeof = (sizeof(pin_wyjscia1)/sizeof(pin_wyjscia1[0]));
int pin_wyjscia2_sizeof = (sizeof(pin_wyjscia2)/sizeof(pin_wyjscia2[0]));

// przechowanie liczników
int encoder1_licznik = -1;
int encoder2_licznik = -1;
unsigned long time1 = 0;
unsigned long time2 = 0;

void zmien_stan_wylaczenia_przyciskiem()
{
  // odczyt włączenia
  if( digitalRead(pin_on_off_switch) == LOW )
  {
    // zmienia stan z wyłączony na włączony i odwrotnie
    stan_wylaczenia = ! stan_wylaczenia;

    // wykonaj wyłączenie urządzenia
    if(stan_wylaczenia)
    {
      ustaw_pin_wyjscia1(-1);
      ustaw_pin_wyjscia2(-1);
      digitalWrite(pin_on_of_wyjscie, LOW);

      // opóźnienie na włączenie. Czeka aż switch zostanie puszczony.
      while(digitalRead(pin_on_off_switch) == LOW)
      {
        delay(1000);
      }
    }
    // wykonaj włącznie urządzenia
    else
    {
      digitalWrite(pin_on_of_wyjscie, HIGH);
      ustaw_pin_wyjscia1(encoder1_licznik);
      ustaw_pin_wyjscia2(encoder2_licznik);

      // opóźnienie na wyłączenie. Czeka aż switch zostanie puszczony
      while(digitalRead(pin_on_off_switch) == LOW)
      {
        delay(1000);
      }
    }
  }
}

// funkcje wywoływane podczas przerwania
void encoder1_zlicz()
{
  // blokada wykonywania gdy urządzenie jest wyłączone
  if(stan_wylaczenia)
    return;

  if((millis() - time1) > 50)
  {
    static int kierunek = 1;
    
    // zliczanie impulsów
    if(digitalRead(encoder1_pin_b))
      encoder1_licznik -= kierunek;
    else
      encoder1_licznik += kierunek;

    // zapętlanie ze zmianą kierunku
    if(encoder1_licznik >= (pin_wyjscia1_sizeof-1))
    {
      kierunek = -kierunek;
      encoder1_licznik = (pin_wyjscia1_sizeof-1);
    }
    if(encoder1_licznik <= 0)
    {
      kierunek = -kierunek;
      encoder1_licznik = 0;
    }
  
    // ustawienie wyjść
    ustaw_pin_wyjscia1(encoder1_licznik);

    Serial.print("A");
    Serial.println(encoder1_licznik);
  }
  time1 = millis();
}

void encoder2_zlicz()
{
  // blokada wykonywania gdy urządzenie jest wyłączone
  if(stan_wylaczenia)
    return;

  if((millis() - time2) > 50)
  {
    static int kierunek = 1;
    // zliczanie impulsów
    if(digitalRead(encoder2_pin_b))
      encoder2_licznik -= kierunek;
    else
      encoder2_licznik += kierunek;
  
    // zapętlanie ze zmianą kierunku
    if(encoder2_licznik >= (pin_wyjscia2_sizeof-1))
    {
      kierunek = -kierunek;
      encoder2_licznik = (pin_wyjscia2_sizeof-1);
    }
    if(encoder2_licznik <= 0)
    {
      kierunek = -kierunek;
      encoder2_licznik = 0;
    }
  
    // ustawienie wejść
    ustaw_pin_wyjscia2(encoder2_licznik);
  
    Serial.print("B");
    Serial.println(encoder2_licznik);
  }
  time2 = millis();
}


void ustaw_pin_wyjscia1(int kod) // powielanie takiego samego kodu, do optymalizacji
{
  for(int i = 0; i < pin_wyjscia1_sizeof; i++)
  {
    if(kod == i)
    {
      digitalWrite(pin_wyjscia1[i], HIGH);
    }
    else
    {
      digitalWrite(pin_wyjscia1[i], LOW);
    }
  }
}

void ustaw_pin_wyjscia2(int kod) // powielanie takiego samego kodu, do optymalizacji
{
  for(int i = 0; i < pin_wyjscia2_sizeof; i++)
  {
    if(kod == i)
    {
      digitalWrite(pin_wyjscia2[i], HIGH);
    }
    else
    {
      digitalWrite(pin_wyjscia2[i], LOW);
    }
  }
}

// reset programowy
void(* resetFunc) (void) = 0;//declare reset function at address 0

// polecenia dla podczerwieni
void command(decode_results results)
{
  // obsługa włączenia i wyłączenia
  switch(results.value)
  {
    // RC6 philips
    case 0xC:
    case 0x1000C:
    // NEC TV
    case 0xFD00FF:
      //resetFunc(); //call reset

      // zmienia stan z wyłączony na włączony i odwrotnie
      stan_wylaczenia = ! stan_wylaczenia;

      // wykonaj wyłączenie urządzenia
      if(stan_wylaczenia)
      {
        ustaw_pin_wyjscia1(-1);
        ustaw_pin_wyjscia2(-1);      
        digitalWrite(pin_on_of_wyjscie, LOW);

        // opóźnienie na włączenie
        delay(1000);
      }
      // wykonaj włącznie urządzenia
      else
      {
        digitalWrite(pin_on_of_wyjscie, HIGH);
        ustaw_pin_wyjscia1(encoder1_licznik);
        ustaw_pin_wyjscia2(encoder2_licznik);

        // opóźnienie na wyłączenie
        delay(1000);
      }
      break;
    default:
      break;
  }

  // blokada wykonywania gdy urządzenie jest wyłączone
  if(stan_wylaczenia)
    return;

  // obsługa pozostałych opcji sterowania
  switch(results.value)
  {
    // Sterowaniem pinami wyjścia1 zwiększanie
    // RC6 philips
    case 0x5B:
    case 0x1005B:
    // NEC TV
    case 0xFD48B7:
      if(++encoder1_licznik >= (pin_wyjscia1_sizeof-1))
        encoder1_licznik = (pin_wyjscia1_sizeof-1);
      ustaw_pin_wyjscia1(encoder1_licznik);
      break;

    // Sterowaniem pinami wyjścia1 zmniejszanie
    // RC6 philips
    case 0x5A:
    case 0x1005A:
    // NEC TV
    case 0xFD8877:
      if(--encoder1_licznik <= 0)
        encoder1_licznik = 0;
      ustaw_pin_wyjscia1(encoder1_licznik);
      break;

    // Sterowaniem pinami wyjścia2 zwiększanie
    // RC6 philips
    case 0x58:
    case 0x10058:
    // NEC TV
    case 0xFDC837:
      if(++encoder2_licznik >= (pin_wyjscia2_sizeof-1))
        encoder2_licznik = (pin_wyjscia2_sizeof-1);
      ustaw_pin_wyjscia2(encoder2_licznik);
      break;

    // Sterowaniem pinami wyjścia2 zmniejszanie
    // RC6 philips
    case 0x59:
    case 0x10059:
    // NEC TV
    case 0xFD28D7:
      if(--encoder2_licznik <= 0)
        encoder2_licznik = 0;
      ustaw_pin_wyjscia2(encoder2_licznik);
      break;

    // inne
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

  // pin włączenie i wyłącznia
  pinMode(pin_on_off_switch,INPUT_PULLUP);
  digitalWrite(pin_on_of_wyjscie, LOW);
  pinMode(pin_on_of_wyjscie,OUTPUT);

  // ustatawienie i konfiguracja pinów wyjściowych 1
  for(int i = 0; i < pin_wyjscia1_sizeof; i++)
  {
    digitalWrite(pin_wyjscia1[i], LOW);
    pinMode(pin_wyjscia1[i],OUTPUT);
  }
  // ustatawienie i konfiguracja pinów wyjściowych 2
  for(int i = 0; i < pin_wyjscia2_sizeof; i++)
  {
    digitalWrite(pin_wyjscia2[i], LOW);
    pinMode(pin_wyjscia2[i],OUTPUT);
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

  // odczyt włączenia
  zmien_stan_wylaczenia_przyciskiem();

  // echo serial
  while(Serial.available() > 0) // Don't read unless
  {
    Serial.println(Serial.readString());
  }

   // odczyt kodów podczerwieni
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
