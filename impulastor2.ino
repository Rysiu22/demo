/*
czas pisania
2020.03.28 - 2,5h
2020.03.29 - 1,5h
2020.04.03 - 3,5h
2020.04.06 - 5h

DC Current per I/O Pin recommended limit: <35mA
DC Current per VCC and GND Pins: 200.0 mA
Overall DC current limit for all IO pins put together: 200 mA

EEPROM 100.000 write cycles per address
*/
// https://github.com/z3t0/Arduino-IRremote
#include <IRremote.h>

// https://github.com/rocketscream/Low-Power
//#include <LowPower.h>

// ustawienie pinu odczytu podczerwieni
#define irPin A0
IRrecv irrecv(irPin);
decode_results results;

// pin włącz/wyłącz
bool stan_wylaczenia = true;
int pin_on_off_switch = A4;
int pin_on_of_wyjscie = A5;
#define CZAS_BLOKADY_ON_OFF_MS 1000

// co jaki czas porównanie i zapis do eepromu, 30s
// 100000cykli/60sekund/60minut = 27godzin zmieniania co sekunde
#define KOPIA_DO_EEPROM_MS 30000

// konfiguracja pinów enkoderów
int encoder1_pin_a = 2; //D2 pin pod przerwanie
int encoder1_pin_b = A6; // A6,A7 pin tylko analogowy bez pull-up
int encoder2_pin_a = 3; //D3 pin pod przerwanie
int encoder2_pin_b = A7; // A6,A7 pin tylko analogowy bez pull-up
int analog_low_high = 512; // granica stanów LOW/HIGH od 0 - 1023

// konfiguracja pinów wyjciowych
int pin_wyjscia1[] = {6,7,8,9,10,11,12,13};
int pin_wyjscia2[] = {A1,A2,A3};

// konfiguracja pinów silniczka
int pin_silnika_kierunek1 = 4;
int pin_silnika_kierunek2 = 5;
unsigned long silnik_czas = 250; // ms

// TU KONIEC ZMIENNYCH DO EDYCJI

// Adresy użyte w eepromie
#include <EEPROM.h>
#define ADDRESS_UINT16_START_COUNT 0
#define ADDRESS_CHAR_LICZNIK1 2
#define ADDRESS_CHAR_LICZNIK2 3
#define ADDRESS_CHAR_STAN_WYLACZENIA 4

// nie zmieniać, samemu oblicza prawidłową wartość
int pin_wyjscia1_sizeof = (sizeof(pin_wyjscia1)/sizeof(pin_wyjscia1[0]));
int pin_wyjscia2_sizeof = (sizeof(pin_wyjscia2)/sizeof(pin_wyjscia2[0]));

// przechowanie liczników
int encoder1_licznik = -1;
int encoder2_licznik = -1;
unsigned long time1 = 0;
unsigned long time2 = 0;
unsigned long time3 = 0;
unsigned long time4 = 0;

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
      // zapis konfiguracji do pamięci EEPROM
      zapamietaj_ustawienia();

      // zerowanie pinów
      ustaw_pin_wyjscia1(-1);
      ustaw_pin_wyjscia2(-1);
      digitalWrite(pin_on_of_wyjscie, HIGH);

      // opóźnienie na włączenie. Czeka aż switch zostanie puszczony.
      while(digitalRead(pin_on_off_switch) == LOW)
      {
        delay(CZAS_BLOKADY_ON_OFF_MS);
      }
    }
    // wykonaj włącznie urządzenia
    else
    {
      // zapis konfiguracji do pamięci EEPROM
      zapamietaj_ustawienia();

      // ustawienie pinów
      digitalWrite(pin_on_of_wyjscie, LOW);
      ustaw_pin_wyjscia1(encoder1_licznik);
      ustaw_pin_wyjscia2(encoder2_licznik);

      // opóźnienie na wyłączenie. Czeka aż switch zostanie puszczony
      while(digitalRead(pin_on_off_switch) == LOW)
      {
        delay(CZAS_BLOKADY_ON_OFF_MS);
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
    //if(digitalRead(encoder1_pin_b))
    if(analogRead(encoder1_pin_b) > analog_low_high) // tylko dla pinów analogowych A6,A7
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
    //if(digitalRead(encoder2_pin_b))
    if(analogRead(encoder2_pin_b) > analog_low_high) // tylko dla pinów analogowych A6,A7
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
      digitalWrite(pin_wyjscia1[i], LOW);
    }
    else
    {
      digitalWrite(pin_wyjscia1[i], HIGH);
    }
  }
}

void ustaw_pin_wyjscia2(int kod) // powielanie takiego samego kodu, do optymalizacji
{
  for(int i = 0; i < pin_wyjscia2_sizeof; i++)
  {
    if(kod == i)
    {
      digitalWrite(pin_wyjscia2[i], LOW);
    }
    else
    {
      digitalWrite(pin_wyjscia2[i], HIGH);
    }
  }
}

// zapis konfiguracji do pamięci EEPROM
bool zapamietaj_ustawienia()
{
  unsigned long time_write = millis();
  int count_writer = 0;
  // zapis wartości do pamięci
  if(!stan_wylaczenia && encoder1_licznik != EEPROM.read(ADDRESS_CHAR_LICZNIK1))
  {
    EEPROM.write(ADDRESS_CHAR_LICZNIK1, (char)encoder1_licznik);
    count_writer++;
  }
  if(!stan_wylaczenia && encoder2_licznik != EEPROM.read(ADDRESS_CHAR_LICZNIK2))
  {
    EEPROM.write(ADDRESS_CHAR_LICZNIK2, (char)encoder2_licznik);
    count_writer++;
  }
  if(stan_wylaczenia != EEPROM.read(ADDRESS_CHAR_STAN_WYLACZENIA))
  {
    EEPROM.write(ADDRESS_CHAR_STAN_WYLACZENIA, (char)stan_wylaczenia);
    count_writer++;
  }

  if(count_writer > 0)
  {
    Serial.print(count_writer);
    Serial.print(" write EEPROM time(ms): ");
    Serial.println(millis() - time_write);
  }
}

// reset programowy
void(* resetFunc) (void) = 0;//declare reset function at address 0

// polecenie dla seriala
void polecenia_serial()
{
  // echo serial
  while(Serial.available() > 0) // Don't read unless
  {
    static String incomingString;
    incomingString = Serial.readString();
    incomingString.toLowerCase();
    incomingString.trim(); //escape spaces

    if(incomingString == "help")
    {
      Serial.println("help - tekst pomocy");
      Serial.println("r0 - fabryczny reset EEPROM (FFFF)");
      Serial.println("r1 - startowy reset EEPROM (0000)");
    }
    else if(incomingString == "r0")
    {
      write_int(0, 65535);
      write_int(2, 65535);
      write_int(4, 65535);
      Serial.println("r0 - ok");
      resetFunc(); //call reset
    }
    else if(incomingString == "r1")
    {
      write_int(0, 0);
      write_int(2, 0);
      write_int(4, 0);
      Serial.println("r1 - ok");
      resetFunc(); //call reset
    }
    else
    {
      Serial.println(incomingString);
    }
  }
}

// polecenia dla podczerwieni
void polecenia_ir(unsigned long czas_blokady_w_ms)
{
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

      delay(czas_blokady_w_ms);
      //LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
      irrecv.resume();
   }  
}

void command(decode_results results)
{
  static decode_results last_results;

  // obsługa włączenia i wyłączenia
  switch(results.value)
  {
    // RC6 philips
    case 0xC:
    case 0x1000C:
      // nie zapętlaj wyłączania
      if(results.value == last_results.value)
        break;
      last_results = results;
    // NEC TV
    case 0xFD00FF:
      //resetFunc(); //call reset

      // zmienia stan z wyłączony na włączony i odwrotnie
      stan_wylaczenia = ! stan_wylaczenia;

      // wykonaj wyłączenie urządzenia
      if(stan_wylaczenia)
      {
        // zapis konfiguracji do pamięci EEPROM
        zapamietaj_ustawienia();

        // zerowanie pinów
        ustaw_pin_wyjscia1(-1);
        ustaw_pin_wyjscia2(-1);      
        digitalWrite(pin_on_of_wyjscie, HIGH);

        // opóźnienie na włączenie
        delay(CZAS_BLOKADY_ON_OFF_MS);
      }
      // wykonaj włącznie urządzenia
      else
      {
        // zapis konfiguracji do pamięci EEPROM
        zapamietaj_ustawienia();
      
        // ustawienie pinów
        digitalWrite(pin_on_of_wyjscie, LOW);
        ustaw_pin_wyjscia1(encoder1_licznik);
        ustaw_pin_wyjscia2(encoder2_licznik);

        // opóźnienie na wyłączenie
        delay(CZAS_BLOKADY_ON_OFF_MS);
      }
      break;
    default:
      break;
  }

  // blokada wykonywania gdy urządzenie jest wyłączone
  if(stan_wylaczenia)
    return;

  // zapamiętywanie ostatnie polecenia NEC, poza poleceniem powtarzania
  if (results.decode_type == NEC && results.value != 0xFFFFFFFF)
    last_results = results;

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

    // Sterowaniem pinami silnika zwiękasznie głośności
    // RC6 philips
    case 0x10:
    case 0x10010:
    // NEC TV
    case 0xFD12ED:
      digitalWrite(pin_silnika_kierunek2, LOW);
      digitalWrite(pin_silnika_kierunek1, HIGH);
      time4 = millis();
      break;

    // Sterowaniem pinami silnika zmniejszanie głośności
    // RC6 philips
    case 0x11:
    case 0x10011:
    // NEC TV
    case 0xFD926D:
      digitalWrite(pin_silnika_kierunek1, LOW);
      digitalWrite(pin_silnika_kierunek2, HIGH);
      time4 = millis();
      break;

    // inne
    case 0xFFFFFFFF:
      // nie powtarzej kodów nie NEC i kodów wyłączenia
      // tu umieścić kody nie podlegające wielokrotnemu powtarzaniu
      if(last_results.decode_type != NEC || last_results.value == 0xFD00FF)
        break;
      // powtórz ostatnie polecenie, ups wywołanie rekurencyjne
      command(last_results);
      break;

    default:
      break;
  }
};

// obsługa pamięci

uint16_t read_int(int addr_pair)
{
  uint16_t value = ((uint16_t)EEPROM.read(addr_pair) << 8) | EEPROM.read(addr_pair+1);
  return value;
}

void write_int(int addr_pair, uint16_t value)
{  
  EEPROM.write(addr_pair, (char)(value >> 8));
  EEPROM.write(addr_pair+1, (char)value);
}

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
  digitalWrite(pin_on_of_wyjscie, HIGH);
  pinMode(pin_on_of_wyjscie,OUTPUT);

  // ustawienie i konfiguracja pinów wyjściowych 1
  for(int i = 0; i < pin_wyjscia1_sizeof; i++)
  {
    digitalWrite(pin_wyjscia1[i], HIGH);
    pinMode(pin_wyjscia1[i],OUTPUT);
  }
  // ustawienie i konfiguracja pinów wyjściowych 2
  for(int i = 0; i < pin_wyjscia2_sizeof; i++)
  {
    digitalWrite(pin_wyjscia2[i], HIGH);
    pinMode(pin_wyjscia2[i],OUTPUT);
  }
  // ustawienie i konfiguracja pinów silnika
  digitalWrite(pin_silnika_kierunek1, LOW);
  pinMode(pin_silnika_kierunek1, OUTPUT);
  digitalWrite(pin_silnika_kierunek2, LOW);
  pinMode(pin_silnika_kierunek2, OUTPUT);

  //dodanie przerwania, funkcja wywoływana po wykryciu zbocza opadającego
  attachInterrupt(digitalPinToInterrupt(encoder1_pin_a), encoder1_zlicz, FALLING);
  attachInterrupt(digitalPinToInterrupt(encoder2_pin_a), encoder2_zlicz, FALLING);

  Serial.begin(9600);
  delay(10);
  display_Running_Sketch();

  write_int(ADDRESS_UINT16_START_COUNT, read_int(ADDRESS_UINT16_START_COUNT) + 1);

  Serial.print("Start count: ");
  Serial.println(read_int(ADDRESS_UINT16_START_COUNT));

  encoder1_licznik = EEPROM.read(ADDRESS_CHAR_LICZNIK1);
  Serial.print("encoder1_licznik (0-7): ");
  Serial.println(EEPROM.read(ADDRESS_CHAR_LICZNIK1));

  encoder2_licznik = EEPROM.read(ADDRESS_CHAR_LICZNIK2);
  Serial.print("encoder2_licznik (0-2): ");
  Serial.println(EEPROM.read(ADDRESS_CHAR_LICZNIK2));

  stan_wylaczenia = EEPROM.read(ADDRESS_CHAR_STAN_WYLACZENIA);
  Serial.print("stan_wylaczenia (0=wlaczony,1=wylaczony): ");
  Serial.println(EEPROM.read(ADDRESS_CHAR_STAN_WYLACZENIA));

  irrecv.enableIRIn();

  time1 = millis();
  time2 = millis();
  time3 = millis();
  time4 = millis();


  if(! stan_wylaczenia)
  {
      digitalWrite(pin_on_of_wyjscie, LOW);
      ustaw_pin_wyjscia1(encoder1_licznik);
      ustaw_pin_wyjscia2(encoder2_licznik);
  }

  Serial.print("time boot: ");
  Serial.print(millis());
  Serial.println("ms");

  Serial.println("help - wyswietli dostepne polecenia");
}

void loop() {
  // odczyt włączenia
  zmien_stan_wylaczenia_przyciskiem();

  // zapisuje ustawienia do EEPROM-u
  if(millis()-time3 > KOPIA_DO_EEPROM_MS)
  {
    zapamietaj_ustawienia();
    time3 = millis();
  }

  // obsługa poleceń dla seriala
  polecenia_serial();

  // odczyt kodów podczerwieni
  polecenia_ir(100); // ms

  // zerowanie ustawienia silnika
  if(millis()-time4 > silnik_czas)
  {
    digitalWrite(pin_silnika_kierunek1, LOW);
    digitalWrite(pin_silnika_kierunek2, LOW);    
  }

  delay(120);
  //LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF); //15 ms, 30 ms, 60 ms, 120 ms, 250 ms, 500 ms, 1 s, 2 s, 4 s, 8 s, and forever
}
