/*
 * Projeto: Aquatech v.0.0.2
 * Autor: Felipe Santos
 * Data de Criação: 08/02/2016
 * 
 * Ideia: 
 * 1 - Fazer leitura da temperatura do aquario;
 * 2 - Acender as luzes do aquario; 
 * 3 - Trocar a cor das luzes do aquario (Fita LED RGB)
 * 4 - Logar a temperatura do aquario em intervalos de 15 minutos, afim de analisar os picos de temperaturas e produzir gráfico
 * 5 - Controlar a bomba de oxigenio * 
 * 
 * WiFiShield Lab
 * Baseado no exemplo httpserver da biblioteca WiFlyHQmaster
 *
 */

// Includes
#include <WiFlyHQ.h>
#include <SoftwareSerial.h>
// #include <Ultrasonic.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// #include <DS1307.h>

// Definições dos pinos da placa
#define Temp 2   // Sensor Temperatura
#define Rele1 7  // Módulo Rele - Define pino de saida para rele 1 
#define Rele2 8  // Módulo Rele - Define pino de saida para rele 2
#define SensorPin  10    // Sensor Temp
#define buzzer  11  // Módulo SD - MOSI


SoftwareSerial wifiSerial(4, 5);

//Modulo RTC DS1307 ligado as portas A4 e A5 do Arduino 
// DS1307 rtc(A4, A5);

WiFly wifly;
OneWire  ds(SensorPin);  // on pin 10 (a 4.7K resistor is necessary)

DallasTemperature sensors(&ds);

void erro404();

char buf[80];

const char site[] = "192.168.236.128/artefestas/index.php/principal/insere_temperatura/12.48";


int Luz = 0; //Variável para a leitura do LDR
int flagRele1 = 0;
int flagRele2 = 0;
int flagLed = 0;
int flagAutoLed = 0;
int flagAutoRele1 = 0;
float fTemperatura = 0;

unsigned long Inicio;

// Altere as duas variáveis abaixo de acordo com sua rede Wifi
const char rede[] = "America";
const char senha[] = "forcatotal";

void setup() {
  Serial.begin(115200);

  pinMode(buzzer,OUTPUT);

  sensors.begin();

  tone(buzzer,1500);   
  delay(750);
   
  //Desligando o buzzer.
  noTone(buzzer); 

  // armazena o tempo inicial no setup
  Inicio = millis(); 

  Serial.println(F("Inicializando"));
  Serial.print(F("Memoria livre: "));
  Serial.println(wifly.getFreeMemory(),DEC);
    
  // inicializa os reles
  pinMode(Rele1, OUTPUT);
  pinMode(Rele2, OUTPUT);
  digitalWrite(Rele1, LOW);
  digitalWrite(Rele2, LOW);

  // pega a temperatura
  fTemperatura = getTemperatura();

  wifiSerial.begin(9600);
  if (!wifly.begin(&wifiSerial, &Serial)) {
    Serial.println(F("Falha ao inicializar o wifly"));
    wifly.terminal();
  }

  //Se não estiver associado a uma rede configura a rede no módulo
  if (!wifly.isAssociated()) {
    Serial.println(F("Conectando..."));
    wifly.setSSID(rede);
    wifly.setPassphrase(senha);
    wifly.enableDHCP();
    wifly.setPort(80);    
    wifly.save();

    if (wifly.join()) {
      Serial.println(F("Conectado a Rede"));
    }  else {
      Serial.println(F("Falha ao conectar na rede"));
      wifly.terminal();
    }
  } else {
    Serial.println(F("Pronto para entrar na rede"));
  }

  // wifly.setBroadcastInterval(0);	// Desliga UPD broadcast

  Serial.print(F("MAC: "));
  Serial.println(wifly.getMAC(buf, sizeof(buf)));
  Serial.print(F("IP: "));
  Serial.println(wifly.getIP(buf, sizeof(buf)));
  wifly.setDeviceID("Arduino_wifly");

  if (wifly.isConnected()) {
    Serial.println(F("Fechando outras conexões ativas"));
    wifly.close();
  }

  wifly.setProtocol(WIFLY_PROTOCOL_TCP);
  if (wifly.getPort() != 80) {
    wifly.setPort(80);
    wifly.save();
    Serial.println(F("Trocando para a porta 80, reiniciando..."));
    wifly.reboot();    // Reiniciar o módulo para alterar a número da porta
    delay(3000);
  }

  tone(buzzer,1250);   
  delay(150);

  tone(buzzer,1500);   
  delay(250);
     
  //Desligando o buzzer.
  noTone(buzzer);  

  Serial.println(F("Pronto"));

}

void loop() {

 
  // modo automatico do led, atualiza a intensidade da fita a cada 3 segundo, de acordo com a luminosidade
  if ((millis() - Inicio) > 120000) {
    Inicio = millis();

    fTemperatura = getTemperatura();

    Serial.print(F("Temperatura: "));
    Serial.println(fTemperatura);

  }  
  
  if (wifly.available() > 0) 
  {
    if (wifly.gets(buf, sizeof(buf))) 
    {
      // funcao que verificar se o arduino esta respondendo
      if (strncmp_P(buf, PSTR("POST /esta_on"), sizeof("POST /esta_on")-1) == 0) {
          
          while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
          //
          Serial.println(F("esta_on:"));
          Serial.println(F("1#ARDUINOWS#estou on, pode mandar solicitacoes"));
          //
          sendCabecalhoHTML(wifly);
          wifly.sendChunkln(F("1#ARDUINOWS#estou on, pode mandar solicitacoes"));
          wifly.sendChunkln();

      // funcao get dados, retorna os dados dos sensores  
      }  else if (strncmp_P(buf, PSTR("POST /get_dados"), sizeof("POST /get_dados")-1) == 0) {

          while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
          //
          Serial.println(F("get_dados:"));
          Serial.println(strcat("1#ARDUINOWS#", getDados()));
          //
          sendCabecalhoHTML(wifly);
          wifly.sendChunkln("1#ARDUINOWS#Funcao executada com sucesso");
          wifly.sendChunkln(); 

      // funcao ligar rele1, liga rele1
      }  else if (strncmp_P(buf, PSTR("POST /ligar_rele1"), sizeof("POST /ligar_rele1")-1) == 0) {

          while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
          //
          ligarRele1();
          //
          Serial.println(F("ligar_rele1:"));
          Serial.println(F("1#ARDUINOWS#rele1 ligado"));
          //
          sendCabecalhoHTML(wifly);
          wifly.sendChunkln("1#ARDUINOWS#rele1 ligado");
          wifly.sendChunkln(); 

      // funcao desligar rele1, desliga rele1
      }  else if (strncmp_P(buf, PSTR("POST /desligar_rele1"), sizeof("POST /desligar_rele1")-1) == 0) {

          while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
          //
          desligarRele1();
          //
          Serial.println(F("desligar_rele1:"));
          Serial.println(F("1#ARDUINOWS#rele1 desligado"));
          //
          sendCabecalhoHTML(wifly);
          wifly.sendChunkln("1#ARDUINOWS#rele1 desligado");
          wifly.sendChunkln(); 


      // funcao ligar rele2, liga rele2
      }  else if (strncmp_P(buf, PSTR("POST /ligar_rele2"), sizeof("POST /ligar_rele2")-1) == 0) {

          while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
          //
          ligarRele2();
          //
          Serial.println(F("ligar_rele2:"));
          Serial.println(F("1#ARDUINOWS#rele2 ligado"));
          //
          sendCabecalhoHTML(wifly);
          wifly.sendChunkln("1#ARDUINOWS#rele2 ligado");
          wifly.sendChunkln(); 

      // funcao desligar rele2, desliga rele2
      }  else if (strncmp_P(buf, PSTR("POST /desligar_rele2"), sizeof("POST /desligar_rele2")-1) == 0) {

          while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
          //
          desligarRele2();
          //
          Serial.println(F("desligar_rele2:"));
          Serial.println(F("1#ARDUINOWS#rele2 desligado"));
          //
          sendCabecalhoHTML(wifly);
          wifly.sendChunkln("1#ARDUINOWS#rele2 desligado");
          wifly.sendChunkln(); 
      
      // nao achou nenhuma funcao, retorna mensagem
      } else {
        while (wifly.gets(buf, sizeof(buf)) > 0) { /* Ignora o restante da requisição */ }
        //
        Serial.println(F("Funcao nao encontrada"));
        Serial.println(F("0#ARDUINOWS#Funcao nao encontrada"));
        //
        sendCabecalhoHTML(wifly);
          wifly.sendChunkln("0#ARDUINOWS#Funcao nao encontrada");
          wifly.sendChunkln();     
      }
    }
  }
}  
  

// Envia página de erro caso seja digitado uma URL inválida
void sendCabecalhoHTML(WiFly wifly)
{
  // Envia Cabeçalho HTML
  wifly.println(F("HTTP/1.1 200 OK"));
  wifly.println(F("Content-Type: text/html"));
  wifly.println(F("Cache-Control: No-Cache"));  
  wifly.println(F("Connection: keep-alive"));
  wifly.println(F("Age: 0"));  
  wifly.println(F("Content-Type: application/json; charset=utf-8"));    
  wifly.println(F("Transfer-Encoding: chunked"));
  wifly.println();
}

void ligarRele1() {
  digitalWrite(Rele1, HIGH);
  flagRele1 = 1;
}

void desligarRele1() {
  digitalWrite(Rele1, LOW);
  flagRele1 = 0;
}

void ligarRele2() {
  digitalWrite(Rele2, HIGH);
  flagRele2 = 1;
}

void desligarRele2() {
  digitalWrite(Rele2, LOW);
  flagRele2 = 0;
}

void LerDados(WiFly novoCliente) {
  char strTemperatura[3], strFlagLed[2], strFlagRele1[2], strFlagAutoLed[2], strFlagAutoRele1[2];
  char strResult[20] = "";
  int iTemperatura;
  
  iTemperatura = (int) fTemperatura; 
     
  snprintf(strTemperatura, 3, "%d", iTemperatura); 
  sprintf(strFlagLed,         "%d", flagLed);
  sprintf(strFlagRele1,       "%d", flagRele1); 
  sprintf(strFlagAutoLed,     "%d", flagAutoLed); 
  sprintf(strFlagAutoRele1,   "%d", flagAutoRele1);  

  strcat(strResult, "0|");
  strcat( strcat(strResult , strTemperatura ),  "|" );
  strcat( strcat(strResult , strFlagLed),       "|");
  strcat( strcat(strResult , strFlagRele1),     "|");
  strcat( strcat(strResult , strFlagAutoLed),   "|");
  strcat( strcat(strResult , strFlagAutoRele1), "|");  
  
  sendCabecalhoHTML(novoCliente);
  
  novoCliente.sendChunkln(strResult);
  novoCliente.sendChunkln();    

  Serial.println(F("LerDados result:"));
  Serial.println(strResult);
  // espera-se receber algo como 50|99|1|1|1|1| - luminosidade|temperatura|flagLed|flagRele1|flagAutoLed
}

char* getDados() {
  static char array[] = "minha array";
  return array;
}

float getTemperatura(){
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
