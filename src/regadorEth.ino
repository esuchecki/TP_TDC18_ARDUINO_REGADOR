#include <EtherCard.h>


static byte myMac[] = { 0xDD, 0xDD, 0xDD, 0x00, 0x01, 0x05 };
//static byte myMac[] = { 0x0A, 0xDD, 0xDD, 0x00, 0x01, 0x05 }; //MAC-ROUTER
static byte myIp[] = { 192, 168, 1, 177 };
byte Ethernet::buffer[1500];
boolean weHaveToSendResponse=false;


typedef struct SensorPlanta {
  int pinAnalogInput;
  int pinDigitalOutput;
  int humedadMin;                     //Start the water bomb.
  int humedadMax;                     //Stop the water bomb.
  boolean estamosRegando;             //Status of Digital Output
  int humedadActual;
};
//Creo mi estructura global tipo "SensorPlanta" llamada jazmin.
SensorPlanta jazmin;


const char* redColor = "'#DA0000'";
const char* greenColor = "'#009951'";
const int delayCheckSensor = 2000;      //in miliseconds
const int maxAnalogValue= 1023;         //From 0 to 1023.
const int maxHumedadValue=100;


//Defino en que pin tengo la entrada analogica.
//A0=Potenciometro, A1=SNR_HUM.
#define OUTPUT_ANALOG_JAZMIN A1


//El modulo de Relay funciona invertido.
//con un LOW, ENCIENDE... y con un HIGH apaga
//podemos parametrizarlo desde aca...!
#define OUTPUT_START_WATER LOW
#define OUTPUT_STOP_WATER HIGH



void setup() {

  Serial.begin(9600);
  
  if (!ether.begin(sizeof Ethernet::buffer, myMac, 10)) {
    Serial.println("ETH_ERROR");
  }
  else {
    Serial.println("ETH OK!");
  }

  if (!ether.staticSetup(myIp)) {
    Serial.println("ETH_ERROR");
  }
  
  initVarsForJazmin();

  
  //Me genero un GND para el SNR_HUM
  pinMode(4,OUTPUT);
  digitalWrite(4,LOW);
}



void loop()
{
  //loopInput();
  //loopOutput();


  checkJazminDelayAndDo();
  checkIncomingTcpPacket();
  sendResponse();
}








static word showWebRegador ()
{
  //"<title>Regador</title>"
  //"<meta http-equiv='refresh' content='2'>"
  //"<h4>TDC 2C2018</h4>"
  
  #define HTML_RESPONSE  "<!DOCTYPE html>" \
                          "<html><head>" \
                          "<title>TDC</title>" \
                          "<meta http-equiv='refresh' content='4'>" \
                          "</head>" \
                          "<body><div style='text-align:center;'>" \
                          "" \
                          "<table style='margin-left: auto; margin-right: auto;' border='1'>" \
                          "<tbody>" \
                          "<tr>" \
                          "<td bgcolor='DBDBDB' width='30%'></td>" \
                          "<td bgcolor='DBDBDB' width='70%'>TDC 2C2018</td>" \
                          "</tr>" \
                          "<tr>" \
                          "<td bgcolor=$S>" \
                          "<p>$S</p>" \
                          "<p>" \
                          "<a href='./?status=00&'><input type='button' value='OFF'></a>" \
                          "<a href='./?status=01&'><input type='button' value='ON'></a>" \
                          "</p>" \
                          "</td>" \
                          "<td>Humedad Actual: $D %" \
                          "<p><progress value='$D' max='100'></progress></p>" \
                          "</td>" \
                          "</tr>" \
                          "<tr>" \
                          "<td colspan='2' bgcolor='DBDBDB'><em>Cambiar Parametros de Riego: (Humedad)</em></td>" \
                          "</tr>" \
                          "<tr>" \
                          "<td>" \
                          "<p>Comenzar Riego:</p>" \
                          "<p>Actual: $D</p>" \
                          "</td>" \
                          "<td>" \
                          "<div class='slidecontainer'>" \
                          "<p><input id='iHumMin' class='slider' max='100' min='10' type='range'/></p>" \
                          "<p> <span id='sHumMin'></span> %</p>" \
                          "</div>" \
                          "</td>" \
                          "</tr>" \
                          "<tr>" \
                          "<td>" \
                          "<p>Detener Riego:</p>" \
                          "<p>Actual: $D</p>" \
                          "</td>" \
                          "<td>" \
                          "<div class='slidecontainer'>" \
                          "<p><input id='iHumMax' class='slider' max='100' min='10' type='range'/></p>" \
                          "<p> <span id='sHumMax'></span> %</p>" \
                          "</div>" \
                          "</td>" \
                          "</tr>" \
                          "<tr>" \
                          "<td colspan='2'>" \
                          "<a href='#' onclick='hum();'><input type='button' value='SAVE'></a>" \
                          "</td>" \
                          "</tr>" \
                          "</tbody>" \
                          "</table>" \
                          "</div>" \
                          "<script>" \
                          "iHumMin.oninput=function(){sHumMin.innerHTML=this.value;}\n" \
                          "iHumMax.oninput=function(){sHumMax.innerHTML=this.value;}\n" \
                          "function hum(){document.location.href='./?humMin='+iHumMin.value+'&humMax='+iHumMax.value+'&'};" \
                          "</script>" \
                          "</body></html>"

  
  //Color (ROJO,VERDE) -string
  //Estado (ON_OFF) -string
  //Nivel_humedad (1-100) -numero
  //Nivel_humedad (1-100) -numero
  //Umbral_comenzar_riego - numero
  //Umbral_detener_riego - numero

  BufferFiller bfill = ether.tcpOffset();
  bfill.emit_p(PSTR
                    (HTML_RESPONSE),
                    GET_HTML_COLOR(),
                    GET_HTML_STATUS(),
                    jazmin.humedadActual,
                    jazmin.humedadActual,
                    jazmin.humedadMin,
                    jazmin.humedadMax
                    );
  return bfill.position();
}





void initVarsForJazmin (){
  jazmin.pinAnalogInput=OUTPUT_ANALOG_JAZMIN;
  jazmin.pinDigitalOutput=2;
  jazmin.humedadMin=10;
  jazmin.humedadMax=90;
  jazmin.humedadActual=50;

  pinMode(jazmin.pinAnalogInput, INPUT);
  pinMode(jazmin.pinDigitalOutput, OUTPUT);
  

  //We make this to call stop Water (the first time)!.
  jazmin.estamosRegando=true;
  stopWater();
}


void readJazminHumedad () {
  // Leo la entrada analogica correspondiente, y le adapto el rango.
  // Rango Analogico: [0-1023] -->> Rango Humedad: [0-100]
  jazmin.humedadActual=(int) (( ((double) analogRead(jazmin.pinAnalogInput)) / maxAnalogValue)* maxHumedadValue );

  //Hay una pequeña observacion.... El sensor de humedad funciona asi:
  //0=HUMEDAD_MAXIMA -->> 100=TOTALMENTE SECO.
  //Por lo tanto tengo que adaptar la escala de lectura al reves....
  jazmin.humedadActual = maxHumedadValue - jazmin.humedadActual;

  
  Serial.println(jazmin.humedadActual);
}


void checkJazminDelayAndDo () {
  static long currentJazminTime;
  
  if (millis() - currentJazminTime >= delayCheckSensor) {
    Serial.println(">SNSR!");
    
    readJazminHumedad();
    checkHumedadLevelAndStartStopOutput();
    currentJazminTime = millis();           //Set the new value.
  }
}


void checkHumedadLevelAndStartStopOutput () {
  if ( jazmin.humedadActual <= jazmin.humedadMin ) {
    startWater();
  }
  if ( jazmin.humedadActual >= jazmin.humedadMax ) {
    stopWater();
  }
}


char* GET_HTML_COLOR() {
  if(jazmin.estamosRegando) {
    return greenColor;
  }else {
    return redColor;
  }
}


char* GET_HTML_STATUS() {
  if(jazmin.estamosRegando) {
    return "ON";
  }else {
    return "OFF";
  }
}


void checkIncomingTcpPacket()
{
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos)
  {
    weHaveToSendResponse=true;
    char* incomingTcpPacket = (char *)Ethernet::buffer + pos;

    Serial.println(incomingTcpPacket);

    #define GET_HEADER "GET /?"
    #define GET_TOKEN_AND "&"
    #define GET_STATUS "status"                   //=01 (two digits)
    #define GET_HUM_MIN "humMin"                  //=99 (two digits)
    #define GET_HUM_MAX "humMax"                  //=99 (two digits)
    #define GET_TOKEN_LENGHT 6                    //lenght of GET_STATUS, GET_HUM_MIN, GET_HUM_MAX
    #define GET_HEADER_LENGHT 6                   //lenght of GET_HEADER
    #define GET_TOKEN_VALUE 2                     //maximun number value of GET_STATUS, GET_HUM_MIN, GET_HUM_MAX
    //We wait for: "GET /?status=01" or "GET /?HUM_MIN=XX&HUM_MAX=YY" values.

    if ( strstr(incomingTcpPacket, GET_HEADER) != 0) {
      incomingTcpPacket = incomingTcpPacket + GET_HEADER_LENGHT;
      char* nextToken = strstr( incomingTcpPacket, GET_TOKEN_AND );
      
      while (nextToken != 0) {
        incomingTcpPacket[ GET_TOKEN_LENGHT ]='\0';
        char key[ GET_TOKEN_LENGHT ];
        strcpy(key, incomingTcpPacket);
        incomingTcpPacket = incomingTcpPacket + GET_TOKEN_LENGHT +1;
        
        incomingTcpPacket[ GET_TOKEN_VALUE ]='\0';
        char value[ GET_TOKEN_VALUE ];
        strcpy(value, incomingTcpPacket);
        incomingTcpPacket = incomingTcpPacket + GET_TOKEN_VALUE +1;

        Serial.println(key);
        Serial.println(value);
        
        //Estan queriendo forzar el ON/OFF?
        if ( strcmp( key, GET_STATUS ) ==0 ){
          if ( strcmp( value, "01" ) ==0 ) {
            startWater();
          }else {
            stopWater();
          }
        }

        //Quieren modificar el minimo de humedad?
        if ( strcmp( key, GET_HUM_MIN ) ==0 ){
          jazmin.humedadMin = atoi(value);
        }
        
        //Quieren modificar el maximo de humedad?
        if ( strcmp( key, GET_HUM_MAX ) ==0 ){
          jazmin.humedadMax = atoi(value);
        }

        nextToken = strstr( incomingTcpPacket, GET_TOKEN_AND );
      }
      
      
    }
  }
}


void sendResponse()
{
  if (weHaveToSendResponse)
  {
    weHaveToSendResponse=false;
    ether.httpServerReply(showWebRegador());
  }
}


void startWater () {
  if (!jazmin.estamosRegando) {
    Serial.println("WATER_ON");
    jazmin.estamosRegando=true;
    digitalWrite(jazmin.pinDigitalOutput, OUTPUT_START_WATER);
  }
}


void stopWater () {
  if (jazmin.estamosRegando) {
    Serial.println("WATER_OFF");
    jazmin.estamosRegando=false;
    digitalWrite(jazmin.pinDigitalOutput, OUTPUT_STOP_WATER);
  }
}





















//void loopInput()
//{
//  // wait for an incoming TCP packet, but ignore its contents
//  if (ether.packetLoop(ether.packetReceive()))
//  {
//    ether.httpServerReply(showWebInputStatus());
//  }
//}
//
//
//
//void loopOutput()
//{
//  word len = ether.packetReceive();
//  word pos = ether.packetLoop(len);
//
//  if (pos)
//  {
//    if (strstr((char *)Ethernet::buffer + pos, "GET /?data1=0") != 0) {
//      Serial.println("Led1 OFF");
//      digitalWrite(pinLed1, LOW);
//      statusLed1 = "OFF";
//    }
//
//    if (strstr((char *)Ethernet::buffer + pos, "GET /?data1=1") != 0) {
//      Serial.println("Led1 ON");
//      digitalWrite(pinLed1, HIGH);
//      statusLed1 = "ON";
//    }
//
//    if (strstr((char *)Ethernet::buffer + pos, "GET /?data2=0") != 0) {
//      Serial.println("Led2 OFF recieved");
//      digitalWrite(pinLed2, LOW);
//      statusLed2 = "OFF";
//    }
//
//    if (strstr((char *)Ethernet::buffer + pos, "GET /?data2=1") != 0) {
//      Serial.println("Led2 ON");
//      digitalWrite(pinLed2, HIGH);
//      statusLed2 = "ON";
//    }
//
//
//    ether.httpServerReply(showWebOutputs());
//  }
//}
//
//
//
//static word showWebInputStatus()
//{
//  BufferFiller bfill = ether.tcpOffset();
//  //   bfill.emit_p(PSTR("HTTP/1.0 200 OKrn"
//  //      "Content-Type: text/htmlrnPragma: no-cachernRefresh: 5rnrn"
//  //      "<html><head><title>Luis Llamas</title></head>"
//  //      "<body>"
//  //      "<div style='text-align:center;'>"
//  //      "<h1>Entradas digitales</h1>"
//  //      "</body></html>"));
//
//  bfill.emit_p(PSTR("<!DOCTYPE html>"
//                    "<html><head><title>INPUT STATUS</title></head>"
//                    "<body>"
//                    "<div style='text-align:center;'>"
//                    "<h1>Entradas digitales</h1>"
//                    "Tiempo transcurrido : $L s"
//                    "<br /><br />D00: $D"
//                    "<br /><br />D01: $D"
//                    "<br /><br />D02: $D"
//                    "<br /><br />D03: $D"
//                    "<br /><br />D04: $D"
//                    "<br /><br />D05: $D"
//                    "<br /><br />D06: $D"
//                    "<br /><br />D07: $D"
//                    "<br /><br />D08: $D"
//                    "<br /><br />D09: $D"
//                    "<br /><br />D10: $D"
//                    "<br /><br />D11: $D"
//                    "<br /><br />D12: $D"
//                    "<br /><br />D13: $D"
//
//                    "<h1>Entradas analogicas</h1>"
//                    "<br /><br />AN0: $D"
//                    "<br /><br />AN1: $D"
//                    "<br /><br />AN2: $D"
//                    "<br /><br />AN3: $D"
//                    "<br /><br />AN4: $D"
//                    "<br /><br />AN5: $D"
//                    "<br /><br />AN6: $D"
//                    "<br /><br />"
//                    "</body></html>"),
//               millis() / 1000,
//               digitalRead(0),
//               digitalRead(1),
//               digitalRead(2),
//               digitalRead(3),
//               digitalRead(4),
//               digitalRead(5),
//               digitalRead(6),
//               digitalRead(7),
//               digitalRead(8),
//               digitalRead(9),
//               digitalRead(10),
//               digitalRead(11),
//               digitalRead(12),
//               digitalRead(13),
//               analogRead(0),
//               analogRead(1),
//               analogRead(2),
//               analogRead(3),
//               analogRead(4),
//               analogRead(5),
//               analogRead(6));
//
//  return bfill.position();
//}
//
//
//
//static word showWebOutputs()
//{
//  BufferFiller bfill = ether.tcpOffset();
//  bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\n"
//                    "Content-Type: text/html\r\nPragma: no-cachernRefresh: 5\r\n\r\n"
//                    "<html><head><title>OUTPUT STATUS</title></head>"
//                    "<body>"
//                    "<div style='text-align:center;'>"
//                    "<h1>Salidas digitales</h1>"
//                    "<br /><br />Estado LED 1 = $S<br />"
//                    "<a href='./?data1=0'><input type='button' value='OFF'></a>"
//                    "<a href='./?data1=1'><input type='button' value='ON'></a>"
//                    "<br /><br />Estado LED 2 = $S<br />"
//                    "<a href='./?data2=0'><input type='button' value='OFF'></a>"
//                    "<a href='./?data2=1'><input type='button' value='ON'></a>"
//                    "<br /></div>\n</body></html>"), statusLed1, statusLed2);
//
//  return bfill.position();
//}
