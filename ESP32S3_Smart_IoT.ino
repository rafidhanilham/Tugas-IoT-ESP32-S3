#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>

// ==========================================
// PIN ESP32-S3
// ==========================================
#define TRIG_PIN 5
#define ECHO_PIN 6
#define LED_PIN 2
#define BOOT_PIN 0

// ==========================================
// ACCESS POINT
// ==========================================
const char* apSSID = "ESP32-S3";
const char* apPassword = "12345678";

const byte DNS_PORT = 53;

// ==========================================
// OBJEK
// ==========================================
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

// ==========================================
// VARIABEL WIFI
// ==========================================
String wifiSSID = "";
String wifiPASS = "";

bool apMode = false;

// ==========================================
// VARIABEL SENSOR
// ==========================================
float distance = -1;
unsigned long previousMillis = 0;
const unsigned long interval = 500;

// ==========================================
// VARIABEL BOOT
// ==========================================
unsigned long bootPressStart = 0;
bool bootTiming = false;
bool bootResetDone = false;


// ==========================================
// BACA SENSOR
// ==========================================
float readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return -1;

  float cm = duration * 0.0343 / 2.0;

  if (cm < 2 || cm > 400) return -1;

  return cm;
}


// ==========================================
// FILTER MEDIAN
// ==========================================
float medianDistance() {
  float values[5];
  int count = 0;

  for (int i = 0; i < 5; i++) {
    float result = readDistance();

    if (result > 0) {
      values[count++] = result;
    }

    delay(10);
  }

  if (count == 0) return -1;

  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (values[i] > values[j]) {
        float temp = values[i];
        values[i] = values[j];
        values[j] = temp;
      }
    }
  }

  return values[count / 2];
}


// ==========================================
// STATUS SENSOR
// ==========================================
String getStatus(float cm) {
  if (cm < 0) return "Tidak Terdeteksi";
  if (cm < 10) return "BAHAYA";
  if (cm <= 30) return "WASPADA";
  return "AMAN";
}

String getColor(float cm) {
  if (cm < 0) return "gray";
  if (cm < 10) return "red";
  if (cm <= 30) return "orange";
  return "green";
}


// ==========================================
// HTML ESCAPE
// ==========================================
String htmlEscape(String input) {
  input.replace("&", "&amp;");
  input.replace("<", "&lt;");
  input.replace(">", "&gt;");
  input.replace("\"", "&quot;");
  input.replace("'", "&#39;");
  return input;
}


// ==========================================
// PEMINDAIAN WIFI
// ==========================================
String wifiListHTML() {
  String html = "";

  int networks = WiFi.scanNetworks();

  if (networks <= 0) {
    html = "<option value=''>Tidak ada WiFi ditemukan</option>";
  } else {
    for (int i = 0; i < networks; i++) {
      String ssid = WiFi.SSID(i);

      if (ssid.length() == 0) continue;

      html += "<option value=\"";
      html += htmlEscape(ssid);
      html += "\">";

      html += htmlEscape(ssid);
      html += " | ";
      html += String(WiFi.RSSI(i));
      html += " dBm</option>";
    }
  }

  WiFi.scanDelete();

  return html;
}


// ==========================================
// HALAMAN CAPTIVE PORTAL FUTURISTIC
// ==========================================
void handleRoot() {

  String networks = wifiListHTML();

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>ESP32-S3 Smart IoT</title>

<style>

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: Arial, sans-serif;
  color: white;
  background:
    radial-gradient(ellipse at 80% 10%, #063c36 0%, transparent 40%),
    radial-gradient(ellipse at 10% 90%, #063b21 0%, transparent 35%),
    #020909;
  min-height: 100vh;
  overflow-x: hidden;
}

body::before {
  content: "";
  position: fixed;
  inset: 0;
  pointer-events: none;
  opacity: .2;
  background-image:
    linear-gradient(rgba(0,255,150,.08) 1px, transparent 1px),
    linear-gradient(90deg, rgba(0,255,150,.08) 1px, transparent 1px);
  background-size: 35px 35px;
}

.container {
  width: 100%;
  max-width: 1100px;
  margin: auto;
  padding: 22px;
  position: relative;
}

.header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 15px;
  border-bottom: 1px solid rgba(0,255,150,.35);
  padding-bottom: 20px;
}

.brand {
  display: flex;
  align-items: center;
  gap: 13px;
}

.logo {
  font-size: 38px;
  color: #39ff75;
  text-shadow: 0 0 20px #00ff77;
}

.brand h2 {
  margin: 0;
  font-size: 22px;
  letter-spacing: 1px;
}

.brand h2 span {
  color: #39ff75;
}

.brand p {
  margin: 5px 0 0;
  color: #a5c5bd;
  font-size: 12px;
}

.status {
  border: 1px solid #23ed83;
  border-radius: 30px;
  padding: 12px 17px;
  color: #55ff8c;
  font-size: 12px;
  background: rgba(0,255,130,.07);
  box-shadow: 0 0 18px rgba(0,255,130,.12);
}

.hero {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 25px;
  align-items: center;
  padding: 40px 5px 30px;
}

.hero h1 {
  font-size: clamp(35px, 6vw, 65px);
  line-height: 1;
  margin: 0;
  font-weight: 900;
}

.hero h1 span {
  color: #38ff71;
  text-shadow: 0 0 20px rgba(0,255,100,.4);
}

.hero h3 {
  color: #b5c9c4;
  font-size: 20px;
  margin: 13px 0;
}

.hero p {
  color: #91aaa2;
  line-height: 1.7;
}

.chip {
  text-align: center;
  font-size: 110px;
  color: #24eaff;
  text-shadow: 0 0 25px #00cfff;
  filter: drop-shadow(0 0 20px #00aaff);
  animation: float 3s ease-in-out infinite;
}

@keyframes float {
  0%,100% { transform: translateY(0); }
  50% { transform: translateY(-12px); }
}

.panel {
  max-width: 820px;
  margin: 0 auto;
  padding: 30px;
  border: 1px solid rgba(49,255,176,.7);
  border-radius: 25px;
  background: linear-gradient(
    135deg,
    rgba(7,45,39,.94),
    rgba(5,17,25,.96)
  );
  box-shadow:
    0 0 30px rgba(0,255,140,.13),
    inset 0 0 25px rgba(0,255,160,.05);
}

.panel-title {
  display: flex;
  align-items: center;
  gap: 15px;
  margin-bottom: 23px;
}

.panel-title .symbol {
  font-size: 38px;
  color: #3cff82;
}

.panel-title h2 {
  margin: 0;
  font-size: 22px;
}

.panel-title p {
  color: #9cbab0;
  margin: 6px 0 0;
  font-size: 13px;
}

label {
  display: block;
  margin: 18px 0 9px;
  color: #d5e8e0;
  font-size: 14px;
  font-weight: bold;
}

select, input {
  width: 100%;
  padding: 15px;
  color: white;
  background: rgba(1,12,18,.9);
  border: 1px solid #385b5d;
  border-radius: 12px;
  outline: none;
  font-size: 14px;
}

select:focus, input:focus {
  border-color: #32ff91;
  box-shadow: 0 0 15px rgba(0,255,140,.2);
}

select option {
  background: #071a1c;
  color: white;
}

.password-box {
  display: flex;
  gap: 8px;
}

.password-box input {
  flex: 1;
  min-width: 0;
}

.show-btn {
  background: #103d35;
  border: 1px solid #26d98b;
  color: #65ffa9;
  padding: 0 15px;
  border-radius: 12px;
  cursor: pointer;
}

.main-btn {
  width: 100%;
  margin-top: 25px;
  padding: 17px;
  border: none;
  border-radius: 13px;
  color: #03130b;
  background: linear-gradient(90deg,#41ff65,#00e9df);
  font-size: 16px;
  font-weight: 900;
  cursor: pointer;
  box-shadow: 0 0 25px rgba(0,255,130,.3);
  transition: .2s;
}

.main-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 0 35px rgba(0,255,130,.55);
}

.info {
  display: grid;
  grid-template-columns: repeat(3,1fr);
  gap: 12px;
  margin-top: 24px;
}

.info-card {
  text-align: center;
  padding: 17px 8px;
  border: 1px solid rgba(0,255,150,.2);
  background: rgba(0,255,150,.04);
  border-radius: 14px;
}

.info-card strong {
  display: block;
  color: #47ff88;
  margin-bottom: 7px;
  font-size: 13px;
}

.info-card small {
  color: #a2bcb5;
  font-size: 11px;
}

.footer {
  text-align: center;
  color: #64877b;
  margin: 28px 0 10px;
  font-size: 12px;
  letter-spacing: 3px;
}

.footer span {
  color: #43ff80;
}

.loading {
  display: none;
  text-align: center;
  color: #4aff94;
  margin-top: 15px;
  font-size: 13px;
}

@media(max-width:650px) {
  .container {
    padding: 15px;
  }

  .header {
    align-items: flex-start;
  }

  .brand h2 {
    font-size: 17px;
  }

  .status {
    font-size: 10px;
    padding: 9px;
  }

  .hero {
    grid-template-columns: 1fr;
    text-align: center;
    padding: 30px 0 20px;
  }

  .chip {
    display: none;
  }

  .hero h1 {
    font-size: 43px;
  }

  .panel {
    padding: 20px 16px;
  }

  .info {
    grid-template-columns: 1fr;
  }
}

</style>
</head>

<body>

<div class="container">

  <header class="header">

    <div class="brand">
      <div class="logo">⌂</div>
      <div>
        <h2>ESP32<span>-S3</span></h2>
        <p>SMART IoT SYSTEM</p>
      </div>
    </div>

    <div class="status">
      ● ACCESS POINT MODE<br>
      192.168.4.1
    </div>

  </header>


  <section class="hero">

    <div>
      <h1>WiFi <span>Setup</span></h1>
      <h3>ESP32-S3 SMART CONNECTIVITY</h3>

      <p>
        Hubungkan perangkat ESP32-S3 ke jaringan WiFi
        Anda untuk memulai pengalaman Smart IoT.
      </p>
    </div>

    <div class="chip">◉</div>

  </section>


  <section class="panel">

    <div class="panel-title">
      <div class="symbol">⌁</div>
      <div>
        <h2>Pilih Jaringan WiFi</h2>
        <p>Temukan dan hubungkan perangkat ke jaringan Anda</p>
      </div>
    </div>

    <form action="/save" method="POST"
          onsubmit="showLoading()">

      <label>Jaringan WiFi tersedia</label>

      <select id="wifi" onchange="pilihWifi()">
        <option value="">-- Pilih jaringan WiFi --</option>

  )rawliteral";

  html += networks;

  html += R"rawliteral(

      </select>

      <label>Nama WiFi (SSID)</label>

      <input
        type="text"
        id="ssid"
        name="ssid"
        placeholder="Masukkan nama WiFi"
        required
      >

      <label>Password WiFi</label>

      <div class="password-box">

        <input
          type="password"
          id="pass"
          name="pass"
          placeholder="Masukkan password WiFi"
        >

        <button
          type="button"
          class="show-btn"
          onclick="togglePassword()"
        >Lihat</button>

      </div>

      <button class="main-btn" type="submit">
        ◉ &nbsp; HUBUNGKAN WIFI →
      </button>

      <div class="loading" id="loading">
        Menyimpan konfigurasi WiFi... Mohon tunggu.
      </div>

    </form>

    <div class="info">

      <div class="info-card">
        <strong>ESP32-S3</strong>
        <small>Powerful MCU</small>
      </div>

      <div class="info-card">
        <strong>SMART IoT</strong>
        <small>Wireless Connectivity</small>
      </div>

      <div class="info-card">
        <strong>SECURE</strong>
        <small>WiFi Configuration</small>
      </div>

    </div>

  </section>


  <footer class="footer">
    SMART TECHNOLOGY <span>•</span> SMART LIFE
  </footer>

</div>


<script>

function pilihWifi() {
  document.getElementById("ssid").value =
    document.getElementById("wifi").value;
}

function togglePassword() {
  const pass = document.getElementById("pass");
  const btn = document.querySelector(".show-btn");

  if (pass.type === "password") {
    pass.type = "text";
    btn.innerText = "Sembunyi";
  } else {
    pass.type = "password";
    btn.innerText = "Lihat";
  }
}

function showLoading() {
  document.getElementById("loading").style.display = "block";
}

</script>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}


// ==========================================
// REDIRECT CAPTIVE PORTAL
// ==========================================
void handleCaptivePortal() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}


// ==========================================
// SIMPAN WIFI
// ==========================================
void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  ssid.trim();

  if (ssid.length() == 0) {
    server.send(400, "text/plain",
                "Nama WiFi tidak boleh kosong.");
    return;
  }

  preferences.begin("wifi", false);

  preferences.putString("ssid", ssid);
  preferences.putString("pass", pass);

  preferences.end();

  server.send(200, "text/html",
    "<html><body style='background:#061512;color:#42ff88;"
    "font-family:Arial;text-align:center;padding:50px'>"
    "<h2>WiFi Berhasil Disimpan!</h2>"
    "<p>ESP32-S3 sedang melakukan restart...</p>"
    "</body></html>");

  delay(1500);
  ESP.restart();
}


// ==========================================
// LOAD WIFI
// ==========================================
void loadWiFiConfig() {
  preferences.begin("wifi", true);

  wifiSSID = preferences.getString("ssid", "");
  wifiPASS = preferences.getString("pass", "");

  preferences.end();
}


// ==========================================
// KONEKSI WIFI
// ==========================================
bool connectWiFi() {
  Serial.println();
  Serial.println("Menghubungkan ke WiFi...");
  Serial.print("SSID: ");
  Serial.println(wifiSSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - startTime < 20000) {

    // Tombol BOOT juga dapat membatalkan percobaan koneksi
    if (digitalRead(BOOT_PIN) == LOW) {
      unsigned long pressStart = millis();

      while (digitalRead(BOOT_PIN) == LOW) {
        if (millis() - pressStart >= 5000) {
          Serial.println("Reset WiFi melalui BOOT...");

          preferences.begin("wifi", false);
          preferences.clear();
          preferences.end();

          delay(500);
          ESP.restart();
        }

        delay(20);
      }
    }

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi berhasil terhubung!");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    digitalWrite(LED_PIN, HIGH);

    return true;
  }

  Serial.println("Gagal terhubung ke WiFi.");

  WiFi.disconnect(true);

  return false;
}


// ==========================================
// ACCESS POINT
// ==========================================
void startAccessPoint() {
  apMode = true;

  Serial.println();
  Serial.println("================================");
  Serial.println("MODE CAPTIVE PORTAL AKTIF");
  Serial.println("Sensor HC-SR04 BELUM DIAKTIFKAN");
  Serial.println("================================");

  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);

  Serial.print("Nama WiFi: ");
  Serial.println(apSSID);

  Serial.print("Password: ");
  Serial.println(apPassword);

  Serial.print("Alamat Portal: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);

  server.on("/generate_204", HTTP_GET, handleCaptivePortal);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortal);
  server.on("/connecttest.txt", HTTP_GET, handleCaptivePortal);
  server.on("/redirect", HTTP_GET, handleCaptivePortal);

  server.onNotFound(handleCaptivePortal);

  server.begin();
}


// ==========================================
// DATA SENSOR
// ==========================================
void handleSensorData() {
  String json = "{";

  json += "\"distance\":" + String(distance, 1) + ",";
  json += "\"status\":\"" + getStatus(distance) + "\",";
  json += "\"color\":\"" + getColor(distance) + "\"";

  json += "}";

  server.send(200, "application/json", json);
}


// ==========================================
// DASHBOARD SENSOR
// ==========================================
void handleDashboard() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ESP32-S3 Monitoring</title>
    <style>
      body {
        background:#020d0b;
        color:white;
        font-family:Arial;
        text-align:center;
        padding:25px;
      }
      .card {
        background:#09251e;
        border:1px solid #35ff85;
        border-radius:20px;
        max-width:430px;
        margin:50px auto;
        padding:30px;
        box-shadow:0 0 25px #064e35;
      }
      h2 { color:#42ff87; }
      #distance {
        font-size:55px;
        font-weight:bold;
        margin:25px;
      }
      #status {
        font-size:23px;
        font-weight:bold;
      }
    </style>
  </head>
  <body>
    <div class="card">
      <h2>ESP32-S3 SMART MONITORING</h2>
      <p>HC-SR04 Distance Sensor</p>
      <div id="distance">-- cm</div>
      <p>Status Kondisi</p>
      <div id="status">Memuat...</div>
    </div>

    <script>
      async function updateData() {
        try {
          const response = await fetch('/data');
          const data = await response.json();

          document.getElementById('distance').innerText =
            data.distance < 0 ? '-- cm' : data.distance + ' cm';

          document.getElementById('status').innerText =
            data.status;

          document.getElementById('status').style.color =
            data.color;

        } catch (e) {
          document.getElementById('status').innerText =
            'Koneksi terputus';
        }
      }

      setInterval(updateData, 500);
      updateData();
    </script>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}


// ==========================================
// SERVER DASHBOARD
// ==========================================
void startStationServer() {
  apMode = false;

  server.on("/", HTTP_GET, handleDashboard);
  server.on("/dashboard", HTTP_GET, handleDashboard);
  server.on("/data", HTTP_GET, handleSensorData);

  server.onNotFound([]() {
    server.send(404, "text/plain", "Halaman tidak ditemukan");
  });

  server.begin();

  Serial.println("Dashboard sensor siap dibuka.");
  Serial.println("HC-SR04 mulai diaktifkan.");
}


// ==========================================
// RESET WIFI TOMBOL BOOT 5 DETIK
// ==========================================
void checkBootButton() {

  if (digitalRead(BOOT_PIN) == LOW) {

    if (!bootTiming) {
      bootPressStart = millis();
      bootTiming = true;

      Serial.println("Tombol BOOT ditekan...");
    }

    if (millis() - bootPressStart >= 5000 &&
        !bootResetDone) {

      bootResetDone = true;

      Serial.println("Menghapus konfigurasi WiFi...");

      preferences.begin("wifi", false);
      preferences.clear();
      preferences.end();

      Serial.println("WiFi berhasil dihapus!");
      Serial.println("ESP32-S3 restart...");

      delay(1000);

      WiFi.disconnect(true);

      ESP.restart();
    }

  } else {
    bootTiming = false;
    bootPressStart = 0;
    bootResetDone = false;
  }
}


// ==========================================
// SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  delay(1000);

  Serial.println();
  Serial.println("ESP32-S3 SMART IoT SYSTEM");

  loadWiFiConfig();

  if (wifiSSID.length() > 0) {
    if (connectWiFi()) {
      startStationServer();
    } else {
      startAccessPoint();
    }
  } else {
    startAccessPoint();
  }
}


// ==========================================
// LOOP
// ==========================================
void loop() {
  checkBootButton();

  server.handleClient();

  if (apMode) {
    dnsServer.processNextRequest();
  }

  // SENSOR HANYA AKTIF SETELAH WIFI TERHUBUNG
  if (!apMode && WiFi.status() == WL_CONNECTED) {

    if (millis() - previousMillis >= interval) {
      previousMillis = millis();

      distance = medianDistance();

      Serial.print("Jarak: ");

      if (distance < 0) {
        Serial.println("Tidak terdeteksi");
      } else {
        Serial.print(distance, 1);
        Serial.print(" cm - ");
        Serial.println(getStatus(distance));
      }
    }
  }
}