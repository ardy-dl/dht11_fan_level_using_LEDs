#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define ALERT_LED 2
#define BUTTON_UP 32
#define BUTTON_DOWN 33

int fan1 = 13, fan2 = 12, fan3 = 14, fan4 = 27;

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

float myTemp = 0.0;
float myHum = 0.0;
int fanLevel = 0; 
unsigned long timer = 0;

void syncHardware() {
  digitalWrite(fan1, fanLevel >= 1);
  digitalWrite(fan2, fanLevel >= 2);
  digitalWrite(fan3, fanLevel >= 3);
  digitalWrite(fan4, fanLevel >= 4);
}

void serveMainPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>IoT Modular Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --bg-gradient: linear-gradient(135deg, #fce4ec 0%, #e1f5fe 100%);
            --card-bg: rgba(255, 255, 255, 0.8);
            --accent-pink: #f06292;
            --accent-blue: #03a9f4;
            --text-main: #444;
        }
        body { font-family: 'Segoe UI', system-ui, sans-serif; background: var(--bg-gradient); background-attachment: fixed; margin: 0; padding: 20px 10px; color: var(--text-main); display: flex; flex-direction: column; align-items: center; }
        header { text-align: center; margin-bottom: 20px; width: 100%; }
        header h1 { font-size: 1.6rem; margin: 0; color: #333; }
        header p { font-size: 0.85rem; margin: 5px 0; opacity: 0.7; }
        .container { display: flex; flex-wrap: wrap; justify-content: center; gap: 15px; width: 100%; max-width: 500px; }
        .card { background: var(--card-bg); backdrop-filter: blur(10px); border-radius: 20px; padding: 20px; box-shadow: 0 8px 20px rgba(0,0,0,0.05); border: 1px solid rgba(255, 255, 255, 0.5); box-sizing: border-box; }
        .card-half { flex: 1 1 calc(50% - 8px); min-width: 140px; text-align: center; }
        .card-full { flex: 1 1 100%; }
        .label { font-size: 0.7rem; text-transform: uppercase; font-weight: 600; color: #888; letter-spacing: 0.5px; }
        .value { font-size: 2rem; font-weight: 800; margin: 5px 0; }
        .temp-color { color: var(--accent-pink); }
        .hum-color { color: var(--accent-blue); }
        .fan-ui { display: flex; align-items: center; justify-content: space-between; margin-top: 15px; }
        .btn { width: 44px; height: 44px; border: none; border-radius: 50%; background: white; font-size: 1.2rem; cursor: pointer; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
        .led-bar { display: flex; flex: 1; gap: 4px; margin: 0 15px; }
        .led { flex: 1; height: 8px; background: #ddd; border-radius: 4px; transition: 0.3s; }
        .led.active { background: var(--accent-blue); box-shadow: 0 0 8px rgba(3, 169, 244, 0.3); }
        .chart-container { height: 200px; width: 100%; }
        .footer-link { margin-top: 30px; font-size: 0.75rem; color: #666; cursor: pointer; text-decoration: underline; text-align: center; }
        .modal { display: none; position: fixed; z-index: 100; left: 0; top: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.3); backdrop-filter: blur(5px); }
        .modal-content { background: white; margin: 20% auto; padding: 25px; border-radius: 25px; width: 85%; max-width: 320px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); animation: slideUp 0.3s ease; }
        @keyframes slideUp { from { transform: translateY(20px); opacity: 0; } to { transform: translateY(0); opacity: 1; } }
        .modal h2 { font-size: 1.1rem; margin-top: 0; border-bottom: 2px solid #fce4ec; padding-bottom: 8px; }
        .member-list { list-style: none; padding: 0; text-align: left; font-size: 0.85rem; line-height: 1.8; color: #555; }
        .close-btn { width: 100%; padding: 10px; margin-top: 15px; border: none; background: #fce4ec; border-radius: 12px; cursor: pointer; font-weight: bold; }
    </style>
</head>
<body>
<header><h1>Smart Lab</h1><p>NodeMCU Live Dashboard</p></header>
<main class="container">
    <section class="card card-half"><span class="label">Temperature</span><div class="value temp-color"><span id="temp">--</span>°C</div></section>
    <section class="card card-half"><span class="label">Humidity</span><div class="value hum-color"><span id="hum">--</span>%</div></section>
    <section class="card card-full">
        <span class="label" style="display:block; text-align:center;">Fan Speed Level: <span id="lvl">0</span>/4</span>
        <div class="fan-ui"><button class="btn" onclick="updateFan(-1)">−</button><div class="led-bar" id="led-container"></div><button class="btn" onclick="updateFan(1)">+</button></div>
    </section>
    <section class="card card-full"><div class="chart-container"><canvas id="liveChart"></canvas></div></section>
</main>
<div class="footer-link" onclick="toggleModal(true)">Laboratory Activity No. 2 - NodeMCU: DHT11 over Wi-Fi</div>
<div id="memberModal" class="modal" onclick="toggleModal(false)">
    <div class="modal-content" onclick="event.stopPropagation()">
        <h2>Group Members</h2><ul class="member-list">
            <li>Balangao, Samantha A.</li><li>De Leon, Rose Denise J.</li><li>Ernia, Franchesca Jean N.</li><li>Herras, Trixie Mae E.</li><li>Legaspi, Lana Cazandra U.</li><li>Malabo, Reniel A.</li><li>Perez, Lea Nikka F.</li><li>Ronio, Ryza Mae P.</li><li>Zurbito, Pierre Victor T.</li>
        </ul><button class="close-btn" onclick="toggleModal(false)">Close</button>
    </div>
</div>
<script>
    function toggleModal(show) { document.getElementById('memberModal').style.display = show ? 'block' : 'none'; }
    let currentFanLevel = 0;
    const ledContainer = document.getElementById('led-container');
    const ctx = document.getElementById('liveChart').getContext('2d');
    
    const liveChart = new Chart(ctx, {
        type: 'line',
        data: { 
            labels: [], 
            datasets: [
                { label: 'Temp', borderColor: '#f06292', borderWidth: 2, pointRadius: 0, data: [], tension: 0.4 },
                { label: 'Hum', borderColor: '#03a9f4', borderWidth: 2, pointRadius: 0, data: [], tension: 0.4 }
            ] 
        },
        options: { 
            responsive: true, 
            maintainAspectRatio: false, 
            plugins: { legend: { display: true, labels: { boxWidth: 10, font: { size: 10 } } } }, 
            scales: { x: { display: false }, y: { display: true } } 
        }
    });

    function drawLEDs(level) {
        ledContainer.innerHTML = '';
        for(let i=1; i<=4; i++) {
            const led = document.createElement('div');
            led.className = i <= level ? 'led active' : 'led';
            ledContainer.appendChild(led);
        }
        document.getElementById('lvl').innerText = level;
    }

    function updateFan(dir) { fetch(dir > 0 ? '/increase' : '/decrease').then(() => fetchData()); }

    function fetchData() {
        fetch('/data').then(res => res.json()).then(data => {
            document.getElementById('temp').innerText = data.temperature;
            document.getElementById('hum').innerText = data.humidity;
            currentFanLevel = data.fan;
            drawLEDs(currentFanLevel);

            liveChart.data.labels.push("");
            liveChart.data.datasets[0].data.push(data.temperature);
            liveChart.data.datasets[1].data.push(data.humidity);

            if(liveChart.data.labels.length > 20) {
                liveChart.data.labels.shift();
                liveChart.data.datasets[0].data.shift();
                liveChart.data.datasets[1].data.shift();
            }
            liveChart.update();
        });
    }
    setInterval(fetchData, 2000);
    fetchData(); 
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void serveJSON() {
  String out = "{";
  out += "\"temperature\": " + String(myTemp) + ",";
  out += "\"humidity\": " + String(myHum) + ",";
  out += "\"fan\": " + String(fanLevel);
  out += "}";
  server.send(200, "application/json", out);
}

void goUp() {
  if (fanLevel < 4) fanLevel++;
  syncHardware();
  server.send(200, "text/plain", "OK");
}

void goDown() {
  if (fanLevel > 0) fanLevel--;
  syncHardware();
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(ALERT_LED, OUTPUT);
  pinMode(fan1, OUTPUT); pinMode(fan2, OUTPUT);
  pinMode(fan3, OUTPUT); pinMode(fan4, OUTPUT);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  WiFi.begin("Wokwi-GUEST", "");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nReady! IP: " + WiFi.localIP().toString());

  server.on("/", serveMainPage);
  server.on("/data", serveJSON);
  server.on("/increase", goUp);
  server.on("/decrease", goDown);
  server.begin();
  
  syncHardware();
}

void loop() {
  server.handleClient();

  if (millis() - timer >= 2000) {
    timer = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (t > 0 && h > 0) {
      myTemp = t;
      myHum = h;
    }

    if (myTemp > 33.0) digitalWrite(ALERT_LED, HIGH);
    else digitalWrite(ALERT_LED, LOW);
  }

  if (digitalRead(BUTTON_UP) == LOW) {
    if (fanLevel < 4) { fanLevel++; syncHardware(); }
    delay(200); 
  }
  if (digitalRead(BUTTON_DOWN) == LOW) {
    if (fanLevel > 0) { fanLevel--; syncHardware(); }
    delay(200); 
  }
}