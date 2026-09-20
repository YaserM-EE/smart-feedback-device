#include <WiFi.h>
#include <WebServer.h>

// ---------------------------------------------------------
// 1. Configuration 
// ---------------------------------------------------------
const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PASSWORD";

// Start a web server on port 80 (standard HTTP port)
WebServer server(80);

// ---------------------------------------------------------
// 2. Hardware Setup (Button Pins)
// ---------------------------------------------------------
// These are the GPIO pins where we will connect our buttons
const int btnUnderstand = 4;
const int btnConfused = 5;
const int btnLost = 32;
const int btnSlowDown = 13;
const int btnRepeatThat = 14;
const int btnYes = 15;
const int btnNo = 18;
const int btnNeedExample = 19;

// Grouping them in arrays makes it easier to use loops later
const int buttonPins[] = {btnUnderstand, btnConfused, btnLost, btnSlowDown, btnRepeatThat, btnYes, btnNo, btnNeedExample};
const int numButtons = 8;
const char* buttonLabels[] = {"Understand", "Confused", "Lost", "Slow down", "Repeat that!", "Yes", "No", "Need an Example"};

// ---------------------------------------------------------
// 3. Variables to store counts and debounce states
// ---------------------------------------------------------
// This array keeps track of how many times each button is pressed
int buttonCounts[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Debouncing variables (prevents one physical press from registering as multiple rapid presses)
unsigned long lastDebounceTime[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int lastButtonState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH}; // HIGH means unpressed
int buttonState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
unsigned long debounceDelay = 50; // 50 milliseconds delay to confirm a valid press

// ---------------------------------------------------------
// 3.5 RGB LED Configuration (Feedback)
// ---------------------------------------------------------
const int rgbGreen = 26;
unsigned long ledTurnOnTime = 0;
bool isLedOn = false;
const unsigned long ledFlashDuration = 250; // Flash for 250 milliseconds

// ---------------------------------------------------------
// 4. HTML Dashboard Code
// ---------------------------------------------------------
// We use a "raw string literal" here so we can write normal HTML across multiple lines easily.
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Classroom Feedback</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { 
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; 
      background: linear-gradient(135deg, #e0eafc 0%, #cfdef3 100%); 
      color: #1f2937; 
      margin: 0; 
      padding: 3rem 1rem; 
      text-align: center; 
      min-height: 100vh;
    }
    .container { max-width: 1200px; margin: 0 auto; }
    .chart-summary-layout { display: flex; gap: 2rem; align-items: stretch; flex-wrap: nowrap; margin-bottom: 2rem; }
    h1 { 
      font-size: 2.2rem; 
      margin-bottom: 2rem; 
      color: #111827;
      display: flex; align-items: center; justify-content: center; gap: 12px; 
    }
    .status { 
      font-size: 0.85rem; background: #dcfce7; color: #166534; padding: 6px 14px; 
      border-radius: 999px; font-weight: 600; display: inline-flex; align-items: center; gap: 8px; 
      border: 1px solid #bbf7d0; box-shadow: 0 2px 4px rgba(0,0,0,0.05);
    }
    .dot { width: 8px; height: 8px; background: #22c55e; border-radius: 50%; box-shadow: 0 0 8px #22c55e; }
    .stats { display: flex; gap: 1.5rem; margin-bottom: 2rem; flex-wrap: wrap; }
    .card { 
      background: #ffffff; padding: 2rem; border-radius: 16px; 
      border: 1px solid rgba(255,255,255,0.5); flex: 1; min-width: 200px;
      box-shadow: 0 10px 25px rgba(0,0,0,0.05); transition: transform 0.2s;
    }
    .card:hover { transform: translateY(-2px); }
    .card h3 { margin: 0; font-size: 0.9rem; color: #6b7280; text-transform: uppercase; letter-spacing: 0.1em; }
    .card p { margin: 0.5rem 0 0; font-size: 3rem; font-weight: 800; color: #2563eb; }
    .top-resp { color: #ea580c !important; }
    .chart-container { 
      background: #ffffff; padding: 2rem; border-radius: 16px; 
      margin-bottom: 0; box-shadow: 0 10px 25px rgba(0,0,0,0.05); flex: 2; min-width: 0; 
    }
    .controls { 
      background: rgba(255,255,255,0.8); padding: 1.5rem; border-radius: 16px; 
      backdrop-filter: blur(10px); text-align: left; margin-bottom: 1.5rem; 
      box-shadow: 0 4px 6px rgba(0,0,0,0.05);
    }
    input { 
      width: 100%; max-width: 450px; padding: 0.75rem; margin-top: 0.75rem; 
      border: 1px solid #d1d5db; border-radius: 8px; font-family: inherit; font-size: 0.95rem;
      transition: border-color 0.2s, box-shadow 0.2s;
    }
    input:focus { outline: none; border-color: #3b82f6; box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.2); }
    .actions { margin-top: 1rem; display: flex; gap: 1rem; justify-content: center; flex-wrap: wrap; }
    button { 
      padding: 0.8rem 1.5rem; font-size: 1rem; font-weight: 600; cursor: pointer; 
      border-radius: 8px; border: none; font-family: inherit; 
      transition: all 0.2s; box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
    .btn-ai { background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%); color: white; }
    .btn-ai:hover { transform: translateY(-2px); box-shadow: 0 6px 12px rgba(37, 99, 235, 0.2); }
    .btn-reset { background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%); color: white; }
    .btn-reset:hover { transform: translateY(-2px); box-shadow: 0 6px 12px rgba(220, 38, 38, 0.2); }
    #summary { 
      flex: 1; min-width: 250px; margin-top: 0; padding: 2rem; background: #ffffff; border-radius: 16px; 
      border-left: 5px solid #3b82f6; display: none; text-align: left; 
      line-height: 1.6; box-shadow: 0 10px 25px rgba(0,0,0,0.05); font-size: 1.1rem;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Live Feedback <span class="status"><div class="dot"></div>ESP32 Online</span></h1>
    
    <div class="stats">
      <div class="card"><h3>Total Responses</h3><p id="totalCount">0</p></div>
      <div class="card"><h3>Top Response</h3><p class="top-resp" id="topResponse">-</p></div>
    </div>
    
    <div class="chart-summary-layout">
      <div class="chart-container"><canvas id="chart"></canvas></div>
      <div id="summary"></div>
    </div>
    
    <div class="controls">
      <label style="font-weight:600;font-size:0.95rem;color:#4b5563;">Gemini API Key (Optional)</label><br>
      <input type="text" id="apiKey" placeholder="Paste your key to test AI generation...">
    </div>
    
    <div class="actions">
      <button class="btn-ai" onclick="getSummary()">Generate AI Summary</button>
      <button class="btn-reset" onclick="resetData()">Reset Data</button>
    </div>
  </div>

  <script>
    const labels = ["Understand", "Confused", "Lost", "Slow down", "Repeat that!", "Yes", "No", "Need an Example"];
    const colors = ['#10b981', '#f59e0b', '#ef4444', '#eab308', '#8b5cf6', '#3b82f6', '#6b7280', '#14b8a6'];
    const ctx = document.getElementById('chart').getContext('2d');
    
    let chart = new Chart(ctx, {
      type: 'bar',
      data: { labels, datasets: [{ data: Array(8).fill(0), backgroundColor: colors, borderRadius: 6 }] },
      options: { 
        responsive: true, 
        plugins: { legend: { display: false } }, 
        scales: { 
          y: { beginAtZero: true, ticks: { stepSize: 1 }, grid: { color: 'rgba(0,0,0,0.05)' } }, 
          x: { grid: { display: false }, ticks: { font: { weight: '600' } } } 
        },
        animation: { duration: 400 } 
      }
    });

    const updateStats = (counts) => {
      const total = counts.reduce((a, b) => a + b, 0);
      document.getElementById('totalCount').innerText = total;
      document.getElementById('topResponse').innerText = total === 0 ? "-" : labels[counts.indexOf(Math.max(...counts))];
    };

    const updateData = () => {
      fetch('/data').then(r => r.json()).then(d => {
        chart.data.datasets[0].data = d.counts;
        chart.update();
        updateStats(d.counts);
      }).catch(e => console.error(e));
    };

    const resetData = () => {
      fetch('/reset').then(updateData).catch(e => console.error(e));
      document.getElementById('summary').style.display = 'none';
    };

    setInterval(updateData, 2000);

    const getSummary = () => {
      const key = document.getElementById('apiKey').value.trim();
      const sumDiv = document.getElementById('summary');
      sumDiv.style.display = 'block';
      if (!key) return sumDiv.innerHTML = "<b style='color:#dc2626'>Error: Please paste a Gemini API Key above to test the AI.</b>";
      
      sumDiv.innerHTML = "<i>✨ Asking Gemini AI...</i>";
      const counts = chart.data.datasets[0].data;
      const dataStr = labels.map((l, i) => `${l}: ${counts[i]}`).join(', ');
      
      fetch(`https://generativelanguage.googleapis.com/v1beta/models/gemini-flash-latest:generateContent?key=${key}`, {
        method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ contents: [{ parts: [{ text: `You are a teaching assistant viewing real-time classroom feedback. The students pressed buttons with these counts: ${dataStr}. Provide a single-sentence summary of the classroom's comprehension level and whether the professor should adjust their pace.` }] }] })
      }).then(r => r.json()).then(d => {
        if(d.candidates) sumDiv.innerHTML = `<b>✨ AI Summary:</b><br><br>${d.candidates[0].content.parts[0].text}`;
        else sumDiv.innerHTML = `<b style='color:#dc2626'>API Error: ${d.error?.message || 'Failed to generate content.'}</b>`;
      }).catch(() => sumDiv.innerHTML = "<b style='color:#dc2626'>Failed to connect to the Gemini API. Check your internet connection.</b>");
    };
  </script>
</body>
</html>
)rawliteral";

// ---------------------------------------------------------
// 5. Server Route Handlers
// ---------------------------------------------------------

// What happens when the user goes to the main IP address
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// What happens when the dashboard JavaScript fetches the latest data (http://IP/data)
void handleData() {
  String json = "{\"counts\": [";
  for (int i = 0; i < numButtons; i++) {
    json += String(buttonCounts[i]);
    if (i < numButtons - 1) {
      json += ", ";
    }
  }
  json += "]}";
  
  server.send(200, "application/json", json);
}

// What happens when the professor clicks the Reset Data button (http://IP/reset)
void handleReset() {
  for (int i = 0; i < numButtons; i++) {
    buttonCounts[i] = 0;
  }
  server.send(200, "text/plain", "OK");
  Serial.println("Data reset by professor.");
}

// ---------------------------------------------------------
// 6. Setup (Runs once on startup)
// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // Setup Green LED pin as output and ensure it is OFF
  pinMode(rgbGreen, OUTPUT);
  digitalWrite(rgbGreen, LOW);

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected!");
  Serial.print("Go to this IP Address in your browser: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);    
  server.on("/data", handleData); 
  server.on("/reset", handleReset);
  
  server.begin();
  Serial.println("Web server started.");
}

// ---------------------------------------------------------
// 7. Main Loop (Runs continuously)
// ---------------------------------------------------------
void loop() {
  server.handleClient();
  
  for (int i = 0; i < numButtons; i++) {
    int reading = digitalRead(buttonPins[i]);
    
    if (reading != lastButtonState[i]) {
      lastDebounceTime[i] = millis();
    }
    
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != buttonState[i]) {
        buttonState[i] = reading; 
        
        if (buttonState[i] == LOW) {
          buttonCounts[i]++; 
          Serial.print(buttonLabels[i]);
          Serial.print(" button pressed! Total count: ");
          Serial.println(buttonCounts[i]);
          
          // Flash the Green LED to indicate success
          digitalWrite(rgbGreen, HIGH);
          ledTurnOnTime = millis();
          isLedOn = true;
        }
      }
    }
    lastButtonState[i] = reading;
  }
  
  // Turn off Green LED if flash duration has passed
  if (isLedOn && (millis() - ledTurnOnTime >= ledFlashDuration)) {
    digitalWrite(rgbGreen, LOW);
    isLedOn = false;
  }
}
