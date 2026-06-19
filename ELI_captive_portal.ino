/*
 * ELI — Cadou pentru Eliza 💕
 * cu Captive Portal (pagina apare automat)
 * ════════════════════════════════════════
 * Board   : ESP32-S2 (LOLIN S2 Mini)
 * Display : SSD1306 128×64 I2C
 *
 * Libraries (Arduino Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Wiring OLED:
 *   VCC → 3.3V | GND → GND
 *   SDA → 33   | SCL → 35
 *
 * ════════════════════════════════════════
 * CUM FUNCTIONEAZA:
 *   1. ELI porneste si creeaza WiFi propriu
 *   2. Eliza se conecteaza la "ELI 💕"
 *      parola: iloveeli
 *   3. Pagina apare AUTOMAT pe iPhone
 *   4. Alege animatia sau scrie un mesaj
 * ════════════════════════════════════════
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "loveMeNot_frames.h"  // animatie Love Me Not
#include "noOne_frames.h"      // animatie No One Noticed

// ══════════════════════════════
//  WiFi Access Point
// ══════════════════════════════
const char* AP_SSID = "ELI";
const char* AP_PASS = "iloveeli";   // minim 8 caractere
const byte  DNS_PORT = 53;
IPAddress   apIP(192, 168, 4, 1);

DNSServer          dns;
WebServer   server(80);

// ══════════════════════════════
//  OLED
// ══════════════════════════════
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ══════════════════════════════
//  STATE
// ══════════════════════════════
int  currentMode = 0;
int  animFrame   = 0;
unsigned long lastTick     = 0;
unsigned long lastSurprise = 0;
unsigned long startTime    = 0;
int  scrollX     = 128;
String customMsg = "Te iubesc!";

// Ora/data setata manual din telefon
long  timeOffsetSec = 0;
bool  timeSet       = false;
int   setDay = 1, setMonth = 1, setYear = 2025;

long currentTimeSec() {
  unsigned long elapsed = (millis() - startTime) / 1000;
  return (timeOffsetSec + (long)elapsed) % 86400L;
}

// ══════════════════════════════
//  PIXEL ART HELPERS
// ══════════════════════════════
void smallHeart(int x, int y) {
  display.drawPixel(x+1,y,   WHITE); display.drawPixel(x+3,y,   WHITE);
  display.drawPixel(x,  y+1, WHITE); display.drawPixel(x+1,y+1, WHITE);
  display.drawPixel(x+2,y+1, WHITE); display.drawPixel(x+3,y+1, WHITE);
  display.drawPixel(x+4,y+1, WHITE);
  display.drawPixel(x,  y+2, WHITE); display.drawPixel(x+1,y+2, WHITE);
  display.drawPixel(x+2,y+2, WHITE); display.drawPixel(x+3,y+2, WHITE);
  display.drawPixel(x+4,y+2, WHITE);
  display.drawPixel(x+1,y+3, WHITE); display.drawPixel(x+2,y+3, WHITE);
  display.drawPixel(x+3,y+3, WHITE);
  display.drawPixel(x+2,y+4, WHITE);
}

void drawStar(int cx, int cy, int r) {
  for(int i=0;i<5;i++){
    float a1 = (i*72 - 90) * PI / 180.0;
    float a2 = ((i*72+36) - 90) * PI / 180.0;
    float a3 = ((i*72+72) - 90) * PI / 180.0;
    int x1=cx+r*cos(a1),     y1=cy+r*sin(a1);
    int x2=cx+(r/2)*cos(a2), y2=cy+(r/2)*sin(a2);
    int x3=cx+r*cos(a3),     y3=cy+r*sin(a3);
    display.drawLine(x1,y1,x2,y2,WHITE);
    display.drawLine(x2,y2,x3,y3,WHITE);
  }
}

// ══════════════════════════════
//  MODURI DISPLAY
// ══════════════════════════════

// 0 — CEAS (bazat pe millis, fara WiFi extern)
void modeClock() {
  display.clearDisplay();

  // Ora curenta
  long ts = currentTimeSec();
  int h = ts / 3600;
  int m = (ts % 3600) / 60;
  int s = ts % 60;

  // Inimi colturi
  smallHeart(1, 1);   smallHeart(119, 1);
  smallHeart(1, 55);  smallHeart(119, 55);

  // "Eliza" sus — centrat
  display.setTextColor(WHITE);
  display.setTextSize(1);
  // "Eliza" = 5 litere * 6px = 30px + inima 7px + spatiu 3px = 40px total
  display.setCursor(44, 3);
  display.print("Eliza");
  smallHeart(93, 2);

  // Linie sus
  display.drawLine(8, 13, 120, 13, WHITE);

  // ORA mare — centrata (8 caractere * 12px = 96px, (128-96)/2 = 16)
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  display.setTextSize(2);
  display.setCursor(16, 18);
  display.print(buf);

  // Linie jos
  display.drawLine(8, 38, 120, 38, WHITE);

  // "te iubesc" centrat (9 litere * 6px = 54px, (128-54)/2 = 37) + inima
  display.setTextSize(1);
  if(s % 2 == 0) {
    display.setCursor(28, 44);
    display.print("te iubesc");
    smallHeart(90, 43);
  } else {
    display.setCursor(30, 44);
    display.print("te iubesc");
    smallHeart(92, 43);
  }

  // Data jos — centrata (10 chars * 6px = 60px, (128-60)/2 = 34)
  char dateBuf[12];
  sprintf(dateBuf, "%02d/%02d/%04d", setDay, setMonth, setYear);
  display.setCursor(34, 55);
  display.print(dateBuf);

  display.display();
}

// 1 — INIMI ZBURATOARE
struct FHeart { float x,y,vy,vx; bool on; };
FHeart fh[10];

void initFlyHearts() {
  for(int i=0;i<10;i++){
    fh[i].x  = 5 + random(118);
    fh[i].y  = 64 + random(20);
    fh[i].vy = -(0.6f + random(80)/100.0f);
    fh[i].vx = (random(60)/100.0f) - 0.3f;
    fh[i].on = true;
  }
}

void modeFlyHearts() {
  display.clearDisplay();
  for(int i=0;i<10;i++){
    if(!fh[i].on) continue;
    fh[i].y += fh[i].vy * 1.8f;
    fh[i].x += fh[i].vx;
    if(fh[i].y < -6) fh[i].on = false;
    if(fh[i].on) smallHeart((int)fh[i].x, (int)fh[i].y);
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  if(animFrame < 22){
    display.setCursor(22, 22); display.print("de la Davide");
    display.setCursor(14, 36); display.print("cu multa dragoste");
  } else {
    // inima mare simpla
    for(int y=-10;y<=10;y++)
      for(int x=-12;x<=12;x++){
        float nx=x/10.0f, ny=y/10.0f;
        float v=nx*nx+(ny-0.8f*sqrtf(fabsf(nx)))*(ny-0.8f*sqrtf(fabsf(nx)))-1.0f;
        if(v<=0) display.drawPixel(64+x, 35-y, WHITE);
      }
  }
  display.display();
  animFrame++;
  if(animFrame > 32) { animFrame=0; currentMode=0; }
}

// 4 — MESAJ CUSTOM
void modeCustomMsg() {
  display.clearDisplay();
  smallHeart(2,2); smallHeart(119,2);
  smallHeart(2,56); smallHeart(119,56);

  display.setTextSize(1);
  display.setTextColor(WHITE);

  int len = customMsg.length();
  int lineW = 21;
  int lines = (len + lineW - 1) / lineW;
  int startY = max(8, 32 - (lines * 10) / 2);
  for(int i=0;i<lines;i++){
    String ln = customMsg.substring(i*lineW, min((i+1)*lineW, len));
    int lx = max(0, (128 - (int)ln.length()*6) / 2);
    display.setCursor(lx, startY + i*11);
    display.print(ln);
  }
  display.display();
}

// ══════════════════════════════
//  PAGINA WEB
// ══════════════════════════════
const char HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ro">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>ELI 💕</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Nunito:wght@400;700;900&family=Dancing+Script:wght@700&display=swap');
*{margin:0;padding:0;box-sizing:border-box}
body{background:#fff0f5;font-family:'Nunito',sans-serif;color:#3d2c4e;min-height:100vh;padding-bottom:50px}
.hdr{background:linear-gradient(135deg,#ff6b9d,#c084fc);padding:26px 16px 20px;text-align:center;position:relative;overflow:hidden}
.hdr::before{content:'';position:absolute;inset:-50%;background:radial-gradient(circle,rgba(255,255,255,.12) 0%,transparent 60%);animation:p 3s ease-in-out infinite}
@keyframes p{0%,100%{transform:scale(1)}50%{transform:scale(1.1)}}
.from{font-size:11px;color:rgba(255,255,255,.75);letter-spacing:1px;margin-bottom:3px}
.hdr h1{font-family:'Dancing Script',cursive;font-size:36px;color:#fff;text-shadow:0 2px 12px rgba(0,0,0,.2);position:relative}
.hdr p{font-size:12px;color:rgba(255,255,255,.8);margin-top:4px;position:relative}
.status{display:flex;align-items:center;justify-content:center;gap:8px;background:#fff;padding:9px;border-bottom:1px solid #fce7f3;font-size:12px;font-weight:700;color:#be185d}
.dot{width:8px;height:8px;border-radius:50%;background:#22c55e;animation:b 1.5s infinite}
@keyframes b{0%,100%{opacity:1}50%{opacity:.3}}
.note{margin:14px 14px 0;background:#fff;border-radius:14px;padding:14px 16px;border-left:4px solid #ff6b9d;box-shadow:0 2px 10px rgba(255,107,157,.1)}
.note p{font-size:13px;color:#be185d;font-style:italic;line-height:1.5}
.note span{font-family:'Dancing Script',cursive;font-size:17px;color:#c084fc;display:block;text-align:right;margin-top:4px}
.sec{padding:16px 14px 0}
.stitle{font-size:11px;font-weight:900;color:#9ca3af;letter-spacing:2px;text-transform:uppercase;margin-bottom:10px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.card{background:#fff;border-radius:14px;padding:14px;text-align:center;cursor:pointer;border:2px solid transparent;box-shadow:0 2px 10px rgba(0,0,0,.05);transition:.2s;position:relative;-webkit-tap-highlight-color:transparent}
.card:active{transform:scale(.96)}
.card.on{border-color:#ff6b9d;background:linear-gradient(135deg,#fff0f8,#f5f0ff)}
.card.on::after{content:'✓';position:absolute;top:7px;right:9px;background:#ff6b9d;color:#fff;width:18px;height:18px;border-radius:50%;font-size:10px;font-weight:900;line-height:18px;text-align:center}
.em{font-size:30px;margin-bottom:6px;display:block}
.cn{font-size:13px;font-weight:700}
.cd{font-size:10px;color:#9ca3af;margin-top:2px}
.mbox{background:#fff;border-radius:14px;padding:14px;box-shadow:0 2px 10px rgba(0,0,0,.05)}
textarea{width:100%;border:1.5px solid #fce7f3;border-radius:10px;padding:10px;font-family:'Nunito',sans-serif;font-size:14px;color:#3d2c4e;resize:none;outline:none;height:72px}
textarea:focus{border-color:#ff6b9d}
.btn{display:block;width:calc(100% - 28px);margin:16px auto 0;background:linear-gradient(135deg,#ff6b9d,#c084fc);color:#fff;font-family:'Nunito',sans-serif;font-size:15px;font-weight:900;padding:15px;border:none;border-radius:14px;cursor:pointer;box-shadow:0 4px 18px rgba(255,107,157,.35);transition:.2s;-webkit-tap-highlight-color:transparent}
.btn:active{transform:scale(.97)}
.btn:disabled{background:#e5e7eb;color:#9ca3af;box-shadow:none}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%) translateY(70px);background:#1a1a2e;color:#fff;font-size:13px;font-weight:700;padding:11px 22px;border-radius:100px;transition:.3s;z-index:99;white-space:nowrap;pointer-events:none}
.toast.show{transform:translateX(-50%) translateY(0)}
</style>
</head>
<body>
<div class="hdr">
  <div class="from">de la Davide, cu drag 🤍</div>
  <h1>Bună, Eliza! 💕</h1>
  <p>tu decizi ce apare pe ELI</p>
</div>
<div class="status"><div class="dot"></div><span>ELI e conectat</span></div>
<div class="note">
  <p>"L-am construit special pentru tine. E al tău."</p>
  <span>— Davide</span>
</div>
<div class="sec">
  <div class="stitle">Alege animația</div>
  <div class="grid">
    <div class="card on" onclick="pick(0,this)"><span class="em">🕐</span><div class="cn">Ceas</div><div class="cd">ora + inimi</div></div>
    <div class="card" onclick="pick(1,this)"><span class="em">💖</span><div class="cn">Inimi</div><div class="cd">zboară spre tine</div></div>
    <div class="card" onclick="pick(10,this)"><span class="em">💔</span><div class="cn">Love Me Not</div><div class="cd">animație muzicală</div></div>
    <div class="card" onclick="pick(11,this)"><span class="em">🎵</span><div class="cn">No One Noticed</div><div class="cd">animație muzicală</div></div>
                              </div>
</div>
<div class="sec" style="margin-top:14px">
  <div class="stitle">Mesaj personal</div>
  <div class="mbox">
    <textarea id="mi" placeholder="Scrie ceva drăguț... 💕" maxlength="80"></textarea>
  </div>
</div>
<div class="sec" style="margin-top:14px">
  <div class="stitle">Setează ora & data</div>
  <div class="mbox" style="display:flex;flex-direction:column;gap:10px">
    <div style="display:flex;gap:8px;align-items:center">
      <input type="time" id="ti" style="flex:1;border:1.5px solid #fce7f3;border-radius:10px;padding:9px;font-family:'Nunito',sans-serif;font-size:15px;color:#3d2c4e;outline:none;background:#fff">
      <input type="date" id="di" style="flex:1;border:1.5px solid #fce7f3;border-radius:10px;padding:9px;font-family:'Nunito',sans-serif;font-size:13px;color:#3d2c4e;outline:none;background:#fff">
    </div>
    <button class="btn" style="margin:0;width:100%" onclick="setTime()">🕐 Setează ora</button>
  </div>
</div>
<button class="btn" id="sb" onclick="go()">💌 Trimite la ELI</button>
<div class="toast" id="t"></div>
<script>
let sel=0;
function pick(n,el){sel=n;document.querySelectorAll('.card').forEach(c=>c.classList.remove('on'));el.classList.add('on');}
function toast(m){const t=document.getElementById('t');t.textContent=m;t.classList.add('show');setTimeout(()=>t.classList.remove('show'),2600);}
// Trimite ora automat din telefon la incarcare
window.addEventListener('load',async function(){
  const now=new Date();
  const pad=n=>String(n).padStart(2,'0');
  const hh=now.getHours(),mm=now.getMinutes(),ss=now.getSeconds();
  const d=now.getDate(),mo=now.getMonth()+1,y=now.getFullYear();
  try{
    await fetch('/set?hour='+hh+'&min='+mm+'&sec='+ss+'&day='+d+'&month='+mo+'&year='+y);
    document.getElementById('ti').value=pad(hh)+':'+pad(mm);
    document.getElementById('di').value=y+'-'+pad(mo)+'-'+pad(d);
  }catch(e){}
});
async function setTime(){
  const ti=document.getElementById('ti').value;
  const di=document.getElementById('di').value;
  if(!ti){toast('⏰ Alege ora mai întâi!');return;}
  const [hh,mm]=ti.split(':');
  let params='hour='+hh+'&min='+mm+'&sec=0';
  if(di){const[y,mo,d]=di.split('-');params+='&day='+d+'&month='+mo+'&year='+y;}
  try{
    const r=await fetch('/set?'+params);
    if(r.ok)toast('⏰ Ora setată pe ELI!');
    else toast('❌ Eroare');
  }catch(e){toast('❌ ELI nu răspunde');}
}
async function go(){
  const b=document.getElementById('sb');
  b.disabled=true;b.textContent='⏳ ELI primește...';
  try{
    const msg=encodeURIComponent(document.getElementById('mi').value||'');
    const r=await fetch('/set?mode='+sel+'&msg='+msg);
    if(r.ok)toast('💕 ELI afișează acum!');
    else toast('❌ Încearcă din nou');
  }catch(e){toast('❌ ELI nu răspunde');}
  b.disabled=false;b.textContent='💌 Trimite la ELI';
}
</script>
</body>
</html>
)rawliteral";

// 10 — LOVE ME NOT animatie
#define LMN_FRAMES 102
#define LMN_DELAY  64
int lmnFrame = 0;
unsigned long lmnLastFrame = 0;

void modeLoveMeNot() {
  if(millis() - lmnLastFrame >= LMN_DELAY) {
    lmnLastFrame = millis();
    display.clearDisplay();
    display.drawBitmap(0, 0, epd_bitmap_allArray[lmnFrame], 128, 64, WHITE);
    display.display();
    lmnFrame++;
    if(lmnFrame >= LMN_FRAMES) {
      lmnFrame = 0;
      currentMode = 0;  // revine la ceas dupa animatie
    }
  }
}

// 11 — NO ONE NOTICED animatie
#define NON_FRAMES 199
#define NON_DELAY  64
int nonFrame = 0;
unsigned long nonLastFrame = 0;

void modeNoOne() {
  if(millis() - nonLastFrame >= NON_DELAY) {
    nonLastFrame = millis();
    display.clearDisplay();
    display.drawBitmap(0, 0, noOne_allArray[nonFrame], 128, 64, WHITE);
    display.display();
    nonFrame++;
    if(nonFrame >= NON_FRAMES) {
      nonFrame = 0;
      currentMode = 0;
    }
  }
}

// ══════════════════════════════
//  HTTP HANDLERS
// ══════════════════════════════
void handleRoot() {
  server.send(200, "text/html", String(HTML));
}

// Captive portal — toate cererile necunoscute redirectate la pagina noastra
void handleCaptive() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

void handleSet() {
  // Seteaza ora/data
  if(server.hasArg("hour") && server.hasArg("min")) {
    int hh = server.arg("hour").toInt();
    int mm = server.arg("min").toInt();
    int ss = server.hasArg("sec") ? server.arg("sec").toInt() : 0;
    timeOffsetSec = (long)hh*3600 + mm*60 + ss;
    startTime = millis();
    timeSet = true;
    if(server.hasArg("day"))   setDay   = server.arg("day").toInt();
    if(server.hasArg("month")) setMonth = server.arg("month").toInt();
    if(server.hasArg("year"))  setYear  = server.arg("year").toInt();
    server.send(200, "text/plain", "OK");
    return;
  }
  if(server.hasArg("msg") && server.arg("msg").length() > 0) {
    customMsg = server.arg("msg");
    currentMode = 4;
    animFrame = 0;
  } else if(server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    currentMode = m;
    animFrame = 0;
    scrollX = 128;
    if(m == 1) initFlyHearts();
    if(m == 10) { lmnFrame=0; lmnLastFrame=0; }
    if(m == 11) { nonFrame=0; nonLastFrame=0; }
  }
  server.send(200, "text/plain", "OK");
}

// ══════════════════════════════
//  SETUP
// ══════════════════════════════
void setup() {
  Serial.begin(115200);
  Wire.begin(33, 35);  // SDA=33, SCL=35 pentru LOLIN S2 Mini

  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED nu gasit!");
    while(true);
  }
  display.setRotation(2);  // 0=normal, 2=intors 180°

  // Splash
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(20, 16);
  display.print("ELI porneste...");
  display.setCursor(14, 30);
  display.print("pentru Eliza");
  smallHeart(60, 46);
  display.display();
  delay(1500);

  // Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  WiFi.softAP(AP_SSID, AP_PASS);

  // DNS — redirecteaza TOT traficul la ESP
  // Asta e ce face iPhone-ul sa deschida pagina automat
  dns.start(DNS_PORT, "*", apIP);

  // Server routes
  server.on("/", handleRoot);
  server.on("/set", handleSet);

  // Captive portal endpoints pentru iPhone/Android
  server.on("/generate_204",        handleCaptive); // Android
  server.on("/gen_204",             handleCaptive);
  server.on("/hotspot-detect.html", handleCaptive); // iPhone/macOS
  server.on("/library/test/success.html", handleCaptive);
  server.on("/success.txt",         handleCaptive);
  server.on("/ncsi.txt",            handleCaptive); // Windows
  server.on("/connecttest.txt",     handleCaptive);
  server.onNotFound(handleCaptive);

  server.begin();

  // Gata — arata pe ecran
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 8);
  display.print("Conecteaza-te la:");
  display.setTextSize(2);
  display.setCursor(28, 22);
  display.print("ELI");
  display.setTextSize(1);
  display.setCursor(16, 42);
  display.print("Parola: iloveeli");
  smallHeart(52, 56);
  display.display();
  delay(3000);

  startTime = millis();
  randomSeed(esp_random());
  lastSurprise = millis();
}

// ══════════════════════════════
//  LOOP
// ══════════════════════════════
void loop() {
  dns.processNextRequest();      // ← IMPORTANT pentru captive portal
  server.handleClient();

  unsigned long now = millis();

  // Surpriza automata la 45 secunde
  if(currentMode == 0 && now - lastSurprise >= 45000) {
    lastSurprise = now;
    int r = random(3);
    if(r==0)      { currentMode=1; initFlyHearts(); }
    else if(r==1) { currentMode=2; scrollX=128; }
    else          { currentMode=3; }
    animFrame = 0;
  }

  switch(currentMode) {
    case 0:
      if(now - lastTick >= 1000) { lastTick=now; modeClock(); }
      break;
    case 1:
      if(now - lastTick >= 70)   { lastTick=now; modeFlyHearts(); }
      break;


    case 4:
      if(now - lastTick >= 100)  { lastTick=now; modeCustomMsg(); }
      break;
    case 10:
      modeLoveMeNot();
      break;
    case 11:
      modeNoOne();
      break;





  }
}
