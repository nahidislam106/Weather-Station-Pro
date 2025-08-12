#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

#define DPIN 4         // DHT sensor pin
#define DTYPE DHT11    // DHT sensor type
#define RAIN_SENSOR_PIN 34  // Analog pin for rain sensor

// WiFi credentials
const char* ssid = "WeatherStationPro";
const char* password = "climate456";

DHT dht(DPIN, DTYPE);
WebServer server(80);

// Pressure simulation
float basePressure = 1013.25;
float pressureVariation = 0;
unsigned long lastPressureUpdate = 0;
const long pressureUpdateInterval = 30000;

// Rain sensor calibration
const int dryValue = 4095;  // Value when completely dry (3.3V)
const int wetValue = 1500;  // Value when completely wet
int rainSensorValue = 0;
bool isRaining = false;
float rainIntensity = 0.0;

String getSensorReadings() {
  // Read actual sensors
  float tc = dht.readTemperature(false);
  float tf = dht.readTemperature(true);
  float hu = dht.readHumidity();
  rainSensorValue = analogRead(RAIN_SENSOR_PIN);
  
  // Calculate rain intensity (0-100%)
  rainIntensity = map(rainSensorValue, wetValue, dryValue, 100, 0);
  rainIntensity = constrain(rainIntensity, 0, 100);
  isRaining = rainIntensity > 10;  // Threshold for rain detection

  // Update pressure simulation
  if (millis() - lastPressureUpdate > pressureUpdateInterval) {
    pressureVariation = random(-20, 20) / 10.0;
    lastPressureUpdate = millis();
  }
  float pressure = basePressure + pressureVariation;

  if (isnan(hu) || isnan(tc) || isnan(tf)) {
    return "null";
  }

  return String(tc,1) + "," + String(tf,1) + "," + String(hu,1) + "," + 
         String(pressure,1) + "," + String(rainIntensity,0) + "," + 
         String(isRaining ? "1" : "0");
}

String getUptime() {
  long seconds = millis() / 1000;
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  seconds = seconds % 60;
  return String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
}

String getWeatherCondition(float pressure) {
  if (pressure > 1022) return "High Pressure";
  if (pressure > 1009) return "Normal";
  if (pressure > 995) return "Low Pressure";
  return "Stormy";
}

void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Advanced Weather Station</title>
  <style>
    :root {
      --bg-dark: #121212;
      --card-bg: #1e1e1e;
      --text-primary: #e0e0e0;
      --text-secondary: #a0a0a0;
      --accent-blue: #3a86ff;
      --accent-orange: #ff7b25;
      --accent-green: #4cc9f0;
      --accent-purple: #8338ec;
      --accent-rain: #00b4d8;
      --status-good: #4ade80;
      --status-warn: #fbbf24;
      --status-bad: #f87171;
    }
    
    body {
      font-family: 'Segoe UI', Roboto, sans-serif;
      background-color: var(--bg-dark);
      color: var(--text-primary);
      margin: 0;
      padding: 20px;
      line-height: 1.6;
    }
    
    .container {
      max-width: 1000px;
      margin: 0 auto;
    }
    
    header {
      text-align: center;
      margin-bottom: 30px;
    }
    
    h1 {
      color: var(--accent-blue);
      font-weight: 300;
      letter-spacing: 1px;
      margin-bottom: 5px;
    }
    
    .subtitle {
      color: var(--text-secondary);
      font-size: 0.9rem;
    }
    
    .dashboard {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 20px;
      margin-top: 30px;
    }
    
    .card {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 20px;
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
      transition: transform 0.2s;
    }
    
    .card:hover {
      transform: translateY(-2px);
    }
    
    .card.rain {
      border-top: 3px solid var(--accent-rain);
    }
    
    .card-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
    }
    
    .card-title {
      font-size: 1rem;
      font-weight: 500;
      color: var(--text-secondary);
      margin: 0;
    }
    
    .card-value {
      font-size: 2rem;
      font-weight: 300;
      margin: 10px 0;
    }
    
    .unit {
      font-size: 1rem;
      color: var(--text-secondary);
    }
    
    .weather-condition {
      font-size: 0.9rem;
      margin-top: 5px;
    }
    
    .rain-status {
      display: inline-block;
      padding: 3px 8px;
      border-radius: 12px;
      font-size: 0.8rem;
      font-weight: 500;
      background-color: rgba(0, 180, 216, 0.2);
      color: var(--accent-rain);
    }
    
    .rain-intensity {
      height: 6px;
      background: #333;
      border-radius: 3px;
      margin-top: 10px;
      overflow: hidden;
    }
    
    .rain-intensity-bar {
      height: 100%;
      background: linear-gradient(90deg, var(--accent-rain), #00e1ff);
      width: 0%;
      transition: width 1s ease;
    }
    
    .status {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: var(--status-good);
    }
    
    .status.warn {
      background: var(--status-warn);
    }
    
    .status.bad {
      background: var(--status-bad);
    }
    
    .meta {
      font-size: 0.8rem;
      color: var(--text-secondary);
      text-align: center;
      margin-top: 40px;
    }
    
    .pressure-trend, .rain-trend {
      display: flex;
      align-items: center;
      margin-top: 10px;
      font-size: 0.9rem;
    }
    
    .trend-icon {
      margin-right: 8px;
      font-size: 1.2rem;
    }
    
    @media (max-width: 600px) {
      .dashboard {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>ADVANCED WEATHER STATION</h1>
      <div class="subtitle">Comprehensive environmental monitoring</div>
    </header>
    
    <div class="dashboard">
      <div class="card">
        <div class="card-header">
          <h2 class="card-title">TEMPERATURE</h2>
          <span class="status" id="temp-status"></span>
        </div>
        <div class="card-value" id="temp-c">--.-</div>
        <div class="card-value" id="temp-f">--.-</div>
      </div>
      
      <div class="card">
        <div class="card-header">
          <h2 class="card-title">HUMIDITY</h2>
          <span class="status" id="hum-status"></span>
        </div>
        <div class="card-value" id="humidity">--.-</div>
      </div>
      
      <div class="card">
        <div class="card-header">
          <h2 class="card-title">BAROMETRIC PRESSURE</h2>
          <span class="status" id="pressure-status"></span>
        </div>
        <div class="card-value" id="pressure">----.-</div>
        <div class="weather-condition" id="weather-condition">--</div>
        <div class="pressure-trend">
          <span class="trend-icon" id="pressure-trend-icon">→</span>
          <span id="pressure-trend">Pressure stable</span>
        </div>
      </div>
      
      <div class="card rain">
        <div class="card-header">
          <h2 class="card-title">PRECIPITATION</h2>
          <span class="rain-status" id="rain-status">DRY</span>
        </div>
        <div class="card-value" id="rain-intensity">--%</div>
        <div class="rain-intensity">
          <div class="rain-intensity-bar" id="rain-intensity-bar"></div>
        </div>
        <div class="rain-trend">
          <span class="trend-icon" id="rain-trend-icon">☀️</span>
          <span id="rain-trend">No precipitation</span>
        </div>
      </div>
    </div>
    
    <div class="meta">
      <div>Last update: <span id="last-update">just now</span></div>
      <div>Uptime: <span id="uptime">0h 0m 0s</span></div>
      <div>Sensor: DHT11 | Rain Sensor: Active | Pressure: Simulated | v1.2</div>
    </div>
  </div>

  <script>
    let lastPressure = 0;
    let lastRainIntensity = 0;
    let trendTimeout;
    let rainTrendTimeout;
    
    function updateReadings() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          if (data === 'null') {
            console.error('Sensor error');
            return;
          }
          
          const [tempC, tempF, humidity, pressure, rainIntensity, isRaining] = data.split(',');
          const now = new Date();
          const timeString = now.toLocaleTimeString();
          
          // Update display values
          document.getElementById('temp-c').innerHTML = `${tempC} <span class="unit">°C</span>`;
          document.getElementById('temp-f').innerHTML = `${tempF} <span class="unit">°F</span>`;
          document.getElementById('humidity').innerHTML = `${humidity} <span class="unit">% RH</span>`;
          document.getElementById('pressure').innerHTML = `${pressure} <span class="unit">hPa</span>`;
          document.getElementById('rain-intensity').innerHTML = `${rainIntensity}<span class="unit">%</span>`;
          document.getElementById('last-update').textContent = timeString;
          
          // Update weather condition
          document.getElementById('weather-condition').textContent = 
            getWeatherCondition(parseFloat(pressure));
          
          // Update pressure trend
          updatePressureTrend(parseFloat(pressure));
          lastPressure = parseFloat(pressure);
          
          // Update rain display
          updateRainDisplay(parseInt(rainIntensity), isRaining === '1');
          lastRainIntensity = parseInt(rainIntensity);
          
          // Update status indicators
          updateStatus('temp-status', tempC, 18, 28);
          updateStatus('hum-status', humidity, 30, 60);
          updateStatus('pressure-status', pressure, 990, 1030);
        })
        .catch(error => console.error('Error:', error));
    }
    
    function getWeatherCondition(pressure) {
      if (pressure > 1022) return "High Pressure";
      if (pressure > 1009) return "Normal";
      if (pressure > 995) return "Low Pressure";
      return "Stormy";
    }
    
    function updatePressureTrend(currentPressure) {
      const trendElement = document.getElementById('pressure-trend');
      const iconElement = document.getElementById('pressure-trend-icon');
      
      clearTimeout(trendTimeout);
      
      if (lastPressure === 0) {
        trendElement.textContent = "Initial reading";
        iconElement.textContent = "→";
        return;
      }
      
      const difference = currentPressure - lastPressure;
      const absDifference = Math.abs(difference);
      
      if (absDifference < 0.3) {
        trendElement.textContent = "Pressure stable";
        iconElement.textContent = "→";
      } else if (difference > 0) {
        trendElement.textContent = `Rising (${absDifference.toFixed(1)} hPa)`;
        iconElement.textContent = "↑";
        iconElement.style.color = "#4ade80";
      } else {
        trendElement.textContent = `Falling (${absDifference.toFixed(1)} hPa)`;
        iconElement.textContent = "↓";
        iconElement.style.color = "#f87171";
      }
      
      trendTimeout = setTimeout(() => {
        trendElement.textContent = "Pressure stable";
        iconElement.textContent = "→";
        iconElement.style.color = "";
      }, 30000);
    }
    
    function updateRainDisplay(intensity, raining) {
      const statusElement = document.getElementById('rain-status');
      const barElement = document.getElementById('rain-intensity-bar');
      const trendElement = document.getElementById('rain-trend');
      const iconElement = document.getElementById('rain-trend-icon');
      
      // Update intensity bar
      barElement.style.width = `${intensity}%`;
      
      // Update status text
      if (intensity < 5) {
        statusElement.textContent = "DRY";
        statusElement.style.backgroundColor = "rgba(100, 100, 100, 0.2)";
        statusElement.style.color = "var(--text-secondary)";
      } else if (intensity < 30) {
        statusElement.textContent = "DAMP";
        statusElement.style.backgroundColor = "rgba(0, 180, 216, 0.3)";
        statusElement.style.color = "var(--accent-rain)";
      } else if (intensity < 70) {
        statusElement.textContent = "WET";
        statusElement.style.backgroundColor = "rgba(0, 180, 216, 0.5)";
        statusElement.style.color = "var(--accent-rain)";
      } else {
        statusElement.textContent = "RAINING";
        statusElement.style.backgroundColor = "rgba(0, 180, 216, 0.8)";
        statusElement.style.color = "white";
      }
      
      // Update trend information
      clearTimeout(rainTrendTimeout);
      
      if (raining) {
        if (intensity > lastRainIntensity) {
          trendElement.textContent = "Rain increasing";
          iconElement.textContent = "🌧️";
        } else if (intensity < lastRainIntensity) {
          trendElement.textContent = "Rain decreasing";
          iconElement.textContent = "🌦️";
        } else {
          trendElement.textContent = "Steady rain";
          iconElement.textContent = "🌧️";
        }
      } else {
        if (lastRainIntensity > 0) {
          trendElement.textContent = "Rain stopped";
          iconElement.textContent = "⛅";
        } else {
          trendElement.textContent = "No precipitation";
          iconElement.textContent = "☀️";
        }
      }
      
      rainTrendTimeout = setTimeout(() => {
        if (raining) {
          trendElement.textContent = "Rain continuing";
          iconElement.textContent = "🌧️";
        } else {
          trendElement.textContent = "Dry conditions";
          iconElement.textContent = "☀️";
        }
      }, 30000);
    }
    
    function updateStatus(elementId, value, minGood, maxGood) {
      const element = document.getElementById(elementId);
      const numValue = parseFloat(value);
      
      element.className = 'status';
      if (numValue < minGood || numValue > maxGood) {
        element.classList.add('bad');
      } else if (numValue < minGood * 1.1 || numValue > maxGood * 0.9) {
        element.classList.add('warn');
      } else {
        element.classList.add('good');
      }
    }
    
    function updateUptime() {
      fetch('/uptime')
        .then(response => response.text())
        .then(uptime => {
          document.getElementById('uptime').textContent = uptime;
        });
    }
    
    // Initial update
    updateReadings();
    updateUptime();
    
    // Update every 2 seconds
    setInterval(updateReadings, 2000);
    setInterval(updateUptime, 1000);
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

void handleData() {
  server.send(200, "text/plain", getSensorReadings());
}

void handleUptime() {
  server.send(200, "text/plain", getUptime());
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  
  // Initialize sensors
  dht.begin();
  pinMode(RAIN_SENSOR_PIN, INPUT);
  
  // Create WiFi access point
  WiFi.softAP(ssid, password);
  
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  
  // Set up server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/uptime", handleUptime);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  delay(100);
}