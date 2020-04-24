 //=======================================================================================//
/*
 Radioenge Equip. de Telecom.
 Abril de 2020
 Código exemplo para uso dos módulos LoRaWAN RD49C com o ESP8266/NodeMCU + ThingSpeak _ Ativação OTAA
 
 Fontes consultadas:
https://circuits4you.com/2018/01/02/esp8266-timer-ticker-example/ 08-04-2020
https://stackoverflow.com/questions/5697047/convert-serial-read-into-a-useable-string-using-arduino //08/04/2020
https://www.filipeflop.com/blog/esp8266-com-thingspeak/ //13-04-2020

obs.: O módulo já deve estar cadastrado no NetworkServer, já deve ter sido configurada a máscara de canais, o APPEUI e AppKey vem como a GPIO 0 deve ter sido configurada como analog
A interface serial do módulo LoRaWAN deve ser conectada nos pinos D1 e D2 (D1-Pino1; D2-Pino 2), GND do ESP8266/NodeMCU e RD49C devem estar conectados. A alimentação pode ser provida
pelo nodeMCU 3v3->pino 4 do RD49C

Este código de exemplo verifica se o RD49C está autenticado no NetowrkServer. Se estiver transmite o valor da leitura de tensão no GPIO 0 (pino 6). Se não estiver autenticado ... pede autenticação (join).
 */
 
//inclusão do DHT11
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include<SoftwareSerial.h> 
//========SoftwareSerial==================
SoftwareSerial SWSerial(4, 5);  //D2, D1 : Será usada a serial por software para deixar livre a serial do monitor serial
//===============Timer====================
Ticker blinker;

// =====================WiFi======================
#define WIFI_SSID "SSID WiFi"
#define WIFI_PASSWORD "Senha WiFi"
//===============================================

//constantes e variáveis globais
char EnderecoAPIThingSpeak[] = "api.thingspeak.com";
String ChaveEscritaThingSpeak = "6S Chave escrita T4";
long lastConnectionTime; 
WiFiClient client;
int count = 0;
char inData[255]; // Buffer com tamanho suficiente para receber qualquer mensagem
char inChar=-1; // char para receber um caractere lido
byte indice = 0; // índice para percorrer o vetor de char = Buffer
int polingID=0;
char flag= false; //flag usada para avisar que a serial recebeu dados
bool flag_join = false;
bool flag_aux = false;
bool flag_gpio = false;
int leituraGPIO=0;
#define LoRaWANport 85
//===============================================================================
//Timer
void ICACHE_RAM_ATTR onTimerISR(){
    digitalWrite(LED_BUILTIN,!(digitalRead(LED_BUILTIN)));  //Toggle LED Pin
    timer1_write(600000);//12us
    count++;
}
//================================================================================

// ===================== protótipos das funções  =================================
char lerSerial();
char TrataRX(byte indice);
char LerGPIO(int id, char gpio);

//================================================================================


void setup() {

  Serial.begin(9600);
  SWSerial.begin(9600);
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("conectando");
  count=0;
  while ((WiFi.status() != WL_CONNECTED) && count<20) 
  {
    Serial.print("Conectando...\n");
    Serial.println(count);
    delay(500);
    count++;
    if(WiFi.status() == WL_CONNECTED)
      count = 20; //se conectar vai sair do while
  }
  Serial.println(WiFi.status());
  if(WiFi.status())
  {
    Serial.println();
    Serial.print("conectado: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("Falha na conexao WiFi");
  }
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output
  //Initialize Ticker every 0.5s
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(600000); //120 ms
}




// the loop function runs over and over again forever
void loop() {

  if(count % 60 == 0)// 250 = 30 segundos
  {
      SWSerial.print("AT+RPIN=0");//leitura GPIO 0 que deve estar configurado como analog
      flag_gpio = true;
  }
  if(count % 250 == 0)// 250 = 30 segundos
  {
      
      
      Serial.println(count%250);
      if(flag_join)//se == 1 fez o join
      {
         char mensagem[50] = {0};
         sprintf(mensagem,"AT+SEND=%d:field1=%.2f\n", LoRaWANport,(LoRaWANport, (3.3/4096)*leituraGPIO) );       
         SWSerial.print(mensagem);
         Serial.println(mensagem);
         Serial.println("Transmitiu Mensagem LoRa");
      }
      else
      {
        
        if(WiFi.status() == WL_CONNECTED)
          {
            char infoTTS[50] = {0};
            sprintf(infoTTS,"field1=%.2f", ((3.3/4096)*leituraGPIO) );
            EnvioTheThingSpeak(infoTTS);
          }
          else
          {
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
          }
          SWSerial.print("AT+NJS=?");//pergunta para o módulo se o estado do join
          flag_aux = true; //avisa que perguntou para o EndDevice se foi feito o Join?
          Serial.println("Perguntou para o EndDevice se já fez o join. ");
      }
       count++;
  }
    flag= LerSerial();
    if (flag)
    {
      TrataRX(indice);
    }

}

//=========================== função para vericar se recebeu alguma informação na serial por software pinos D1 e D2 ===================
char LerSerial( ) 
{
    indice=0;
    while (SWSerial.available() > 0) // Don't read unless// there you know there is data
    {
        if(indice < 254) // One less than the size of the array
        {
            inChar = SWSerial.read(); // Read a character
            inData[indice] = inChar; // Store it
            delay (10);
            indice++; // Increment where to write next
            inData[indice] = '\0'; // Null terminate the string
        }        
    }
    if(indice>0) 
    {
        return(1); //retorno da função avisando que recebeu dados na serial
    }
    else
    {
      return (0);
    }

}
//-----------------------------------------------------------------------------//

//================== tratamento do que recebeu na serial ========================
char TrataRX(byte indice) 
{
      Serial.println(inData); //mostra no console serial
      if( (strstr (inData, "AT_NO_NETWORK_JOINED")) || (strstr (inData, "AT_JOIN_ERROR") ))
      {
          Serial.println("Ainda nao \"joined\"");
          flag_join=false;
      }
      if(!flag_join) // se ainda nao fez ou nao sabe se fez join OTAA
      {
        if(strstr (inData, "AT_ALREADY_JOINED"))//Somente para Debug. Pode ser apagado.
        {
          Serial.println("Ja havia feito o join."); //Somente para Debug. Pode ser apagado.
          flag_join = true;
        }
        if(strstr (inData, "AT_JOIN_OK"))//Somente para Debug. Pode ser apagado.
        {
          Serial.println("Acabou de fazer o join");//Somente para Debug. Pode ser apagado.
          flag_join = true;
        }
        //flag_join = strstr (inData, "AT_ALREADY_JOINED") ||  strstr (inData, "AT_JOIN_OK"); // verifica se recebeu menssagens que indicam que já fez o join
        char *joined = 0;//Somente para Debug. Pode ser apagado.
        joined = strstr (inData, "AT_ALREADY_JOINED");//Somente para Debug. Pode ser apagado.
        Serial.println(joined);//Somente para Debug. Pode ser apagado.
        joined =  strstr (inData, "AT_JOIN_OK");//Somente para Debug. Pode ser apagado.
        Serial.println(joined);//Somente para Debug. Pode ser apagado.
        if(flag_aux)//se perguntou o join status 
        {
          if(inData[0]=='1')//e a resposta foi 1
          {
            flag_join = true;  
            Serial.println("Resposta do AT+JOIN recebida. Join ok... pode transmitir");//Somente para Debug. Pode ser apagado.
          }
          else
          {
            flag_join = false;
            Serial.println("flag_join = false");//Somente para Debug. Pode ser apagado.
          }
          flag_aux = false;
         }
         if(!flag_join) // se após ler a resposta na serial ainda não identificou que foi feito o join... pede para o EndDevice fazer o join novamente.
         {
          SWSerial.print("AT+JOIN");//pede join para o GW
            Serial.println("Join nao foi feito. pede join para o gw novamente.");//Somente para Debug. Pode ser apagado.
            delay (1000);
         }
      }
      if(flag_gpio)
      {
        leituraGPIO = atoi(&inData[2]);
        Serial.print("Valor em bits do AD: ");
        Serial.println(leituraGPIO);
        Serial.print("Tensao no AD: ");
        Serial.println((3.3/4096)*leituraGPIO);
        flag_gpio = false;
      }
}




//==========================================================================


//==========================================================================
char EnvioTheThingSpeak(String dados)
{
if (client.connect(EnderecoAPIThingSpeak, 80))
    {
        /* faz a requisição HTTP ao ThingSpeak */
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+ChaveEscritaThingSpeak+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(dados.length());
        client.print("\n\n");
        client.print(dados);
        Serial.println("- Informações enviadas ao ThingSpeak!");
        Serial.println(dados);
    }
}