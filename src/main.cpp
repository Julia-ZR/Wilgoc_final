#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);
DHT dht;

// czujnik DHT
int wilgoc, temperatura;
int staraWilgoc = -1, staraTemp = -1; // przechowanie starych wartosci -1 bo DHT11 ma zakres od 0% do 100%
// przycisk
int stanPrzycisku = LOW;             // do przechowywania zmiennej
unsigned long czas_opoznienia = 100; // drganie stykow
// odczyt DHT i LCD
static unsigned long czas_odczytuDHT = 0; // czas od kiedy dziala DHT
const unsigned long odczyt_DHT = 1000;    // czas po ktorym odczytuje wartosc
unsigned long czas_akt_LCD = 0;           // aktualizowanie DHT
const unsigned long odczyt_LCD = 500;     // czas po ktorym odczytuje wartosc
// wyswieltanie
int aktualnaStrona = 0;
// RFID
#define RST_PIN 9
#define SDA_PIN 10

MFRC522 mfrc522(SDA_PIN, RST_PIN);


void aktualizacjaLCD()
{
  unsigned long obecnyCzas = millis();
  if (obecnyCzas - czas_akt_LCD >= odczyt_LCD);
  {
    czas_akt_LCD = obecnyCzas;
    if (wilgoc != staraWilgoc)
    {
      lcd.setCursor(8, 0);
      lcd.print("   ");
      lcd.setCursor(8, 0);
      lcd.print(wilgoc);
      staraWilgoc = wilgoc; // aktualizacja zachodzi gdy wartosci sa rozne wiec wyrownuje je aby nie "migaly" na LCD
    }
    if (temperatura != staraTemp)
    {
      lcd.setCursor(8, 1);
      lcd.print("   ");
      lcd.setCursor(8, 1);
      lcd.print(temperatura);
      staraTemp = temperatura;
    }
  }
}


void wyswietlanieStrony(int nr_strony)
{

  if (nr_strony == 1) // strona 2.
  {
    aktualizacjaLCD();
    lcd.setCursor(0, 0);
    lcd.print("Wilgoc:");
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.setCursor(11, 1);
    lcd.print("*C");
    lcd.setCursor(11, 0);
    lcd.print("%");
  }
  else if (nr_strony == 0) // strona 1.
  {
    lcd.setCursor(2, 0);
    lcd.print("Kliknij, aby");
    lcd.setCursor(1, 1);
    // lcd.scrollDisplayLeft();
    lcd.print("zobaczyc pogode");
  }
  else if (nr_strony == 2) // strona powitalna
  {
    lcd.setCursor(5, 0);
    lcd.print("WITAJ!");
    lcd.setCursor(2, 1);
    // lcd.scrollDisplayLeft();
    lcd.print("Przyloz karte");
  }
  else if (nr_strony == 3) // odmowa dostepu
  {
    lcd.setCursor(5, 0);
    lcd.print("ODMOWA!");
    lcd.setCursor(2, 1);
    // lcd.scrollDisplayLeft();
    lcd.print("DOSTEPU");
  }
}

void setup()
{

  pinMode(4, INPUT_PULLUP); // przycisk z rezystorem
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init(); // inicjalizacja RC522
  Serial.println("Kod RFID: ");
  lcd.init();
  lcd.backlight();
  dht.setup(2); // pin nr 2 dla wilgotnosciomierza
  lcd.setCursor(5, 0);
  wyswietlanieStrony(2);
  // lcd.print("WITAJ!");
  // lcd.setCursor(2, 1);
  // // lcd.scrollDisplayLeft();
  // lcd.print("Przyloz karte");
}


void dump_byte_array(byte *buffer, byte bufferSize)
{
 
  for (byte i = 0; i < bufferSize; i++)
  {
    //Serial.print("xd");
    Serial.print(buffer[i] < 0x10 ? "0" : " "); //daje 0 na poczatku i spacje pomiedzy

    Serial.print(buffer[i], HEX);
  }
}


void RfidScan()

{
  

  if (!mfrc522.PICC_IsNewCardPresent()) // czy jest karta
  {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) // zczytanie danych karty
  {
    return;
  }
  // sprawdzanie prawidlowosci karty
  byte prawidloweUID[] = {0xDD, 0xB8, 0x75, 0x37};
  bool dostep = true;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    if (mfrc522.uid.uidByte[i] != prawidloweUID[i])
    {
      dostep = false;
      break;
    }
  }
  
  if (dostep) // w razie break program sie zatrzyma
  {
    Serial.println(" --> dostep");
    wyswietlanieStrony(aktualnaStrona);

    
    
  }
  else
  {
    Serial.println(" --> brak dostepu");
    lcd.clear();
    wyswietlanieStrony(3);
  }
 dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size); //ogarnia czy jest czy nie ma dostepu
}




void drganie_stykow()
{
  int aktualnyStanPrzycisku = digitalRead(4);

  static unsigned long czas_odczytu_przycisku = 0;
  static int poprzedniStanPrzycisku = LOW;

  if (aktualnyStanPrzycisku != poprzedniStanPrzycisku)
  {
    czas_odczytu_przycisku = millis();
    poprzedniStanPrzycisku = aktualnyStanPrzycisku;
  }

  if (millis() - czas_odczytu_przycisku > czas_opoznienia) // Pokazywanie stanu co 100 ms
  {
    // Serial.println(aktualnyStanPrzycisku);
    czas_odczytu_przycisku = millis(); // aktualizacja czasu ostatniego odczytu
    if (aktualnyStanPrzycisku == LOW)
    {

      aktualnaStrona++;
      if (aktualnaStrona > 1)
      {
        aktualnaStrona = 0;
      }
      lcd.clear();
      lcd.setCursor(8, 0);
      lcd.print(wilgoc);
      lcd.setCursor(8, 1);
      lcd.print(temperatura);
      wyswietlanieStrony(aktualnaStrona);

      // Serial.println(aktualnaStrona);
    }
  }
}

void TempWilg()
{
  unsigned long obecnyCzas = millis();
  if (obecnyCzas - czas_odczytuDHT >= odczyt_DHT)
  {
    czas_odczytuDHT = obecnyCzas;
    wilgoc = dht.getHumidity();
    temperatura = dht.getTemperature();
  }
}


void loop()
{
  
  RfidScan();
  drganie_stykow();
  TempWilg();
  
  
}
