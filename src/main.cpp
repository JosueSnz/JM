#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_system.h>
// #include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// Bluetooth
BluetoothSerial SerialBT; // Define BluetoothSerial object
char valorRecebido;       // Variável para armazenar o valor recebido
//-----------------------------

// Led Piloto
#define LED_vermelho 19 // Até 100% do TPS
#define LED_azul 18     // Até 75% do TPS
#define LED_azul2 5     // Até 50% do TPS
#define LED_verde 9     // Até 25% do TPS
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
#define SD_CS_PIN 15
#define LED_BUILTIN 2
//-----------------------------

// Cebolinha
#define CEBOLINHA 25
int sensorCEBOLINHA = 0; // Variável para armazenar o valor lido do sensor
//-----------------------------

// Infravermelho
#define infra1 0
#define infra2 4
float velocidade1 = 0.0;
float velocidade2 = 0.0;
float velocidade = 0.0;
const float raio = 0.3; // Raio da roda em metros
//-----------------------------

void setup()
{
    SerialBT.begin("esp32"); // Nome do dispositivo Bluetooth

    SPI.begin(); // Inicializa a comunicação SPI para o cartão SD

    // Inicializa o cartão SD com o pino CS especificado
    if (!SD.begin(SD_CS_PIN))
    {
        SerialBT.println("Erro ao inicializar o cartão SD");
        digitalWrite(LED_BUILTIN, HIGH);
        return;
    }
    SerialBT.println("Cartão SD inicializado com sucesso");

    // Inicializando os pinos dos LEDs
    pinMode(LED_vermelho, OUTPUT);
    pinMode(LED_azul, OUTPUT);
    pinMode(LED_azul2, OUTPUT);
    pinMode(LED_verde, OUTPUT);

    pinMode(CEBOLINHA, INPUT); // Pino para ler o sinal no sensor de pressão
    pinMode(infra1, INPUT);    // Pino para ler o sinal no coletor do fototransistor
    pinMode(infra2, INPUT);    // Pino para ler o sinal no coletor do fototransistor
}

void loop()
{
    // Cria data.txt para armazenar os dados
    File dataFile = SD.open("/data.txt", FILE_WRITE);

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

    // Verificar se os valores estão dentro da margem de 5%
    if (abs(sensorTPS1 - sensorTPS2) <= 0.10 * sensorTPS1)
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
        // Caso os valores estejam fora da margem de 10% apenas o led vermelho é aceso (100% do TPS)
        SerialBT.print("\nOs valores dos sensores estão fora da margem de 10%");
        analogWrite(LED_vermelho, 255);
        analogWrite(LED_azul, 0);
        analogWrite(LED_azul2, 0);
        analogWrite(LED_verde, 0);
    }

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
    if (dataFile)
    {
        dataFile.print("\nTPS1 = ");
        dataFile.println(sensorTPS1);
        dataFile.print("\nTPS2 = ");
        dataFile.println(sensorTPS2);
        dataFile.print("\nCEBOLINHA = ");
        dataFile.println(sensorCEBOLINHA);
        dataFile.print("\nVelocidade = ");
        dataFile.println(velocidade);
        dataFile.close();
        SerialBT.println("Dados gravados com sucesso");
    }
    else
    {
        SerialBT.println("Erro ao abrir o arquivo");
    }
    delay(150);
}

float speed(int pin)
{
    int Time;
    int lasTime;
    int difference;
    int lapCount = 0;
    float rpm = 0;
    float velocidade = 0.0;

    Time = lasTime = difference = 0;

    if (analogRead(pin) > 1000)
    {
        Time = millis();
        difference = Time - lasTime;
        if (difference > 100)
        {
            lapCount++;
            rpm = (lapCount * 60000) / Time;
            velocidade = ((rpm * 2 * 3.14 * raio) / 60) * 3.6; // Velocidade em km/h
            lasTime = Time;
        }
    }
    return velocidade;
}
