#include "BluetoothSerial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SparkFunTSL2561.h>
#include <Wire.h>
#include <RBDdimmer.h>
#include <BlynkSimpleEsp32_BT.h>

//Acima são as bibliotecas para o modulo bluetooth, modulo dimmer, aplicativo  e sensor
 
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define BLYNK_USE_DIRECT_CONNECT

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

// Acima estão as definições e a biblioteca que permitem o uso do app Blynk

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "DuCrj-k17zkAFCKGl6b3k2NHUC2SDGQJ";
 
//Parâmetros
const int zeroCrossPin  = 27;
const int acdPin  = 26;
double lux = 5000; // lux captado pelo sensor

//Objetos
dimmerLamp acd(acdPin,zeroCrossPin); //dimmer
SFE_TSL2561 light; //sensor
BluetoothSerial SerialBT; //bluetooth
 
//Variáveis
int power  = 30; // total de porcentagem de energia enviada para a lâmpada
int passo = 1; // passo do aumento ou redução da porcentagem

int lux_desejado_int_utilizado = 5000; //valor de lux inserido pelo usuário no app
int lux_desejado_int_utilizado_antigo = 5000; //valor de lux inserido pelo usuário no app do loop anterior

int trigger = 0; // valor que define se a lampada não precisa estar acesa

int botao = 0; // valor que define se o botao está On ou Off
	int botao_desligado = 0; // valor que indica se o botão está On ou Off

boolean gain; // Setando o gain d, 0 = X1, 1 = X16;
unsigned int ms; // Tempo em milissegundos do “shutter”

BLYNK_WRITE(V0)  // Pin virtual que define se o sistema liga ou desliga
{
  botao = param.asInt(); 
}

BLYNK_WRITE(V1) // Pin virtual que define o valor de lux definido pelo usuário
{
  lux_desejado_int_utilizado = param.asInt(); 
}

void sensorLuz(){ //função do sensor de luz. Retorna o valor em lux do ambiente
  unsigned int data0, data1;
  Serial.println("3");
  if (light.getData(data0,data1))
  {
    boolean good; // True if neither sensor is saturated
    good = light.getLux(gain,ms,data0,data1,lux);
    Serial.print(" lux: ");
    Serial.print(lux);
  }
}

void setup() {
  //Init Serial USB
  Serial.begin(9600);

  Serial.println("Waiting for connections...");

  Blynk.setDeviceName("Luminária"); // nome do dispositivo

  Blynk.begin(auth); // inicia conexão com aplicativo

  acd.begin(NORMAL_MODE, ON); // inicia conexão com o módulo dimmer
  
  light.begin(); // inicia conexão com sensor
   
  gain = 0; // gain = X1 para evitar saturação do sensor (em ambientes de brilho alto)
  
  unsigned char time = 2; // tempo no qual o sensor capta luz
 
  light.setTiming(gain,time,ms); // função que seta o tempo
 
  light.setPowerUp(); // Inicia o sensor

  acd.setState(OFF); // Lâmpada inicia desligado

}
 
void loop() {
  Blynk.run(); //Mantém a conexão com o aplicativo a cada loop
  if (botao==0){ //estado desligado
    acd.setState(OFF); // desliga lampada
    botao_desligado = 1; //trigger do botao desligado
  }
  if (botao==1){ //estado ligado
    
    if(botao_desligado == 1){ //lampada deve ligar se o botão ligar
      acd.setPower(power);
      botao_desligado = 0;
    }
    if(power<20){ //caso o valor da lâmpada chega abaixo de 20% (limite para a lâmpada LED), ele desliga
      acd.setState(OFF);
      trigger = 1; //trigger que indica que está desligado para power<20
    }
    
    if(power>80){ //para que a lâmpada não ultrapasse 80% (valor de lux ultrapassa 10000)
      power=80;
    }
  
    if (lux_desejado_int_utilizado != lux_desejado_int_utilizado_antigo){ //compara se o usuário colocou novo valor de lux desejado
      lux_desejado_int_utilizado_antigo = lux_desejado_int_utilizado; // atualiza o valor do lux desejado antigo
      if (trigger == 1){ // se a lâmpada estiver desligada porque o power foi abaixo de 20%, acende ela com 30%
        power = 30;
        acd.setPower(power);
        trigger = 0;
      }
    }
    
    sensorLuz(); // mede o lux do ambiente. Utilizado para comparar com a do usuário
    while (((lux_desejado_int_utilizado-lux)<=-500|(lux_desejado_int_utilizado-lux)>=500) && (power>=20 && power<=90)){ //loop atualiza até que o valor de lux do ambiente alcance o valor de lux desejado pelo usuário
      sensorLuz(); //  mede o lux do ambiente. Utilizado para comparar com a do usuário
      if ((lux_desejado_int_utilizado-lux)>0){ //se lux do ambiente é menor do que a especificada pelo usuário, aumenta intensidade da luz
        power = power+passo; // aumento da porcentagem de energia que passará pela lâmpada
        acd.setPower(power);  // lâmpada acende com a intensidade especificada por power
      }
      if ((lux_desejado_int_utilizado-lux)<0){
        power = power-passo; // diminuição da porcentagem de energia que passará pela lâmpada
        acd.setPower(power); // lâmpada acende com a intensidade especificada por power
      }
      delay(200); // para não sobrecarregar o sensor e o Esp32
    }
  }
  delay(100); // para não sobrecarregar o sensor e o Esp32
}