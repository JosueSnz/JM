#include <Arduino.h>
#include <BluetoothSerial.h>
#include <esp_system.h>
//#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// Bluetooth
BluetoothSerial SerialBT; // Define BluetoothSerial object
char valorRecebido; // Variável para armazenar o valor recebido
//-----------------------------

// Led Piloto
#define LED_vermelho 19  // Até 100% do TPS 
#define LED_azul 18  // Até 75% do TPS
#define LED_azul2 5  // Até 50% do TPS
#define LED_verde 9  // Até 25% do TPS 
//-----------------------------

// TPS 1
const int TPS1 = 32;  
int sensorTPS1 = 0;  // Variável para armazenar o valor lido do sensor       

// TPS 2
const int TPS2 = 33;  
int sensorTPS2 = 0;  // Variável para armazenar o valor lido do sensor

// Manipular leds
int outputVTPS = 0;
int outputATPS = 0;  
int outputA2TPS = 0;  
int outputVeTPS = 0; 
//-----------------------------


void setup() {

    // Inicializando os pinos dos LEDs
    pinMode(LED_vermelho, OUTPUT);
    pinMode(LED_azul, OUTPUT);
    pinMode(LED_azul2, OUTPUT);
    pinMode(LED_verde, OUTPUT);

    SerialBT.begin("esp32"); // Nome do dispositivo Bluetooth
}

void loop() {

    // Caso seja necessário reiniciar o ESP32
    valorRecebido = (char)SerialBT.read();
    if (valorRecebido == '1'){
        SerialBT.print("\nReiniciando o ESP32");
        delay(2000);
        ESP.restart();
    }

    // Lendo o valor do sensor TPS
    sensorTPS1 = analogRead(TPS1);
    sensorTPS2 = analogRead(TPS2);

    // Verificar se os valores estão dentro da margem de 5%
    if (abs(sensorTPS1 - sensorTPS2) <= 0.10 * sensorTPS1) {

        // Mapeando o valor do sensor para o valor do led, verificar amplitude do TPS
        outputVeTPS = map(sensorTPS1, 0, 1023, 0, 255);
        outputATPS = map(sensorTPS1, 1023, 2046, 0, 255);
        outputA2TPS = map(sensorTPS1, 2046, 3069, 0, 255);
        outputVTPS = map(sensorTPS1, 3069, 4095, 0, 255);

        // Controlando a taxa de variação dos LEDs
        if (sensorTPS1 <= 1023) {
            outputATPS = 0;
            outputA2TPS = 0;
            outputVTPS = 0;
        } else if (sensorTPS1 <= 2046) {
            outputA2TPS = 0;
            outputVTPS = 0;
        } else if (sensorTPS1 <= 3069) {
            outputVTPS = 0;
        }

        // Mudando a intensidade do led
        analogWrite(LED_vermelho, outputVTPS);
        analogWrite(LED_azul, outputATPS);
        analogWrite(LED_azul2, outputA2TPS);
        analogWrite(LED_verde, outputVeTPS);

    } else {
        // Caso os valores estejam fora da margem de 10% apenas o led vermelho é aceso (100% do TPS)
        SerialBT.print("\nOs valores dos sensores estão fora da margem de 10%");
        analogWrite(LED_vermelho, 255);
        analogWrite(LED_azul, 0);
        analogWrite(LED_azul2, 0);
        analogWrite(LED_verde, 0);
    }

    // Resultados dos sensores (trabalhar com csv)
    SerialBT.print("\nTPS1 = ");
    SerialBT.println(sensorTPS1);
    SerialBT.print("\nTPS2 = ");
    SerialBT.println(sensorTPS2);
    delay(150);
}
