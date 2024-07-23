// --------------------------------------------
// Código para o JOÃO MIGUEL
// Projeto: Grupo Cheetah
// Autoria: Josué Benevides Sanchez
// Data da ultima modificação: 16/06/2024
// --------------------------------------------

#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_system.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS.h>
#include <SoftwareSerial.h>

// Bluetooth
BluetoothSerial SerialBT; // Define BluetoothSerial object
char valorRecebido;       // Variável para armazenar o valor recebido
//-----------------------------

// Painel Piloto
#define LED_vermelho 0 // Até 100% do TPS
#define LED_azul2 4    // Até 75% do TPS
#define LED_azul 2     // Até 50% do TPS
#define LED_verde 15   // Até 25% do TPS
// #define Botao_Reset 12 // Botão de reset
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

// Aceletometro / Giroscopio / Temperatura MPU6050
const int MPU = 0x68;
float AccX, AccY, AccZ, Temp, GyrX, GyrY, GyrZ;
//-----------------------------

TinyGPS gps;
SoftwareSerial ss(16, 17); // RX, TX

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
    // SerialBT.begin("esp32"); // Nome do dispositivo Bluetooth

    Wire.setClock(400000); // 400kHz, alterar caso nao de interferencia

    Serial.begin(115200);
    ss.begin(9600); // Inicializa a comunicação serial com o GPS

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

    // Inicializa o MPU-6050
    Wire.begin();
    Wire.beginTransmission(MPU);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);

    // Configura Giroscópio para fundo de escala desejado
    /*
        Wire.write(0b00000000); // fundo de escala em +/-250°/s
        Wire.write(0b00001000); // fundo de escala em +/-500°/s
        Wire.write(0b00010000); // fundo de escala em +/-1000°/s
        Wire.write(0b00011000); // fundo de escala em +/-2000°/s
    */
    Wire.beginTransmission(MPU);
    Wire.write(0x1B);
    Wire.write(0b00001000); // Trocar esse comando para fundo de escala desejado conforme acima
    Wire.endTransmission();

    // Configura Acelerometro para fundo de escala desejado
    /*
        Wire.write(0b00000000); // fundo de escala em +/-2g
        Wire.write(0b00001000); // fundo de escala em +/-4g
        Wire.write(0b00010000); // fundo de escala em +/-8g
        Wire.write(0b00011000); // fundo de escala em +/-16g
    */
    Wire.beginTransmission(MPU);
    Wire.write(0x1C);
    Wire.write(0b00001000); // Trocar esse comando para fundo de escala desejado conforme acima
    Wire.endTransmission();

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
    valorRecebido = (char)Serial.read();
    if (valorRecebido == '1')
    {
        Serial.print("\nReiniciando o ESP32");
        delay(2000);
        ESP.restart();
    }
    // if (digitalRead(Botao_Reset) == 1)
    // {
    //     SerialBT.print("\nReiniciando o ESP32");
    //     delay(2000);
    //     ESP.restart();
    // }

    //-----------------------------

    // Lendo os dados do GPS
    float latitude, longitude, speed, altitude;
    unsigned long age;
    int year;
    byte month, day, hour, minute, second;

    // Solicita os dados ao GPS
    gps.f_get_position(&latitude, &longitude, &age);
    speed = gps.f_speed_kmph();
    altitude = gps.f_altitude();
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second);

    // Comandos para iniciar transmissão de dados
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 14, true); // Solicita os dados ao sensor

    // Armazena o valor dos sensores nas variaveis correspondentes
    AccX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    AccY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AccZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    Temp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    GyrX = Wire.read() << 8 | Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    GyrY = Wire.read() << 8 | Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    GyrZ = Wire.read() << 8 | Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

    /* Alterar divisão conforme fundo de escala escolhido:
        Acelerômetro
        +/-2g = 16384
        +/-4g = 8192
        +/-8g = 4096
        +/-16g = 2048

        Giroscópio
        +/-250°/s = 131
        +/-500°/s = 65.6
        +/-1000°/s = 32.8
        +/-2000°/s = 16.4
    */
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
    if (sensorCEBOLINHA == 0) // esta no modo teste
    {
        // Mapeando o valor do sensor para o valor do led, verificar amplitude do TPS
        outputVeTPS = map(sensorTPS1, 0, 516, 0, 255);
        outputATPS = map(sensorTPS1, 516, 1032, 0, 255);
        outputA2TPS = map(sensorTPS1, 1032, 1548, 0, 255);
        outputVTPS = map(sensorTPS1, 1548, 2090, 0, 255);

        // Controlando a taxa de variação dos LEDs
        if (sensorTPS1 <= 516)
        {
            outputATPS = 0;
            outputA2TPS = 0;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 1032)
        {
            outputVeTPS = 255;
            outputA2TPS = 0;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 1548)
        {
            outputVeTPS = 255;
            outputATPS = 255;
            outputVTPS = 0;
        }
        else if (sensorTPS1 <= 2090)
        {
            outputVeTPS = 255;
            outputATPS = 255;
            outputA2TPS = 255;
        }
        else if (sensorTPS1 >= 2090)
        {
            outputVeTPS = 255;
            outputATPS = 255;
            outputA2TPS = 255;
            outputVTPS = 255;
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

    // Exibir a velocidade e temperatura no LCD
    float TEMPC = ((Temp / 340.0) + 36.53) * 0.1; // Temperatura em graus Celsius
    lcd.setCursor(0, 0);
    lcd.print("Temp: "); // Exibir a temperatura
    lcd.print(TEMPC);
    lcd.setCursor(0, 1);
    lcd.print("          "); // Limpar a linha
    lcd.setCursor(0, 1);
    lcd.print("Vel: ");    // Exibir a velocidade
    lcd.print(velocidade); // Exibir a velocidade

    //-----------------------------
    // Armazena os dados para o cartao sd
    String date = String(day) + "/" + String(month) + "/" + String(year);
    String horario = String(hour) + ":" + String(minute) + ":" + String(second);

    // Resultados dos sensores Terminal
    Serial.print("\nData = "); // Data GMT 0
    Serial.println(date);
    Serial.print("\nHorario = "); // Horario GMT 0
    Serial.println(horario);

    Serial.print("\nTPS1 = ");
    Serial.println(sensorTPS1);
    Serial.print("\nTPS2 = ");
    Serial.println(sensorTPS2);
    Serial.print("\nCEBOLINHA = ");
    Serial.println(sensorCEBOLINHA);
    Serial.print("\nVELOCIDADE = ");
    Serial.println(velocidade);

    Serial.print("\nLATITUDE = ");
    Serial.println(latitude, 6);
    Serial.print("\nLONGITUDE = ");
    Serial.println(longitude, 6);
    Serial.print("\nVELOCIDADE GPS = ");
    Serial.println(speed); // KM/H
    Serial.print("\nALTITUDE = ");
    Serial.println(altitude);

    Serial.print("\nTEMPERATURA = ");
    Serial.println(((Temp / 340.0) + 36.53) * 0.1); // Temperatura em graus Celsius
    Serial.print("\nVELOCIDADE ANGULAR X = ");
    Serial.println(GyrX / 65.6); // Velocidade angular em graus por segundo, alterar a escala conforme escolhido
    Serial.print("\nVELOCIDADE ANGULAR Y = ");
    Serial.println(GyrY / 65.6); // Velocidade angular em graus por segundo, alterar a escala conforme escolhido
    Serial.print("\nVELOCIDADE ANGULAR Z = ");
    Serial.println(GyrZ / 65.6); // Velocidade angular em graus por segundo, alterar a escala conforme escolhido
    Serial.print("\nACELERAÇÃO X = ");
    Serial.println(AccX / 8192); // Aceleração em m/s², alterar a escala conforme escolhido
    Serial.print("\nACELERAÇÃO Y = ");
    Serial.println(AccY / 8192); // Aceleração em m/s², alterar a escala conforme escolhido
    Serial.print("\nACELERAÇÃO Z = ");
    Serial.println(AccZ / 8192); // Aceleração em m/s², alterar a escala conforme escolhido

    // Gravando os dados no cartão SD
    dataMessage = String(date) + "," + String(horario) + "," + String(sensorTPS1) + "," + String(sensorTPS2) + "," + String(sensorCEBOLINHA) + "," + String(velocidade) + "," + String(((Temp / 340) + 36.53) * 0.1) + "," + String(GyrX / 65.6) + "," + String(GyrY / 65.6) + "," + String(GyrZ / 65.6) + "," + String(AccX / 8192) + "," + String(AccY / 8192) + "," + String(AccZ / 8192) + "," + String(speed) + "," + String(latitude, 6) + "," + String(longitude, 6) + "," + String(altitude, 4) + "\r\n";
    Serial.print("Salvando Arquivos: ");
    Serial.println(dataMessage);
    WriteFile("/data.txt", dataMessage.c_str());
    // myFile.println(data);

    myFile.close();
    delay(500); // para dados de teste e visualização
}

void pulseCounter1()
{
    pulseCount1++;
}

void pulseCounter2()
{
    pulseCount2++;
}
