/*
czas pisania
2020.03.28 - 2,5h
*/

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
int encoder1_licznik = 0;
int encoder2_licznik = 0;

// funkcje wywoływane podczas przerwania
void encoder1_zlicz()
{
  // zliczanie impulsów
  if(digitalRead(encoder1_pin_b))
    encoder1_licznik--;
  else
    encoder1_licznik++;
  
  Serial.print("A");
  Serial.println(encoder1_licznik);
}

void encoder2_zlicz()
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
  display_Running_Sketch();
}

void loop() {
  // put your main code here, to run repeatedly:

}
