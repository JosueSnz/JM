// --------------------------------------------
// Código para o JOÃO MIGUEL
// Projeto: Grupo Cheetah
// Autoria: Josué Benevides Sanchez
// Data da ultima modificação: 20/04/2024
// --------------------------------------------

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_system.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <ADXL345.h>
#include <Adafruit_BMP085.h>

// Bluetooth
BluetoothSerial SerialBT; // Define BluetoothSerial object
char valorRecebido;       // Variável para armazenar o valor recebido
//-----------------------------

// Led Piloto
#define LED_vermelho 0 // Até 100% do TPS
#define LED_azul2 4    // Até 75% do TPS
#define LED_azul 16    // Até 50% do TPS
#define LED_verde 17   // Até 25% do TPS
//------------------------------

// TPS 1
const int TPS1 = 32;
int sensorTPS1 = 0; // Variável para armazenar o valor lido do sensor

// TPS 2
const int TPS2 = 33;
int sensorTPS2 = 0; // Variável para armazenar o valor lido do sensor

// Manipular leds
int outputVTPS = 0;
int outputATPS = 0;
int outputA2TPS = 0;
int outputVeTPS = 0;
//-----------------------------

// Cartão SD
const int CS = 5;
String dataMessage;
File myFile;
//-----------------------------

// Cebolinha
#define CEBOLINHA 39     // Pino do sensor
int sensorCEBOLINHA = 0; // Variável para armazenar o valor lido do sensor
//-----------------------------

// Infravermelho
#define infra1 34
#define infra2 35

const int faixa1 = 12;
const int faixa2 = 12;
const float diametro = 0.1;
const float circunferencia1 = (diametro)*PI;
const float circunferencia2 = (diametro)*PI;
const int timer1 = 1000;
const int timer2 = 1000;
volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
float velocidade1 = 0;
float velocidade2 = 0;
float velocidade = 0;

void pulseCounter1();
void pulseCounter2();
//-----------------------------

// LCD
#define endereco 0x27 // Endereços comuns: 0x27, 0x3F
#define colunas 16
#define linhas 2

LiquidCrystal_I2C lcd(endereco, colunas, linhas);
//-----------------------------

// Aceletometro
const float alpha = 0.5;

double fXg = 0;
double fYg = 0;
double fZg = 0;

ADXL345 acc;
//-----------------------------

// Temperatura
Adafruit_BMP085 bmp;
float temperatura = 0;
//-----------------------------

void WriteFile(const char *path, const char *message)
{
    myFile = SD.open(path, FILE_APPEND);
    if (myFile)
    {
        myFile.print(message);
        myFile.close();
    }
    else
    {
        Serial.println("erro ao abrir o arquivo");
    }
}

void setup()
{
    SerialBT.begin("esp32"); // Nome do dispositivo Bluetooth

    Serial.begin(115200);
    while (!Serial)
        delay(10);

    Serial.println("\nGrupo Cheetah");
    Serial.println("Iniciando SD card...");
    Serial.println("==============================================");
    Serial.print("MOSI: ");
    Serial.println(MOSI);
    Serial.print("MISO: ");
    Serial.println(MISO);
    Serial.print("SCK: ");
    Serial.println(SCK);
    Serial.print("CS: ");
    Serial.println(SS);
    Serial.println("==============================================\n");
    Serial.println("");
    delay(100);

    Serial.println("Iniciando SD card...");
    if (!SD.begin(CS))
    {
        Serial.println("SD card falhou!");
        while (1)
        {
            delay(10);
        }
    }
    Serial.println("SD card inicialização completa.");
    delay(100);

    // Create a header in the file
    WriteFile("/data.txt", " "); // é recomendado inserir os nomes manualmente

    // Inicializando os pinos dos LEDs
    pinMode(LED_vermelho, OUTPUT);
    pinMode(LED_azul, OUTPUT);
    pinMode(LED_azul2, OUTPUT);
    pinMode(LED_verde, OUTPUT);

    pinMode(CEBOLINHA, INPUT); // Pino para ler o sinal do sensor cebolinha
    pinMode(infra1, INPUT);    // Pino para ler o sinal no coletor do fototransistor
    pinMode(infra2, INPUT);    // Pino para ler o sinal no coletor do fototransistor
    attachInterrupt(digitalPinToInterrupt(infra1), pulseCounter1, RISING);
    attachInterrupt(digitalPinToInterrupt(infra2), pulseCounter2, RISING);
    acc.begin();
    bmp.begin();

    // Inicializando o LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.print("Cheetah-E 2024");
    lcd.setCursor(0, 1);
    lcd.print("Iniciando...");
    delay(2000);
    lcd.clear();
}

void loop()
{
    // Caso seja necessário reiniciar o ESP32
    valorRecebido = (char)SerialBT.read();
    if (valorRecebido == '1')
    {
        SerialBT.print("\nReiniciando o ESP32");
        delay(2000);
        ESP.restart();
    }

    //-----------------------------

    // Lendo os valores do acelerômetro
    double pitch, roll, Xg, Yg, Zg;
    acc.read(&Xg, &Yg, &Zg);
    fXg = Xg * alpha + (fXg * (1.0 - alpha));
    fYg = Yg * alpha + (fYg * (1.0 - alpha));
    fZg = Zg * alpha + (fZg * (1.0 - alpha));
    roll = (atan2(-fYg, fZg) * 180.0) / M_PI;
    pitch = (atan2(fXg, sqrt(fYg * fYg + fZg * fZg)) * 180.0) / M_PI;

    //-----------------------------

    // Lendo a temperatura
    temperatura = bmp.readTemperature();

    //-----------------------------

    // Lendo a velocidade
    unsigned long currentMillis1 = millis();

    if (currentMillis1 - previousMillis1 >= timer1)
    {
        // Calcula as RPM
        float rpm1 = (pulseCount1 * 60.0) / (faixa1 * (currentMillis1 - previousMillis1));
        // Calcula a velocidade em km/h
        velocidade1 = ((rpm1 * diametro) / 60) * 3.6;
        // Zera o contador de pulsos
        pulseCount1 = 0;
        // Atualiza o tempo anterior
        previousMillis1 = currentMillis1;
    }

    unsigned long currentMillis2 = millis();

    if (currentMillis2 - previousMillis2 >= timer2)
    {
        // Calcula as RPM
        float rpm2 = (pulseCount2 * 60.0) / (faixa2 * (currentMillis2 - previousMillis2));
        // Calcula a velocidade em km/h
        velocidade2 = ((rpm2 * diametro) / 60) * 3.6;
        // Zera o contador de pulsos
        pulseCount2 = 0;
        // Atualiza o tempo anterior
        previousMillis2 = currentMillis2;
    }

    velocidade = (velocidade1 + velocidade2) / 2; // Usar para o LCD

    //-----------------------------

    // Lendo o valor do sensor TPS e cebolinha
    sensorTPS1 = analogRead(TPS1);
    sensorTPS2 = analogRead(TPS2);
    sensorCEBOLINHA = digitalRead(CEBOLINHA);

    // Verificar se os valores estão dentro da margem de 5% e se o freio nao esta sendo acionado
    if (abs(sensorTPS1 - sensorTPS2) <= 1 * sensorTPS1 && sensorCEBOLINHA == 1) // esta no modo teste, deve ser ajustado para 5% no projeto final
    {
        // Mapeando o valor do sensor para o valor do led, verificar amplitude do TPS
        outputVeTPS = map(sensorTPS1, 0, 695, 0, 255);
        outputATPS = map(sensorTPS1, 695, 1390, 0, 255);
        outputA2TPS = map(sensorTPS1, 1390, 2085, 0, 255);
        outputVTPS = map(sensorTPS1, 2085, 2700, 0, 255);

        // Controlando a taxa de variação dos LEDs
        if (sensorTPS1 <= 695)
        {
            outputATPS = 0;
            outputA2TPS = 0;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 1390)
        {
            outputA2TPS = 0;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 2085)
        {
            outputVTPS = 0;
        }

        // Mudando a intensidade do led
        analogWrite(LED_vermelho, outputVTPS);
        analogWrite(LED_azul, outputATPS);
        analogWrite(LED_azul2, outputA2TPS);
        analogWrite(LED_verde, outputVeTPS);
    }
    else
    {
        // Caso os valores estejam fora da margem de 5% apenas o led vermelho é aceso (100% do TPS) ou o freio foi acionado
        Serial.print("\nOs valores dos sensores estão fora da margem aceitavel ou foi apertado o freio");
        analogWrite(LED_vermelho, 255);
        analogWrite(LED_azul, 0);
        analogWrite(LED_azul2, 0);
        analogWrite(LED_verde, 0);
    }

    //-----------------------------

    // Exibir a velocidade no LCD
    lcd.setCursor(0, 0);
    lcd.print("VELOCIDADE: ");
    lcd.setCursor(0, 1);
    lcd.print("          "); // Limpar a linha
    lcd.setCursor(0, 1);
    lcd.print(velocidade); // Exibir a velocidade

    //-----------------------------

    // Resultados dos sensores Terminal
    SerialBT.print("\nTPS1 = ");
    SerialBT.println(sensorTPS1);
    SerialBT.print("\nTPS2 = ");
    SerialBT.println(sensorTPS2);
    SerialBT.print("\nCEBOLINHA = ");
    SerialBT.println(sensorCEBOLINHA);
    SerialBT.print("\nVELOCIDADE = ");
    SerialBT.println(velocidade);
    SerialBT.print("\nTEMPERATURA = ");
    SerialBT.println(temperatura);
    SerialBT.print("\nCAMPO 1 = ");
    SerialBT.println(roll);
    SerialBT.print("\nCAMPO 2 = ");
    SerialBT.println(pitch);

    // Gravando os dados no cartão SD
    dataMessage = String(sensorTPS1) + "," + String(sensorTPS2) + "," + String(sensorCEBOLINHA) + "," + String(velocidade) + "," + String(temperatura) + "," + String(roll) + "," + String(pitch) + "\r\n";
    SerialBT.print("Salvando Arquivos: ");
    SerialBT.println(dataMessage);

    WriteFile("/data.txt", dataMessage.c_str());
    // myFile.println(data);

    myFile.close();
    delay(10); // para dados de teste e visualização, no projeto final deve ser retirado ou ajustado para (1)
}

void pulseCounter1()
{
    pulseCount1++;
}

void pulseCounter2()
{
    pulseCount2++;
}