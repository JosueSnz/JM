#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_system.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>

// Bluetooth
BluetoothSerial SerialBT; // Define BluetoothSerial object
char valorRecebido;       // Variável para armazenar o valor recebido
//-----------------------------

// Led Piloto
#define LED_vermelho 0 // Até 100% do TPS
#define LED_azul2 4    // Até 75% do TPS
#define LED_azul 16    // Até 50% do TPS
#define LED_verde 17   // Até 25% do TPS
//-----------------------------

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
const int CEBOLINHA = 39; // Pino do sensor
int sensorCEBOLINHA = 0;  // Variável para armazenar o valor lido do sensor
//-----------------------------

// Infravermelho
#define infra1 34
#define infra2 35
float velocidade1 = 0.0;
float velocidade2 = 0.0;
float velocidade = 0.0;
const float raio = 0.21; // Raio da roda em metros
//-----------------------------

// LCD RS (Register Select) - Pin 25 // Enable - Pin 16 // D4 - Pin 5 // D5 - Pin 4 // D6 - Pin 0 // D7 - Pin 2
LiquidCrystal lcd(25, 16, 5, 4, 0, 2);
//-----------------------------

float speed(int pin)
{
    int Time;
    int ativado = 0;
    int lapCount = 0;
    float rpm = 0;
    float velocidade = 0.0;

    Time = 0;

    if (digitalRead(pin) == HIGH) // O sensor indentifica apenas preto e branco, logo, quando o sensor detecta preto, o valor é HIGH
    {
        Time = millis();
        ++ativado;
        if (ativado == 12)
        {
            lapCount++;
            rpm = (lapCount * 60000) / Time;
            velocidade = ((rpm * 2 * 3.14 * raio) / 60) * 3.6; // Velocidade em km/h
        }
    }
    return velocidade;
}

void WriteFile(const char *path, const char *message)
{
    myFile = SD.open(path, FILE_APPEND);
    if (myFile)
    {
        myFile.println(message);
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

    // Create a header in the file
    WriteFile("/data.txt", "TPS1, TPS2, CEBOLINHA, VELOCIDADE, TEMP1, TEMP2, TEMP3, TEMP4 \r\n");

    // Inicializando os pinos dos LEDs
    pinMode(LED_vermelho, OUTPUT);
    pinMode(LED_azul, OUTPUT);
    pinMode(LED_azul2, OUTPUT);
    pinMode(LED_verde, OUTPUT);

    pinMode(infra1, INPUT); // Pino para ler o sinal no coletor do fototransistor
    pinMode(infra2, INPUT); // Pino para ler o sinal no coletor do fototransistor

    // Inicializando o LCD
    lcd.begin(16, 2);
    lcd.print("Velocidade:");
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

    // Lendo o valor do sensor infravermelho
    velocidade1 = speed(infra1);
    velocidade2 = speed(infra2);
    velocidade = (velocidade1 + velocidade2) / 2; // Pegar daqui para o painel

    //-----------------------------

    // Lendo o valor do sensor TPS e cebolinha
    sensorTPS1 = analogRead(TPS1);
    sensorTPS2 = analogRead(TPS2);
    sensorCEBOLINHA = analogRead(CEBOLINHA);

    // Verificar se os valores estão dentro da margem de 1%
    if (abs(sensorTPS1 - sensorTPS2) <= 0.01 * sensorTPS1)
    {
        // Mapeando o valor do sensor para o valor do led, verificar amplitude do TPS
        outputVeTPS = map(sensorTPS1, 0, 1023, 0, 255);
        outputATPS = map(sensorTPS1, 1023, 2046, 0, 255);
        outputA2TPS = map(sensorTPS1, 2046, 3069, 0, 255);
        outputVTPS = map(sensorTPS1, 3069, 4095, 0, 255);

        // Controlando a taxa de variação dos LEDs
        if (sensorTPS1 <= 1023)
        {
            outputATPS = 0;
            outputA2TPS = 0;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 2046)
        {
            outputA2TPS = 0;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 3069)
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
        // Caso os valores estejam fora da margem de 1% apenas o led vermelho é aceso (100% do TPS)
        Serial.print("\nOs valores dos sensores estão fora da margem de 1%");
        analogWrite(LED_vermelho, 255);
        analogWrite(LED_azul, 0);
        analogWrite(LED_azul2, 0);
        analogWrite(LED_verde, 0);
    }

    //-----------------------------

    // Exibir a velocidade no LCD
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
    SerialBT.print("\nVelocidade = ");
    SerialBT.println(velocidade);

    // Gravando os dados no cartão SD
    dataMessage = String(sensorTPS1) + "," + String(sensorTPS2) + "," + String(sensorCEBOLINHA) + "," + String(velocidade) + "\r\n";
    SerialBT.print("Salvando Arquivos: ");
    SerialBT.println(dataMessage);

    WriteFile("/data.txt", dataMessage.c_str());
    // myFile.println(data);

    myFile.close();
    delay(150);
}
