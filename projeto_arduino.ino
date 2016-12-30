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
      // função le dados, retorna os dados das variaveis e sensores
      if (strncmp_P(buf, PSTR("GET /ajax_LerDados"), 18) == 0) {
        Serial.println(F("ajax_LerDados"));
        while (wifly.gets(buf, sizeof(buf)) > 0) 
        {
          // Ignora o restante da requisição
        }

        LerDados(wifly);

      // função luz, liga ou desliga a luz
      } else if (strncmp_P(buf, PSTR("GET /ajax_carga1"), 16) == 0) {
        Serial.println(F("ajax_carga1"));        
        while (wifly.gets(buf, sizeof(buf)) > 0) 
        {
          // Ignora o restante da requisição
        }

        flagAutoLed = 0;

        if (flagLed == 0) {
          digitalWrite(Rele2, HIGH);          
          flagLed = 1;
        } else {
          digitalWrite(Rele2, LOW);
          flagLed = 0;  
        }

        LerDados(wifly);

      // Verifica se foi feito um POST
      } else if (strncmp_P(buf, PSTR("GET /ajax_carga2"), 16) == 0) {
        Serial.println(F("ajax_carga2"));        
        while (wifly.gets(buf, sizeof(buf)) > 0) 
        {
          // Ignora o restante da requisição
        }

        flagAutoRele1 = 0;

        if (flagRele1 == 0) {
          digitalWrite(Rele1, HIGH);
          flagRele1 = 1;
        } else {
          digitalWrite(Rele1, LOW);
          flagRele1 = 0;  
        }

        LerDados(wifly);

      // Verifica se foi feito um POST
      } else if (strncmp_P(buf, PSTR("GET /ajax_carga3"), 16) == 0) {
        Serial.println(F("ajax_carga3"));         
        while (wifly.gets(buf, sizeof(buf)) > 0) 
        {
          // Ignora o restante da requisição
        }

        if (flagAutoLed == 0) {
          flagAutoLed = 1;
        } else {
          flagAutoLed = 0;  
        }

        LerDados(wifly);

      // Verifica se foi feito um POST
      }  else if (strncmp_P(buf, PSTR("GET /ajax_carga4"), 16) == 0) {
        Serial.println(F("ajax_carga4"));         
        while (wifly.gets(buf, sizeof(buf)) > 0) 
        {
          // Ignora o restante da requisição
        }

        if (flagAutoRele1 == 0) {
          flagAutoRele1 = 1;
        } else {
          flagAutoRele1 = 0;  
        }

        LerDados(wifly);

      // Verifica se foi feito um POST
      } else {
        Serial.println(F("pagina principal"));
        while (wifly.gets(buf, sizeof(buf)) > 0) 
        {
          // Ignora o restante da requisição
        }
               
        pag_principal(); // Pula para a rotina que imprime a página principal        
      }
    }
  }
}  
  
// Envia a página principal
void pag_principal()
{
  /* Envia Cabeçalho HTML*/
  sendCabecalhoHTML(wifly);

wifly.sendChunkln(F("<!DOCTYPE html>"));
wifly.sendChunkln(F("<html>"));
  wifly.sendChunkln(F("<head> "));
    wifly.sendChunkln(F("<title>Aquaino</title>    "));
    wifly.sendChunkln(F("<meta name=\"viewport\" content=\"width=device-width\"; intial-scale=1; maximum-scale=1>   "));
    wifly.sendChunkln(F("<script type=\"text/javascript\">      "));

    
      wifly.sendChunkln(F("function LerSensores() {"));
      wifly.sendChunkln(F("  var request = new XMLHttpRequest();"));        
      wifly.sendChunkln(F("  request.onreadystatechange = function() {"));
      wifly.sendChunkln(F("    if (this.readyState == 4) {"));
      wifly.sendChunkln(F("      if (this.status == 200) {"));
      wifly.sendChunkln(F("        if (this.responseText != null ) {"));
      wifly.sendChunkln(F("          var dados = this.responseText.split('|'); "));
      wifly.sendChunkln(F("          document.getElementById(\"sensor1\").innerHTML = dados[1];"));
      wifly.sendChunkln(F("          if (dados[2] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga1\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga1\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));
      wifly.sendChunkln(F("          if (dados[3] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga2\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga2\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));  
      wifly.sendChunkln(F("          if (dados[4] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga3\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga3\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));   
      wifly.sendChunkln(F("          if (dados[5] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga4\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga4\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));                      
      wifly.sendChunkln(F("        }"));
      wifly.sendChunkln(F("      }"));
      wifly.sendChunkln(F("    }"));
      wifly.sendChunkln(F("  }"));
      wifly.sendChunkln(F("  request.open(\"GET\", \"ajax_LerDados\", true);"));  
      wifly.sendChunkln(F("  request.send(null);"));              
        wifly.sendChunkln(F("setTimeout('LerSensores()', 45000);"));                
      wifly.sendChunkln(F("}      "));

      wifly.sendChunkln(F("function BotaoCarga(numero) {"));
      wifly.sendChunkln(F("  var request = new XMLHttpRequest();"));
      wifly.sendChunkln(F("  if (numero == 1) { "));
      wifly.sendChunkln(F("    request.open(\"GET\", \"ajax_carga1\", true); "));
      wifly.sendChunkln(F("  } else if (numero == 2) { "));
      wifly.sendChunkln(F("    request.open(\"GET\", \"ajax_carga2\", true); "));
      wifly.sendChunkln(F("  } else if (numero == 3) { "));
      wifly.sendChunkln(F("    request.open(\"GET\", \"ajax_carga3\", true); "));
      wifly.sendChunkln(F("  } else if (numero == 4) { "));
      wifly.sendChunkln(F("    request.open(\"GET\", \"ajax_carga4\", true); "));      
      wifly.sendChunkln(F("  } else { "));
      wifly.sendChunkln(F("    request.open(\"GET\", \"ajax_LerDados\", true); "));
      wifly.sendChunkln(F("  } "));
      wifly.sendChunkln(F("  request.send(null); "));      
      wifly.sendChunkln(F("  request.onreadystatechange = function() {"));
      wifly.sendChunkln(F("    if (this.readyState == 4) {"));
      wifly.sendChunkln(F("      if (this.status == 200) {"));
      wifly.sendChunkln(F("        if (this.responseText != null ) {"));  
      wifly.sendChunkln(F("          var dados = this.responseText.split('|'); "));
      wifly.sendChunkln(F("          document.getElementById(\"sensor1\").innerHTML = dados[1];"));
      wifly.sendChunkln(F("          if (dados[2] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga1\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga1\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));
      wifly.sendChunkln(F("          if (dados[3] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga2\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga2\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));  
      wifly.sendChunkln(F("          if (dados[4] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga3\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga3\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } ")); 
      wifly.sendChunkln(F("          if (dados[5] == 0) { "));
      wifly.sendChunkln(F("            document.getElementById(\"carga4\").style.backgroundColor = \"#FF9696\"; "));
      wifly.sendChunkln(F("          } else {  "));
      wifly.sendChunkln(F("            document.getElementById(\"carga4\").style.backgroundColor = \"#ADFF85\";"));
      wifly.sendChunkln(F("          } "));        
      wifly.sendChunkln(F("        }"));
      wifly.sendChunkln(F("      }"));
      wifly.sendChunkln(F("    }"));
      wifly.sendChunkln(F("  }"));       
      wifly.sendChunkln(F("} "));
      
    wifly.sendChunkln(F("</script>    "));
    wifly.sendChunkln(F("<style type=\"text/css\">"));
      wifly.sendChunkln(F("div > h2 { color: #00979C; }"));
      wifly.sendChunkln(F("body { font-family: Arial; margin: 0px; }"));     
      wifly.sendChunkln(F(".header { width: 100%; height: 110px; border: 10px; background-color: #F6F9F9; margin-top: 0px; }"));
      wifly.sendChunkln(F(".principal { width: 480px; height: 225px; margin-top: 20px; }"));
      wifly.sendChunkln(F(".quadrado { float:left; border-style: solid; border-color: #00979C; background-color: #F6F9F9; width: 150px; "));
        wifly.sendChunkln(F("height: 215px; text-align: center; }"));
      wifly.sendChunkln(F(".quadrado2 { float: left; width: 150px; height: 155px; border-style: solid; border-color: #00979C; "));        
        wifly.sendChunkln(F("margin-left: 5px; margin-bottom: 5px; text-align: center; cursor: pointer; }"));
      wifly.sendChunkln(F(".quadrado3 { float: left; width: 150px; height: 50px; border-style: solid; border-color: #00979C; "));  
        wifly.sendChunkln(F("margin-left: 5px; text-align: center; cursor: pointer; }"));            
      wifly.sendChunkln(F("h5 { margin-top: 55px; margin-bottom: 0px; color: #00979C; font-size: 18px; }"));
      wifly.sendChunkln(F("p { font-size: 50px; margin: 0px; color: #00979C; }"));
      wifly.sendChunkln(F(".carga { font-size: 30px; font-weight: bold; margin-top: 55px; }"));
      wifly.sendChunkln(F(".carga2 { font-size: 16px; font-weight: bold; margin-top: 15px; }"));
      wifly.sendChunkln(F("#botao1 { border-style: solid; border-color: #00979C; background-color: #F6F9F9;"));
        wifly.sendChunkln(F("width: 150px; height: 150px; text-align: center; display: inline-block; margin-bottom: 5px; cursor: pointer; }"));
    wifly.sendChunkln(F("</style>"));
    
  wifly.sendChunkln(F("</head>"));
  wifly.sendChunkln(F("<body onload=\"LerSensores()\">"));
    wifly.sendChunkln(F("<center>"));
    
      wifly.sendChunkln(F("<div class=\"header\">"));
      wifly.sendChunkln(F("</br>"));
        wifly.sendChunkln(F("<div>"));
          wifly.sendChunkln(F("<h2>Measurement system</h2>"));
        wifly.sendChunkln(F("</div>"));
      wifly.sendChunkln(F("</br>  "));
      wifly.sendChunkln(F("</div>     "));
    wifly.sendChunkln(F("</br>      "));

    wifly.sendChunkln(F("<div class=\"principal\">"));
    
      wifly.sendChunkln(F("<div class=\"quadrado\" onclick=\"LerSensores(1)\"><h5>Temperatura:</h5><p id=\"sensor1\">25&#176;</p></div>"));
      
      wifly.sendChunkln(F("<div class=\"quadrado2\" onclick=\"BotaoCarga(1)\" id=\"carga1\" ><p class=\"carga\">Luz </p></div>"));
      wifly.sendChunkln(F("<div class=\"quadrado2\" onclick=\"BotaoCarga(2)\" id=\"carga2\" ><p class=\"carga\">Bomba O&sup2;</p></div>"));

      wifly.sendChunkln(F("<div class=\"quadrado3\" onclick=\"BotaoCarga(3)\" id=\"carga3\" ><p class=\"carga2\">Modo Auto</p></div>"));
      wifly.sendChunkln(F("<div class=\"quadrado3\" onclick=\"BotaoCarga(4)\" id=\"carga4\" ><p class=\"carga2\">Modo Auto</p></div>"));

      
    wifly.sendChunkln(F("</div>"));
    wifly.sendChunkln(F("</br> "));
    wifly.sendChunkln(F("</center>"));
  wifly.sendChunkln(F("</body>"));
  wifly.sendChunkln(F("</html>"));
  wifly.sendChunkln();
}

// Envia página de erro caso seja digitado uma URL inválida
void sendCabecalhoHTML(WiFly novoCliente)
{
  // Envia Cabeçalho HTML
  novoCliente.println(F("HTTP/1.1 200 OK"));
  novoCliente.println(F("Content-Type: text/html"));
  novoCliente.println(F("Transfer-Encoding: chunked"));
  novoCliente.println();
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

float getTemperatura(){
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
