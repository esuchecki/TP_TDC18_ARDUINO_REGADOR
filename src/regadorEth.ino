#include <EtherCard.h>

static byte mymac[] = { 0xDD, 0xDD, 0xDD, 0x00, 0x01, 0x05 };
static byte myip[] = { 192, 168, 1, 177 };
byte Ethernet::buffer[700];

const int pinLed1 = 2;
const int pinLed2 = 3;
char* statusLed1 = "OFF";
char* statusLed2 = "OFF";



void setup() {

  Serial.begin(9600);

  if (!ether.begin(sizeof Ethernet::buffer, mymac, 10)) {
    Serial.println("No se ha podido acceder a la controlador Ethernet");
  }
  else {
    Serial.println("Controlador Ethernet inicializado");
  }

  if (!ether.staticSetup(myip)) {
    Serial.println("No se pudo establecer la dirección IP");
  }
  
  Serial.println();

  pinMode(pinLed1, OUTPUT);
  pinMode(pinLed2, OUTPUT);
  digitalWrite(pinLed1, LOW);
  digitalWrite(pinLed2, LOW);
}



static word showWebInputStatus()
{
  BufferFiller bfill = ether.tcpOffset();

  bfill.emit_p(PSTR("<!DOCTYPE html>"
                    "<html><head><title>INPUT STATUS</title></head>"
                    "<body>"
                    "<div style='text-align:center;'>"
                    "<h1>Entradas digitales</h1>"
                    "Tiempo transcurrido : $L s"
                    "<br /><br />D00: $D"
                    "<br /><br />D01: $D"
                    "<br /><br />D02: $D"
                    "<br /><br />D03: $D"
                    "<br /><br />D04: $D"
                    "<br /><br />D05: $D"
                    "<br /><br />D06: $D"
                    "<br /><br />D07: $D"
                    "<br /><br />D08: $D"
                    "<br /><br />D09: $D"
                    "<br /><br />D10: $D"
                    "<br /><br />D11: $D"
                    "<br /><br />D12: $D"
                    "<br /><br />D13: $D"

                    "<h1>Entradas analogicas</h1>"
                    "<br /><br />AN0: $D"
                    "<br /><br />AN1: $D"
                    "<br /><br />AN2: $D"
                    "<br /><br />AN3: $D"
                    "<br /><br />AN4: $D"
                    "<br /><br />AN5: $D"
                    "<br /><br />AN6: $D"
                    "<br /><br />"
                    "</body></html>"),
               millis() / 1000,
               digitalRead(0),
               digitalRead(1),
               digitalRead(2),
               digitalRead(3),
               digitalRead(4),
               digitalRead(5),
               digitalRead(6),
               digitalRead(7),
               digitalRead(8),
               digitalRead(9),
               digitalRead(10),
               digitalRead(11),
               digitalRead(12),
               digitalRead(13),
               analogRead(0),
               analogRead(1),
               analogRead(2),
               analogRead(3),
               analogRead(4),
               analogRead(5),
               analogRead(6));

  return bfill.position();
}



static word showWebOutputs()
{
  BufferFiller bfill = ether.tcpOffset();
  bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\n"
                    "Content-Type: text/html\r\nPragma: no-cachernRefresh: 5\r\n\r\n"
                    "<html><head><title>OUTPUT STATUS</title></head>"
                    "<body>"
                    "<div style='text-align:center;'>"
                    "<h1>Salidas digitales</h1>"
                    "<br /><br />Estado LED 1 = $S<br />"
                    "<a href='./?data1=0'><input type='button' value='OFF'></a>"
                    "<a href='./?data1=1'><input type='button' value='ON'></a>"
                    "<br /><br />Estado LED 2 = $S<br />"
                    "<a href='./?data2=0'><input type='button' value='OFF'></a>"
                    "<a href='./?data2=1'><input type='button' value='ON'></a>"
                    "<br /></div>\n</body></html>"), statusLed1, statusLed2);

  return bfill.position();
}



void loop()
{
  //loopInput();
  loopOutput();
}



void loopInput()
{
  // wait for an incoming TCP packet, but ignore its contents
  if (ether.packetLoop(ether.packetReceive()))
  {
    ether.httpServerReply(showWebInputStatus());
  }
}



void loopOutput()
{
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (pos)
  {
    if (strstr((char *)Ethernet::buffer + pos, "GET /?data1=0") != 0) {
      Serial.println("Led1 OFF");
      digitalWrite(pinLed1, LOW);
      statusLed1 = "OFF";
    }

    if (strstr((char *)Ethernet::buffer + pos, "GET /?data1=1") != 0) {
      Serial.println("Led1 ON");
      digitalWrite(pinLed1, HIGH);
      statusLed1 = "ON";
    }

    if (strstr((char *)Ethernet::buffer + pos, "GET /?data2=0") != 0) {
      Serial.println("Led2 OFF recieved");
      digitalWrite(pinLed2, LOW);
      statusLed2 = "OFF";
    }

    if (strstr((char *)Ethernet::buffer + pos, "GET /?data2=1") != 0) {
      Serial.println("Led2 ON");
      digitalWrite(pinLed2, HIGH);
      statusLed2 = "ON";
    }


    ether.httpServerReply(showWebOutputs());
  }
}
