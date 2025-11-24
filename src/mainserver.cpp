#include "mainserver.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ESPmDNS.h>

bool led1_state = false;
bool led2_state = false;
bool isAPMode = true;

WebServer server(80);
Adafruit_NeoPixel neoPixel(1, LED2_PIN, NEO_GRB + NEO_KHZ800);

unsigned long connect_start_ms = 0;
bool connecting = false;

// Thêm biến để tracking connection status
String connectionStatus = "idle";
String newIPAddress = "";

String mainPage() {
  float temperature = glob_temperature;
  float humidity = glob_humidity;
  float score = glob_anomaly_score;
  String message = glob_anomaly_message;
  String led1 = led1_state ? "ON" : "OFF";
  String led2 = led2_state ? "ON" : "OFF";
  String location_line1 = "";
  String location_line2 = "";
  String location = location_label;

  String thanh_pho = "Thành phố";
  int thanh_pho_pos = location.indexOf(thanh_pho);
  if (thanh_pho_pos >= 0) {
    location_line1 = thanh_pho;
    int start_pos = thanh_pho_pos + thanh_pho.length();
    if (start_pos < location.length()) {
      String remaining = location.substring(start_pos);
      while (remaining.length() > 0 && remaining.charAt(0) == ' ') {
        remaining = remaining.substring(1);
      }
      location_line2 = remaining;
    }
  } else {
    int space_pos = location.indexOf(" ");
    if (space_pos > 0) {
      location_line1 = location.substring(0, space_pos);
      location_line2 = location.substring(space_pos + 1);
    } else {
      location_line1 = location;
    }
  }

  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta charset='utf-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>ESP32 Dashboard</title>
      <style>
        :root{
          --bg:#0f172a;
          --card:#111827;
          --muted:#9ca3af;
          --text:#e5e7eb;
          --primary:#3b82f6;
          --accent:#22d3ee;
          --success:#22c55e;
          --warning:#f59e0b;
          --shadow:0 10px 30px rgba(0,0,0,0.35);
          --radius:16px;
        }
        *{box-sizing:border-box}
        body {
          margin:0;
          font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, "Noto Sans", "Apple Color Emoji", "Segoe UI Emoji";
          color:var(--text);
          background:
            radial-gradient(1200px 600px at -10% -10%, rgba(59,130,246,.15), transparent 40%),
            radial-gradient(800px 400px at 120% 10%, rgba(34,211,238,.12), transparent 40%),
            linear-gradient(180deg, #0b1220, #0f172a 30%, #0b1220 100%);
          min-height:100vh;
          display:flex;
          align-items:center;
          justify-content:center;
          padding:24px;
        }
        .container {
          width:100%;
          max-width:600px;
          background: linear-gradient(180deg, rgba(255,255,255,0.04), rgba(255,255,255,0.02)) , var(--card);
          border:1px solid rgba(255,255,255,0.08);
          border-radius: var(--radius);
          box-shadow: var(--shadow);
          padding: 22px 22px 18px;
          backdrop-filter: blur(8px);
        }
        .header{
          display:flex;
          align-items:center;
          justify-content:space-between;
          margin-bottom:16px;
        }
        .title{
          display:flex;
          align-items:center;
          gap:10px;
          font-weight:700;
          letter-spacing:.3px;
        }
        .title .logo{
          width:32px;height:32px;border-radius:9px;
          background: conic-gradient(from 180deg at 50% 50%, var(--accent), var(--primary), var(--accent));
          filter: drop-shadow(0 6px 14px rgba(59,130,246,.45));
        }
        .settings-btn{
          appearance:none;border:0;
          background: linear-gradient(180deg, rgba(59,130,246,.25), rgba(59,130,246,.15));
          color:white;
          padding:10px 14px;
          border-radius:10px;
          cursor:pointer;
          font-weight:600;
          border:1px solid rgba(59,130,246,.45);
          transition: transform .08s ease, box-shadow .2s ease, background .2s ease;
          box-shadow: 0 6px 18px rgba(59,130,246,.25);
        }
        .settings-btn:hover{ transform: translateY(-1px); }
        .grid{
          display:grid;
          grid-template-columns: 1fr 1fr;
          grid-auto-rows: 1fr;
          align-items: stretch;
          gap:12px;
          margin: 10px 0 6px;
        }
        .metric{
          position:relative;
          overflow:hidden;
          background: linear-gradient(180deg, rgba(255,255,255,0.06), rgba(255,255,255,0.02));
          border:1px solid rgba(255,255,255,0.08);
          border-radius:14px;
          padding:22px;
          box-shadow: inset 0 1px 0 rgba(255,255,255,0.04);
          transition: transform 0.15s ease, box-shadow 0.15s ease;
          min-width: 0;
          display: flex;
          flex-direction: column;
        }
        .metric::after{
          content:"";
          position:absolute;
          inset:0;
          opacity:0;
          transition:opacity 0.2s ease;
          border-radius:inherit;
        }
        .metric:hover{
          transform: translateY(-2px);
          box-shadow: rgba(15,23,42,0.35) 0 10px 25px;
        }
        .metric:hover::after{ opacity:0.12; }
        .metric-temp{
          background: linear-gradient(145deg, rgba(249,115,22,0.22), rgba(217,70,239,0.18), rgba(15,23,42,0.9));
        }
        .metric-temp::after{
          background: radial-gradient(circle at 100% 0%, rgba(255,161,90,0.6), transparent 55%);
        }
        .metric-humid{
          background: linear-gradient(145deg, rgba(56,189,248,0.22), rgba(14,165,233,0.18), rgba(15,23,42,0.9));
        }
        .metric-humid::after{
          background: radial-gradient(circle at 100% 0%, rgba(125,211,252,0.6), transparent 55%);
        }
        .metric-location{
          background: linear-gradient(145deg, rgba(45,212,191,0.22), rgba(34,197,94,0.18), rgba(15,23,42,0.9));
        }
        .metric-location::after{
          background: radial-gradient(circle at 100% 0%, rgba(52,211,153,0.55), transparent 55%);
        }
        .metric-time{
          background: linear-gradient(145deg, rgba(168,85,247,0.22), rgba(139,92,246,0.18), rgba(15,23,42,0.9));
        }
        .metric-time::after{
          background: radial-gradient(circle at 100% 0%, rgba(196,181,253,0.55), transparent 55%);
        }
        .metric-score{
          background: linear-gradient(145deg, rgba(251,146,60,0.22), rgba(234,88,12,0.18), rgba(15,23,42,0.9));
        }
        .metric-score::after{
          background: radial-gradient(circle at 100% 0%, rgba(253,186,116,0.6), transparent 55%);
        }
        .metric-message{
          background: linear-gradient(145deg, rgba(147,51,234,0.22), rgba(126,34,206,0.18), rgba(15,23,42,0.9));
        }
        .metric-message::after{
          background: radial-gradient(circle at 100% 0%, rgba(196,181,253,0.6), transparent 55%);
        }
        .metric-header{
          display:flex;
          align-items:center;
          gap:10px;
          position:relative;
        }
        .metric .label{
          color:var(--muted);
          font-size:15px;
          letter-spacing:.4px;
        }
        .icon-wrap{
          width:34px;height:34px;
          border-radius:10px;
          display:flex;
          align-items:center;
          justify-content:center;
          background: rgba(255,255,255,0.12);
          box-shadow: inset 0 1px 0 rgba(255,255,255,0.25);
        }
        .icon{
          width:20px;
          height:20px;
        }
        .icon svg{ width:100%; height:100%; stroke-width:1.8; stroke-linecap:round; stroke-linejoin:round; stroke:currentColor; fill:none; }
        .metric-temp .icon{ color:rgba(251,191,36,0.95); }
        .metric-humid .icon{ color:rgba(125,211,252,0.95); }
        .metric-location .icon{ color:rgba(52,211,153,0.95); }
        .metric-time .icon{ color:rgba(196,181,253,0.95); }
        .metric-score .icon{ color:rgba(253,186,116,0.95); }
        .metric-message .icon{ color:rgba(196,181,253,0.95); }
        .metric .value{
          margin-top:8px;
          font-size:26px;
          font-weight:800;
          letter-spacing:.3px;
          text-align:center;
          word-wrap: break-word;
          overflow-wrap: break-word;
          flex: 1;
          display: flex;
          align-items: center;
          justify-content: center;
          min-height: 0;
        }
        .metric-location .value{
          white-space: normal;
          line-height: 1.3;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
        }
        .metric-message .value{
          white-space: normal;
          line-height: 1.3;
        }
        
        /* LED CONTROLS */
        .led-control{
          background: linear-gradient(180deg, rgba(255,255,255,0.04), rgba(255,255,255,0.02));
          border:1px solid rgba(255,255,255,0.08);
          border-radius:14px;
          padding:16px;
          margin-bottom:12px;
        }
        .led-header{
          display:flex;
          align-items:center;
          justify-content:space-between;
          margin-bottom:12px;
        }
        .led-name{ font-weight:700; font-size:16px; }
        .led-status{
          padding:6px 10px;
          border-radius:999px;
          font-size:12px;
          font-weight:700;
          letter-spacing:.4px;
        }
        .led-status.on{ 
          background: rgba(34,197,94,.15); 
          color:#86efac; 
          border:1px solid rgba(34,197,94,.35); 
        }
        .led-status.off{ 
          background: rgba(239,68,68,.12); 
          color:#fca5a5; 
          border:1px solid rgba(239,68,68,.30); 
        }
        .led-buttons{
          display:flex;
          gap:8px;
          margin-bottom:8px;
        }
        .btn-on, .btn-off{
          flex:1;
          appearance:none;
          border:0;
          cursor:pointer;
          font-weight:700;
          padding:10px 14px;
          border-radius:10px;
          transition: all .2s ease;
        }
        .btn-on{
          background: linear-gradient(180deg, rgba(34,197,94,.25), rgba(34,197,94,.15));
          color:#86efac;
          border:1px solid rgba(34,197,94,.35);
        }
        .btn-off{
          background: linear-gradient(180deg, rgba(239,68,68,.25), rgba(239,68,68,.15));
          color:#fca5a5;
          border:1px solid rgba(239,68,68,.35);
        }
        .btn-on:hover{ 
          background: linear-gradient(180deg, rgba(34,197,94,.35), rgba(34,197,94,.25));
          transform: translateY(-1px);
        }
        .btn-off:hover{ 
          background: linear-gradient(180deg, rgba(239,68,68,.35), rgba(239,68,68,.25));
          transform: translateY(-1px);
        }
        
        /* COLOR PICKER */
        .color-section{
          margin-top:12px;
          padding-top:12px;
          border-top:1px solid rgba(255,255,255,0.06);
        }
        .color-label{
          font-size:13px;
          color:var(--muted);
          margin-bottom:8px;
          font-weight:600;
        }
        .color-grid{
          display:grid;
          grid-template-columns: repeat(7, 1fr);
          gap:8px;
          margin-bottom:10px;
        }
        .color-btn{
          width:100%;
          aspect-ratio: 1;
          border-radius:8px;
          border:2px solid rgba(255,255,255,0.15);
          cursor:pointer;
          transition: transform .15s ease, border-color .2s ease, box-shadow .2s ease;
        }
        .color-btn:hover{
          transform: scale(1.15);
          border-color: rgba(255,255,255,0.5);
          box-shadow: 0 4px 12px rgba(0,0,0,0.3);
        }
        .color-btn:active{
          transform: scale(1.05);
        }
        .custom-color{
          display:flex;
          gap:8px;
          align-items:center;
        }
        .custom-color span{
          font-size:12px;
          color:var(--muted);
          font-weight:600;
        }
        .color-input{
          width:50px;
          height:32px;
          border:2px solid rgba(255,255,255,0.15);
          border-radius:8px;
          cursor:pointer;
          background: transparent;
          transition: border-color .2s ease;
        }
        .color-input:hover{
          border-color: rgba(255,255,255,0.35);
        }
        
        .footer{
          margin-top:14px;
          text-align:center;
          color:var(--muted);
          font-size:12px;
          font-weight:600;
          letter-spacing:1px;
        }
        
        @media (max-width: 480px){
          .grid{ grid-template-columns: 1fr; }
        }
      </style>
    </head>
    <body>
      <div class='container'>
        <div class="header">
          <div class="title"><div class="logo"></div> ESP32 Dashboard</div>
          <button class="settings-btn" onclick="window.location='/settings'">&#9881; Settings</button>
        </div>
        
        <!-- SENSOR METRICS -->
        <div class="grid">
          <div class="metric metric-location">
            <div class="metric-header">
              <div class="icon-wrap">
                <div class="icon">
                  <svg viewBox="0 0 24 24">
                    <path d="M12 21s-6-5.4-6-10a6 6 0 1 1 12 0c0 4.6-6 10-6 10z" fill="currentColor" opacity="0.25"/>
                    <path d="M12 21s-6-5.4-6-10a6 6 0 1 1 12 0c0 4.6-6 10-6 10z"/>
                    <circle cx="12" cy="11" r="2.2"/>
                  </svg>
                </div>
              </div>
              <div class="label">Location</div>
            </div>
            <div class="value">
              <div>)rawliteral" + location_line1 + R"rawliteral(</div>
              <div>)rawliteral" + location_line2 + R"rawliteral(</div>
            </div>
          </div>
          <div class="metric metric-time">
            <div class="metric-header">
              <div class="icon-wrap">
                <div class="icon">
                  <svg viewBox="0 0 24 24">
                    <circle cx="12" cy="12" r="10" fill="currentColor" opacity="0.25"/>
                    <circle cx="12" cy="12" r="10"/>
                    <line x1="12" y1="12" x2="12" y2="7"/>
                    <line x1="12" y1="12" x2="16" y2="12"/>
                  </svg>
                </div>
              </div>
              <div class="label">Time</div>
            </div>
            <div class="value" id='time'>--:--:--</div>
          </div>
          <div class="metric metric-temp">
            <div class="metric-header">
              <div class="icon-wrap">
                <div class="icon">
                  <svg viewBox="0 0 24 24">
                    <path d="M10 13.5V5a2 2 0 0 1 4 0v8.5a4 4 0 1 1-4 0z"/>
                    <circle cx="12" cy="18" r="1.8" fill="currentColor" opacity="0.25"/>
                    <circle cx="12" cy="18" r="1.1"/>
                    <line x1="12" y1="10" x2="12" y2="15"/>
                  </svg>
                </div>
              </div>
              <div class="label">Temperature</div>
            </div>
            <div class="value"><span id='temp'>)rawliteral" + String(temperature) + R"rawliteral(</span> &deg;C</div>
          </div>
          <div class="metric metric-humid">
            <div class="metric-header">
              <div class="icon-wrap">
                <div class="icon">
                  <svg viewBox="0 0 24 24">
                    <path d="M12 3c-2.5 3.3-5 6.5-5 9.5a5 5 0 0 0 10 0C17 9.5 14.5 6.3 12 3z" fill="currentColor" opacity="0.25"/>
                    <path d="M12 3c-2.5 3.3-5 6.5-5 9.5a5 5 0 0 0 10 0C17 9.5 14.5 6.3 12 3z"/>
                  </svg>
                </div>
              </div>
              <div class="label">Humidity</div>
            </div>
            <div class="value"><span id='hum'>)rawliteral" + String(humidity) + R"rawliteral(</span> %</div>
          </div>
          <div class="metric metric-score">
            <div class="metric-header">
              <div class="icon-wrap">
                <div class="icon">
                  <svg viewBox="0 0 24 24">
                    <path d="M12 2L2 7l10 5 10-5-10-5z" fill="currentColor" opacity="0.25"/>
                    <path d="M2 17l10 5 10-5M2 12l10 5 10-5"/>
                  </svg>
                </div>
              </div>
              <div class="label">Score</div>
            </div>
            <div class="value"><span id='score'>)rawliteral" + String(score, 4) + R"rawliteral(</span></div>
          </div>
          <div class="metric metric-message">
            <div class="metric-header">
              <div class="icon-wrap">
                <div class="icon">
                  <svg viewBox="0 0 24 24">
                    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z" fill="currentColor" opacity="0.25"/>
                    <path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>
                  </svg>
                </div>
              </div>
              <div class="label">Message</div>
            </div>
            <div class="value" id='message'>)rawliteral" + message + R"rawliteral(</div>
          </div>
        </div>
        
        <!-- LED CONTROLS -->
        <div class="led-control" style="margin-top:14px;">
          <div class="led-header">
            <div class="led-name">💡 LED 1 (Regular)</div>
            <div class="led-status" id="led1-status">)rawliteral" + led1 + R"rawliteral(</div>
          </div>
          <div class="led-buttons">
            <button class="btn-on" onclick="controlLED(1, 'on')">ON</button>
            <button class="btn-off" onclick="controlLED(1, 'off')">OFF</button>
          </div>
        </div>
        
        <div class="led-control">
          <div class="led-header">
            <div class="led-name">🌈 LED 2 (NeoPixel)</div>
            <div class="led-status" id="led2-status">)rawliteral" + led2 + R"rawliteral(</div>
          </div>
          <div class="led-buttons">
            <button class="btn-on" onclick="controlLED(2, 'on', currentColor)">ON</button>
            <button class="btn-off" onclick="controlLED(2, 'off')">OFF</button>
          </div>
          
          <!-- COLOR PICKER -->
          <div class="color-section">
            <div class="color-label">Choose Color:</div>
            <div class="color-grid">
              <div class="color-btn" style="background:#ff0000" onclick="setColor('red')" title="Red"></div>
              <div class="color-btn" style="background:#00ff00" onclick="setColor('green')" title="Green"></div>
              <div class="color-btn" style="background:#0000ff" onclick="setColor('blue')" title="Blue"></div>
              <div class="color-btn" style="background:#ffff00" onclick="setColor('yellow')" title="Yellow"></div>
              <div class="color-btn" style="background:#ff00ff" onclick="setColor('purple')" title="Purple"></div>
              <div class="color-btn" style="background:#00ffff" onclick="setColor('cyan')" title="Cyan"></div>
              <div class="color-btn" style="background:#ffffff" onclick="setColor('white')" title="White"></div>
            </div>
            <div class="custom-color">
              <span>Custom:</span>
              <input type="color" class="color-input" id="customColor" onchange="setCustomColor()" value="#ffffff">
            </div>
          </div>
        </div>
        
        <div class="footer">EMBEDDED SYSTEM - LAB 3</div>
      </div>
      
      <script>
        let currentColor = 'white';
        
        function controlLED(id, action, color) {
          let url = '/toggle?led=' + id + '&action=' + action;
          
          if (color) {
            url += '&color=' + encodeURIComponent(color);
          }
          
          fetch(url)
          .then(response => response.json())
          .then(json => {
            updateStatus('led1-status', json.led1);
            updateStatus('led2-status', json.led2);
          });
        }
        
        function setColor(color) {
          currentColor = color;
          const led2Status = document.getElementById('led2-status').textContent.trim();
          if (led2Status === 'ON') {
            controlLED(2, 'on', color);
          }
        }
        
        function setCustomColor() {
          const colorInput = document.getElementById('customColor');
          currentColor = colorInput.value;
          const led2Status = document.getElementById('led2-status').textContent.trim();
          if (led2Status === 'ON') {
            controlLED(2, 'on', currentColor);
          }
        }
        
        function updateStatus(elementId, state) {
          const el = document.getElementById(elementId);
          const isOn = state.toUpperCase() === 'ON';
          el.textContent = isOn ? 'ON' : 'OFF';
          el.classList.remove('on', 'off');
          el.classList.add(isOn ? 'on' : 'off');
        }
        
        function updateTime() {
          const now = new Date();
          const hours = String(now.getHours()).padStart(2, '0');
          const minutes = String(now.getMinutes()).padStart(2, '0');
          const seconds = String(now.getSeconds()).padStart(2, '0');
          document.getElementById('time').innerText = hours + ':' + minutes + ':' + seconds;
        }
        
        updateStatus('led1-status', ')rawliteral" + led1 + R"rawliteral(');
        updateStatus('led2-status', ')rawliteral" + led2 + R"rawliteral(');
        updateTime();
        setInterval(updateTime, 1000);
        
        setInterval(() => {
          fetch('/sensors')
           .then(res => res.json())
           .then(d => {
             document.getElementById('temp').innerText = d.temp;
             document.getElementById('hum').innerText = d.hum;
             document.getElementById('score').innerText = d.score;
             document.getElementById('message').innerText = d.message;
           });
        }, 5000);
      </script>
    </body></html>
  )rawliteral";
}

String settingsPage() {
  String location = location_label;
  String currentMode = isAPMode ? "AP Mode" : "STA Mode";
  String currentIP = isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  
  return R"rawliteral(
    <!DOCTYPE html><html><head>
      <meta charset='utf-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1.0'>
      <title>Settings</title>
      <style>
        :root{
          --bg:#0f172a;
          --card:#111827;
          --muted:#9ca3af;
          --text:#e5e7eb;
          --primary:#3b82f6;
          --shadow:0 10px 30px rgba(0,0,0,0.35);
          --radius:16px;
        }
        *{box-sizing:border-box}
        body{
          margin:0;
          font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, "Noto Sans", "Apple Color Emoji", "Segoe UI Emoji";
          color:var(--text);
          background:
            radial-gradient(1200px 600px at -10% -10%, rgba(59,130,246,.15), transparent 40%),
            radial-gradient(800px 400px at 120% 10%, rgba(34,211,238,.12), transparent 40%),
            linear-gradient(180deg, #0b1220, #0f172a 30%, #0b1220 100%);
          min-height:100vh;
          display:flex;
          align-items:center;
          justify-content:center;
          padding:24px;
        }
        .container{
          width:100%;
          max-width:520px;
          background: linear-gradient(180deg, rgba(255,255,255,0.04), rgba(255,255,255,0.02)) , var(--card);
          border:1px solid rgba(255,255,255,0.08);
          border-radius: var(--radius);
          box-shadow: var(--shadow);
          padding: 22px;
          backdrop-filter: blur(8px);
        }
        h2{ margin:6px 0 14px; }
        
        .status-banner{
          background: linear-gradient(135deg, rgba(34,197,94,0.15), rgba(16,185,129,0.10));
          border:1px solid rgba(34,197,94,0.3);
          border-radius:12px;
          padding:12px 16px;
          margin-bottom:16px;
          display:flex;
          align-items:center;
          justify-content:space-between;
        }
        .status-info{
          display:flex;
          flex-direction:column;
          gap:4px;
        }
        .status-mode{
          font-weight:700;
          color:#86efac;
          font-size:14px;
        }
        .status-ip{
          font-size:12px;
          color:var(--muted);
          font-family:monospace;
        }
        .status-icon{
          width:24px;
          height:24px;
          color:#86efac;
        }
        
        form{
          display:grid;
          gap:12px;
        }
        .form-label{
          font-size:13px;
          color:var(--muted);
          margin-bottom:6px;
          font-weight:600;
          display:flex;
          align-items:center;
          gap:6px;
        }
        .form-label svg{
          width:14px;
          height:14px;
        }
        .input{
          background: transparent;
          border:0;
          color:var(--text);
          width:100%;
          font-size:15px;
          outline:none;
          padding:0;
        }
        .input::placeholder{
          color:rgba(156,163,175,0.5);
        }
        .field,
        .password-field{
          background: rgba(255,255,255,.04);
          border:1px solid rgba(255,255,255,.10);
          border-radius:12px;
          padding:12px 14px;
          transition: border-color .2s ease, background .2s ease;
        }
        .field:focus-within,
        .password-field:focus-within{
          border-color: rgba(59,130,246,0.5);
          background: rgba(255,255,255,.06);
        }
        .password-field{ display:flex; align-items:center; gap:8px; }
        .input-group{
          display:grid;
          gap:6px;
        }
        .toggle-pass{
          appearance:none;
          border:0;
          background: rgba(255,255,255,0.08);
          color:var(--muted);
          padding:6px 12px;
          border-radius:999px;
          font-size:12px;
          font-weight:600;
          cursor:pointer;
          transition: background .2s ease,color .2s ease;
        }
        .toggle-pass:hover{
          background: rgba(59,130,246,0.25);
          color:#dbeafe;
        }
        .row{
          display:flex;
          gap:10px;
        }
        .primary{
          appearance:none;border:0;cursor:pointer;font-weight:700;
          color:white;
          background: linear-gradient(180deg, rgba(59,130,246,.25), rgba(59,130,246,.15));
          border:1px solid rgba(59,130,246,.45);
          padding:10px 14px;border-radius:12px;
          flex:1;
        }
        .ghost{
          appearance:none;border:1px solid rgba(255,255,255,.15);cursor:pointer;font-weight:700;
          color:var(--text);
          background: transparent;
          padding:10px 14px;border-radius:12px;
          flex:1;
          transition: all .2s ease;
        }
        .primary:hover{
          background: linear-gradient(180deg, rgba(59,130,246,.35), rgba(59,130,246,.25));
          transform: translateY(-1px);
          box-shadow: 0 6px 18px rgba(59,130,246,.3);
        }
        .ghost:hover{
          background: rgba(255,255,255,0.05);
          border-color: rgba(255,255,255,0.25);
        }
        .primary:disabled{
          opacity: 0.5;
          cursor: not-allowed;
          transform: none;
        }
        
        #msg{ 
          margin-top:12px; 
          padding:12px 14px;
          border-radius:10px;
          font-size:13px;
          font-weight:600;
          display:none;
        }
        #msg.info{
          display:block;
          background: rgba(59,130,246,0.15);
          border:1px solid rgba(59,130,246,0.3);
          color:#93c5fd;
        }
        #msg.success{
          display:block;
          background: rgba(34,197,94,0.15);
          border:1px solid rgba(34,197,94,0.3);
          color:#86efac;
        }
        #msg.error{
          display:block;
          background: rgba(239,68,68,0.15);
          border:1px solid rgba(239,68,68,0.3);
          color:#fca5a5;
        }
        
        .spinner{
          display:inline-block;
          width:16px;
          height:16px;
          border:2px solid rgba(255,255,255,0.2);
          border-top-color:#93c5fd;
          border-radius:50%;
          animation:spin 0.8s linear infinite;
          vertical-align:middle;
          margin-right:8px;
        }
        @keyframes spin{
          to{ transform:rotate(360deg); }
        }
        
        .info-box{
          background: rgba(59,130,246,0.08);
          border:1px solid rgba(59,130,246,0.2);
          border-radius:12px;
          padding:12px 14px;
          margin-top:14px;
          font-size:13px;
          color:var(--muted);
          line-height:1.5;
        }
        .info-box strong{
          color:#93c5fd;
          font-weight:600;
        }
        .info-box code{
          background: rgba(255,255,255,0.08);
          padding:2px 6px;
          border-radius:4px;
          font-family:monospace;
          font-size:12px;
          color:#dbeafe;
        }
      </style>
    </head>
    <body>
      <div class='container'>
        <div class="header" style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px;">
          <div class="title" style="display:flex;align-items:center;gap:10px;font-weight:700;">
            <div class="logo" style="width:28px;height:28px;border-radius:8px;background: conic-gradient(from 180deg at 50% 50%, #22d3ee, #3b82f6, #22d3ee);filter: drop-shadow(0 6px 14px rgba(59,130,246,.45));"></div>
            Wi‑Fi Settings
          </div>
        </div>
        
        <div class="status-banner">
          <div class="status-info">
            <div class="status-mode">)rawliteral" + currentMode + R"rawliteral(</div>
            <div class="status-ip">IP: )rawliteral" + currentIP + R"rawliteral(</div>
          </div>
          <svg class="status-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor">
            <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
            <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
            <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
            <line x1="12" y1="20" x2="12.01" y2="20"></line>
          </svg>
        </div>
        
        <form id="wifiForm">
          <div class="input-group">
            <div class="form-label">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
                <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
                <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
                <line x1="12" y1="20" x2="12.01" y2="20"></line>
              </svg>
              WiFi Network
            </div>
            <div class="field">
              <input class="input" name="ssid" id="ssid" placeholder="Enter SSID" required>
            </div>
          </div>
          
          <div class="input-group">
            <div class="form-label">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect>
                <path d="M7 11V7a5 5 0 0 1 10 0v4"></path>
              </svg>
              Password
            </div>
            <div class="password-field">
              <input class="input" name="password" id="pass" type="password" placeholder="Enter password" required>
              <button id="togglePass" class="toggle-pass" type="button" aria-pressed="false">Show</button>
            </div>
          </div>
          
          <div class="input-group">
            <div class="form-label">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"></path>
                <circle cx="12" cy="10" r="3"></circle>
              </svg>
              Location
            </div>
            <div class="field">
              <input class="input" name="location" id="location" placeholder="City, Room, ..." value=")rawliteral" + location + R"rawliteral(" required>
            </div>
          </div>
          
          <div class="row">
            <button class="primary" type="submit" id="submitBtn">Connect</button>
            <button class="ghost" type="button" onclick="window.location='/'">Cancel</button>
          </div>
        </form>
        
        <div id="msg"></div>
        
        <div class="info-box">
          <strong>Note:</strong> After connecting to a new WiFi network, the device will switch to <code>STA Mode</code>. 
          If connection fails, it will automatically return to <code>AP Mode</code>.
        </div>
      </div>
      
      <script>
        const msgBox = document.getElementById('msg');
        const submitBtn = document.getElementById('submitBtn');
        
        document.getElementById('wifiForm').onsubmit = function(e){
          e.preventDefault();
          
          const ssid = document.getElementById('ssid').value;
          const pass = document.getElementById('pass').value;
          const location = document.getElementById('location').value;
          
          submitBtn.disabled = true;
          submitBtn.innerHTML = '<span class="spinner"></span>Connecting...';
          
          msgBox.className = 'info';
          msgBox.innerHTML = '<span class="spinner"></span>Attempting to connect to <strong>' + ssid + '</strong>...';
          
          fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass)+'&loc='+encodeURIComponent(location))
            .then(r => r.text())
            .then(msg => {
              // Bắt đầu polling để check status
              let checkCount = 0;
              const maxChecks = 20; // 20 lần x 500ms = 10 giây
              
              const checkStatus = setInterval(() => {
                checkCount++;
                
                fetch('/status')
                  .then(r => r.json())
                  .then(data => {
                    if (data.status === 'connected') {
                      clearInterval(checkStatus);
                      msgBox.className = 'success';
                      msgBox.innerHTML = '✓ Connected successfully!<br><br>' +
                        '<strong>New IP:</strong> <code>' + data.ip + '</code><br>' +
                        '<strong>mDNS:</strong> <code>http://' + data.mdns + '</code><br><br>' +
                        '<em>Please connect to the new WiFi network: <strong>' + ssid + '</strong></em><br>' +
                        'Then access: <a href="http://' + data.ip + '" style="color:#86efac">http://' + data.ip + '</a> or ' +
                        '<a href="http://' + data.mdns + '" style="color:#86efac">http://' + data.mdns + '</a>';
                      
                      submitBtn.disabled = false;
                      submitBtn.textContent = 'Connect';
                    } else if (data.status === 'failed' || checkCount >= maxChecks) {
                      clearInterval(checkStatus);
                      msgBox.className = 'error';
                      msgBox.innerHTML = '✗ Connection failed. Device returned to AP Mode.<br>' +
                        'IP: <code>' + data.ip + '</code>';
                      submitBtn.disabled = false;
                      submitBtn.textContent = 'Connect';
                    }
                  })
                  .catch(err => {
                    // Nếu không fetch được, có thể đã đổi mạng
                    if (checkCount >= 5) {
                      clearInterval(checkStatus);
                      msgBox.className = 'info';
                      msgBox.innerHTML = '⚠ Connection may be successful but device is now on different network.<br><br>' +
                        'Please connect your device to <strong>' + ssid + '</strong> and access:<br>' +
                        '• Check your router for ESP32 IP address<br>' +
                        '• Or try: <a href="http://esp32.local" style="color:#93c5fd">http://esp32.local</a>';
                      submitBtn.disabled = false;
                      submitBtn.textContent = 'Connect';
                    }
                  });
              }, 500);
            })
            .catch(err => {
              msgBox.className = 'error';
              msgBox.innerHTML = '✗ Connection request failed: ' + err.message;
              submitBtn.disabled = false;
              submitBtn.textContent = 'Connect';
            });
        };
        
        document.getElementById('togglePass').onclick = function(){
          const passInput = document.getElementById('pass');
          const isPassword = passInput.type === 'password';
          passInput.type = isPassword ? 'text' : 'password';
          this.textContent = isPassword ? 'Hide' : 'Show';
          this.setAttribute('aria-pressed', isPassword ? 'true' : 'false');
        };
      </script>
    </body></html>
  )rawliteral";
}

// ========== Handlers ==========
void handleRoot() { 
  server.send(200, "text/html", mainPage()); 
}

void handleToggle() {
  int led = server.arg("led").toInt();
  String action = server.arg("action");
  
  if (led == 1) {
    if (action == "on") {
      led1_state = true;
      digitalWrite(LED1_PIN, HIGH);
    } else {
      led1_state = false;
      digitalWrite(LED1_PIN, LOW);
    }
    web_led1_control_enabled = true;
    Serial.println("LED1 " + String(led1_state ? "ON" : "OFF"));
  }
  else if (led == 2) {
    if (action == "on") {
      led2_state = true;
      
      String color = server.arg("color");
      uint32_t rgbColor;
      
      if (color.startsWith("#")) {
        long hexValue = strtol(color.substring(1).c_str(), NULL, 16);
        uint8_t r = (hexValue >> 16) & 0xFF;
        uint8_t g = (hexValue >> 8) & 0xFF;
        uint8_t b = hexValue & 0xFF;
        rgbColor = neoPixel.Color(r, g, b);
        Serial.println("Custom color: R=" + String(r) + " G=" + String(g) + " B=" + String(b));
      } else {
        if (color == "red") rgbColor = neoPixel.Color(255, 0, 0);
        else if (color == "green") rgbColor = neoPixel.Color(0, 255, 0);
        else if (color == "blue") rgbColor = neoPixel.Color(0, 0, 255);
        else if (color == "yellow") rgbColor = neoPixel.Color(255, 255, 0);
        else if (color == "purple") rgbColor = neoPixel.Color(255, 0, 255);
        else if (color == "cyan") rgbColor = neoPixel.Color(0, 255, 255);
        else if (color == "white") rgbColor = neoPixel.Color(255, 255, 255);
        else rgbColor = neoPixel.Color(255, 255, 255);
        Serial.println("Preset color: " + color);
      }
      
      neoPixel.setPixelColor(0, rgbColor);
      neoPixel.show();
    } else {
      led2_state = false;
      neoPixel.setPixelColor(0, neoPixel.Color(0, 0, 0));
      neoPixel.show();
    }
    web_led2_control_enabled = true;
    Serial.println("LED2 " + String(led2_state ? "ON" : "OFF"));
  }
  
  server.send(200, "application/json",
    "{\"led1\":\"" + String(led1_state ? "ON":"OFF") +
    "\",\"led2\":\"" + String(led2_state ? "ON":"OFF") + "\"}");
}

void handleSensors() {
  float t = glob_temperature;
  float h = glob_humidity;
  float s = glob_anomaly_score;
  String m = glob_anomaly_message;
  String json = "{\"temp\":"+String(t)+",\"hum\":"+String(h)+",\"score\":"+String(s,4)+",\"message\":\""+m+"\"}";
  server.send(200, "application/json", json);
}

void handleSettings() { 
  server.send(200, "text/html", settingsPage()); 
}

void handleConnect() {
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  if (server.hasArg("loc")) {
    location_label = server.arg("loc");
  }
  
  // Gửi response ngay để không bị timeout
  server.send(200, "text/plain", "OK");
  
  // Đợi một chút để response được gửi đi
  delay(100);
  
  isAPMode = false;
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
}

// Thêm handler để check status kết nối
void handleStatus() {
  String status = "connecting";
  String ip = "";
  
  if (WiFi.status() == WL_CONNECTED) {
    status = "connected";
    ip = WiFi.localIP().toString();
  } else if (millis() - connect_start_ms > 10000) {
    status = "failed";
    ip = WiFi.softAPIP().toString();
  }
  
  String json = "{\"status\":\"" + status + "\",\"ip\":\"" + ip + "\",\"mdns\":\"esp32.local\"}";
  server.send(200, "application/json", json);
}

// ========== WiFi ==========
void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/status", HTTP_GET, handleStatus);
  server.begin();
}

void startAP() {
  WiFi.mode(WIFI_AP);
  delay(100);
  bool connected = WiFi.softAP(ssid.c_str(), password.c_str());
  if (connected) {
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Setup mDNS
    if (MDNS.begin("esp32")) {
      Serial.println("mDNS responder started: http://esp32.local");
      MDNS.addService("http", "tcp", 80);
    }
  } else {
    Serial.println("Failed to start AP");
  }
  
  isAPMode = true;
  connecting = false;
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  Serial.print("Connecting to: ");
  Serial.print(wifi_ssid.c_str());
  Serial.print(" Password: ");
  Serial.println(wifi_password.c_str());
}

// ========== Main task ==========
void main_server_task(void *pvParameters){
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  Serial.println("Web Server Task Started");
  
  // Tự động thử STA mode trước
  Serial.println("Trying STA mode first...");
  isAPMode = false;
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
  
  // Đợi 8 giây để thử kết nối
  unsigned long waitStart = millis();
  while (millis() - waitStart < 8000) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("✅ STA Connected! IP: ");
      Serial.println(WiFi.localIP());
      
      // Setup mDNS for STA mode
      if (MDNS.begin("esp32")) {
        Serial.println("mDNS responder started: http://esp32.local");
        MDNS.addService("http", "tcp", 80);
      }
      
      isWifiConnected = true;
      isAPMode = false;
      connecting = false;
      xSemaphoreGive(xBinarySemaphoreInternet);
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  
  // Nếu STA thất bại → chuyển AP
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ STA failed → Starting AP mode");
    startAP();
    isAPMode = true;
    connecting = false;
    isWifiConnected = false;
  }
  
  // Setup server
  setupServer();
  Serial.println("Web server ready!");
  
  while(1){
    server.handleClient();
    
    // Handle WiFi connection logic
    if (connecting) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Connected to WiFi! IP: ");
        Serial.println(WiFi.localIP());
        
        // Setup mDNS
        if (MDNS.begin("esp32")) {
          Serial.println("mDNS responder started: http://esp32.local");
          MDNS.addService("http", "tcp", 80);
        }
        
        isWifiConnected = true;
        xSemaphoreGive(xBinarySemaphoreInternet);
        isAPMode = false;
        connecting = false;
      } else if (millis() - connect_start_ms > 10000) {
        Serial.println("WiFi connection timeout, switching back to AP mode");
        startAP();
        setupServer();
        connecting = false;
        isWifiConnected = false;
      }
    }
    
    // Debug status
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 20000) {
      if (isAPMode) {
        Serial.println("AP Mode - IP: " + WiFi.softAPIP().toString());
      } else {
        Serial.println("STA Mode - IP: " + WiFi.localIP().toString());
      }
      lastDebug = millis();
    }
    
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}