// Gerekli Kütüphaneleri Çağırıyoruz
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


// Ağ bilgilerinizi girin
const char* ssid = "avcilar-hk3";
const char* password = "avcilar-hk31234";

//2 numaralı GPIO pini LED için tanımladık.
bool ledDurum = LOW;
const int ledPin = D2;
const int ledWifi = D4;
boolean butonDurum = 0;

// LED'i yakıp söndürmek için gerekli zamanlama değişkenleri
unsigned long simdikizaman = 0;
unsigned long oncekizaman = 0;

// 80 portunda çalışacak bir sunucu ve bu sunucu üzerinde çalışacak "ws" adında bir AsyncWebServer nesnesi yaratalım
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//web sayfamız ile ilgili HTML kodları:
//aslında bunu doğrudan bir dosya olarak SPIFFS kullanarak da alabilirdik.
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Avcılar Bahçe Kapısı</title>
  <meta charset="utf-8" name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
  }
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
  </style>
<title>ESP Web Socket Sunucusu</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>Avcılar Bahçe Kapısı Otomatiği</h1>
  </div>
  <div class="content">
    <div class="card">
      <!--p class="state">Durum: <span id="state">%STATE%</span></p-->
      <p><button id="button" onclick="myFunction()" class="button">Tıkla</button></p>
    </div>
  </div>
<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function WebSocketBaslat() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    //websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(WebSocketBaslat, 2000);
  }
  //function onMessage(event) {
  //  var state;
  //  if (event.data == "1"){
  //    state = "ON";
  //  }
  //  else{
  //    state = "OFF";
  //  }
  // document.getElementById('state').innerHTML = state;
  //}
  function onLoad(event) {
    WebSocketBaslat();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
  }
  function toggle(){
    websocket.send('toggle');
  }
  var buton = document.getElementById('button');

  function myFunction() {
      buton.innerHTML = 'Tıklandı';
      buton.disabled = true;

      setTimeout(function () {
          buton.innerHTML = 'Tıkla';
          buton.disabled = false;
      }, 15000);
    }
</script>
</script>
</body>
</html>
)rawliteral";

//güncel LedDurum bilgisini tüm istemcilere gönderen fonksiyon
void istemciBilgilendir() {
  ws.textAll(String(ledDurum));
}

void WebSocketMesajiniYonet(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      ledDurum = !ledDurum;
      istemciBilgilendir();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket istemci #%u bağlandı: %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket istemci #%u ayrıldı\n", client->id());
      break;
    case WS_EVT_DATA:
      WebSocketMesajiniYonet(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void WebSocketBaslat() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledDurum){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
}


// Butona basılıp basılmadığını takip eden KESME fonksiyonumuz
void IRAM_ATTR ledKontrol() {
    butonDurum = 1;                                                     //butonDurum bayrağını 1'e çekerek ana koda bilgi gönderiyoruz.
}

void setup(){
  
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(ledWifi, OUTPUT);
  digitalWrite(ledWifi, HIGH);
  attachInterrupt(digitalPinToInterrupt(ledDurum), ledKontrol, RISING); //HIGH oldukça çalışacak //nodemcu için çalışmaz. nodemcu için RISING yapın
  
  // wifi bağlantısı
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Ağ bağlantısı sağlanıyor..");
  }

  // ESP'nin IP'sini yazdıralım
  Serial.println("Ağ bağlantısı kuruldu:");
  Serial.println(WiFi.localIP());
  digitalWrite(ledWifi, LOW);
  WebSocketBaslat();

  // web sayfasının kök dizini anlaması için:
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // sunucuyu başlatalım
  server.begin();
}

void loop() {
  ws.cleanupClients();                                                //istemcileri yeniliyor
  //digitalWrite(ledPin, ledDurum);
  // şu anki zamanı sürekli olarak simdikizaman değişkenine atayalım
  simdikizaman = millis();
  if (butonDurum == 1) {
    Serial.println("BUTONA BASILDI");
    digitalWrite(ledPin, ledDurum);
    istemciBilgilendir();
    butonDurum = 0;                                                   //güncelleyelim ki düzgün çalışsın
  }
  else{
  if ((simdikizaman - oncekizaman) >= 1000) {
    oncekizaman = simdikizaman;                                       //oncekizamanı da güncelleyelim ki düzgün çalışsın
    if (ledDurum == HIGH) {
      ledDurum = LOW;
      istemciBilgilendir();
    }
    //istemciBilgilendir();
  }
  digitalWrite(ledPin, ledDurum);
  }
}
