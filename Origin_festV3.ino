#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(3, 2);

const int trigPin = 32;
const int echoPin = 33;
int duration;
int distance;

const int flowMeter1 = 27;
const int flowMeter2 = 26;
const int flowMeter3 = 25;
const int tamperPin1 = 15;
const int tamperPin2 = 23;
const int tamperPin3 = 34;
const int pump_ = 12;
const int valve_ = 14;
const int valve_ = 5;
const int valve_ = 18;

volatile int Pulse1_Count;
volatile int Pulse2_Count;
volatile int Pulse3_Count;
unsigned int Liter_per_hour;
unsigned long Current_Time, Loop_Time;

const char* ssid = "CDED";
const char* password = "CDED2024.";

AsyncWebServer server(80);

// Variables for multiple meters
float flowVolume1 = 50.0;
float quota1 = 20000.0;
bool valveState1 = false;
bool meterTampered1 = false;

float flowVolume2 = 75.0;
float quota2 = 25000.0;
bool valveState2 = false;
bool meterTampered2 = false;

float flowVolume3 = 100.0;
float quota3 = 30000.0;
bool valveState3 = false;
bool meterTampered3 = false;

float waterLevel = 0.0;

const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Flow Meter Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            flex-direction: column;
        }
        .water-level {
            background-color: #e3f2fd;
            padding: 10px 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
            text-align: center;
            width: 200px;
            margin-bottom: 20px;
        }
        .container {
            display: flex;
            justify-content: space-around;
            width: 100%;
            max-width: 1200px;
        }
        .meter {
            background-color: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 15px rgba(0,0,0,0.1);
            width: 300px;
            margin: 10px;
        }
        .section {
            margin-bottom: 20px;
        }
        .bar-container {
            width: 100%;
            background-color: #e0e0e0;
            border-radius: 5px;
            overflow: hidden;
            position: relative;
        }
        .bar {
            height: 25px;
            background-color: #4caf50;
            width: 0;
            text-align: center;
            color: white;
            line-height: 25px;
        }
        .value {
            font-size: 20px;
            margin: 10px 0;
            text-align: center; 
        }
        .label-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .status {
            font-weight: bold;
        }
        .tampered {
            color: red;
        }
        .normal {
            color: green;
        }
    </style>
</head>
<body>
    <div class="water-level">
        <h3>Water Level</h3>
        <p class="value" id="waterLevel">0 cm</p>
    </div>
    <div class="container">
        <div class="meter" id="upstream">
            <h3>Upstream</h3>
            <div class="section">
                <h4>Flow Meter Volume</h4>
                <div class="bar-container">
                    <div id="quotaBar1" class="bar"></div>
                </div>
                <p class="value" id="volume1">0 L</p>
            </div>
            <div class="section">
                <div class="label-row">
                    <h4>Valve State:</h4>
                    <p class="status" id="valveStatus1">OFF</p>
                </div>
            </div>
            <div class="section">
                <div class="label-row">
                    <h4>Meter Status:</h4>
                    <p class="status" id="meterStatus1" class="normal">Normal</p>
                </div>
            </div>
        </div>

        <div class="meter" id="midstream">
            <h3>Midstream</h3>
            <div class="section">
                <h4>Flow Meter Volume</h4>
                <div class="bar-container">
                    <div id="quotaBar2" class="bar"></div>
                </div>
                <p class="value" id="volume2">0 L</p>
            </div>
            <div class="section">
                <div class="label-row">
                    <h4>Valve State:</h4>
                    <p class="status" id="valveStatus2">OFF</p>
                </div>
            </div>
            <div class="section">
                <div class="label-row">
                    <h4>Meter Status:</h4>
                    <p class="status" id="meterStatus2" class="normal">Normal</p>
                </div>
            </div>
        </div>

        <div class="meter" id="downstream">
            <h3>Downstream</h3>
            <div class="section">
                <h4>Flow Meter Volume</h4>
                <div class="bar-container">
                    <div id="quotaBar3" class="bar"></div>
                </div>
                <p class="value" id="volume3">0 L</p>
            </div>
            <div class="section">
                <div class="label-row">
                    <h4>Valve State:</h4>
                    <p class="status" id="valveStatus3">OFF</p>
                </div>
            </div>
            <div class="section">
                <div class="label-row">
                    <h4>Meter Status:</h4>
                    <p class="status" id="meterStatus3" class="normal">Normal</p>
                </div>
            </div>
        </div>
    </div>

    <script>
        async function fetchData() {
            try {
                const response = await fetch('/getFlowData');
                const data = await response.json();

                document.getElementById("waterLevel").textContent = `${data.waterLevel} cm`;

                for (let i = 1; i <= 3; i++) {
                    const volume = data[`flowVolume${i}`];
                    const quota = data[`quota${i}`];
                    const valveState = data[`valveState${i}`];
                    const meterTampered = data[`meterTampered${i}`];

                    document.getElementById(`volume${i}`).textContent = `${volume} L`;

                    let quotaPercent = (volume / quota) * 100;
                    let quotaBar = document.getElementById(`quotaBar${i}`);
                    quotaBar.style.width = `${Math.min(quotaPercent, 100)}%`;
                    quotaBar.textContent = `${Math.min(quotaPercent, 100).toFixed(1)}%`;

                    document.getElementById(`valveStatus${i}`).textContent = valveState ? "ON" : "OFF";

                    let meterStatusElement = document.getElementById(`meterStatus${i}`);
                    if (meterTampered) {
                        meterStatusElement.textContent = "Tampered";
                        meterStatusElement.className = "tampered";
                    } else {
                        meterStatusElement.textContent = "Normal";
                        meterStatusElement.className = "normal";
                    }
                }
            } catch (error) {
                console.error("Error fetching data:", error);
            }
        }

        setInterval(fetchData, 5000);
        fetchData();
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(9600);

    pinMode(pump_, OUTPUT);
    pinMode(valve_, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    pinMode(flowMeter1, INPUT);
    attachInterrupt(flowMeter1, detectFlow1, RISING);
    pinMode(flowMeter2, INPUT);
    attachInterrupt(flowMeter1, detectFlow2, RISING);
    pinMode(flowMeter3, INPUT);
    attachInterrupt(flowMeter1, detectFlow3, RISING);


    pinMode(tamperPin1, INPUT_PULLUP);
    attachInterrupt(tamperPin, detectTamper1, RISING);
    pinMode(tamperPin2, INPUT);
    attachInterrupt(tamperPin, detectTamper2, RISING);
    pinMode(tamperPin3, INPUT);
    attachInterrupt(tamperPin, detectTamper3, RISING);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/getFlowData", HTTP_GET, [](AsyncWebServerRequest *request){
        String jsonResponse = "{\"flowVolume1\":" + String(flowVolume1) +
                              ",\"quota1\":" + String(quota1) +
                              ",\"valveState1\":" + String(valveState1) +
                              ",\"meterTampered1\":" + String(meterTampered1) +
                              ",\"flowVolume2\":" + String(flowVolume2) +
                              ",\"quota2\":" + String(quota2) +
                              ",\"valveState2\":" + String(valveState2) +
                              ",\"meterTampered2\":" + String(meterTampered2) +
                              ",\"flowVolume3\":" + String(flowVolume3) +
                              ",\"quota3\":" + String(quota3) +
                              ",\"valveState3\":" + String(valveState3) +
                              ",\"meterTampered3\":" + String(meterTampered3) +
                              ",\"waterLevel\":" + String(waterLevel) + "}";
        request->send(200, "application/json", jsonResponse);
    });

    server.begin();
}

void loop() {
  measureWaterLevel();
  if (waterLevel > 200 ){// The level of water is measured from the top by an ultrasonic sensor
    pump(1);
  }else{
    pump(0);
  }
  if (meterTampered1 || flowVolume1 >= quota1){
    valve1(0);
  }
  if (meterTampered2 || flowVolume2 >= quota2){
    valve1(0);
  }
  if (meterTampered3 || flowVolume3 >= quota3){
    valve1(0);
  }

  

}

void measureWaterLevel(){
  digitalWrite(trigPin, LOW);
  delay(50);

  digitalWrite(trigPin, HIGH);
  delay(50);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  waterLevel = duration*0.034/2;

  Serial.print("WaterLevel: ");
  Serial.println(waterLevel);
}


void detectFlow1() {
    Pulse1_Count++;
}

void detectFlow2() {
    Pulse2_Count++;
}

void detectFlow3() {
    Pulse3_Count++;
}

void measureVolume1(){
  Current_Time = millis();
   if(Current_Time >= (Loop_Time + 1000))
   {
      Loop_Time = Current_Time;
      Liter_per_hour = (Pulse1_Count * 60 / 7.5);
      Pulse_Count = 0;
   }
    flowVolume1 = 2.663 * Pulse1_Count;
    Serial.print(flowVolume1);
    Serial.println(" mL");
    
}

void measureVolume2(){
  Current_Time = millis();
   if(Current_Time >= (Loop_Time + 1000))
   {
      Loop_Time = Current_Time;
      Liter_per_hour = (Pulse2_Count * 60 / 7.5);
      Pulse2_Count = 0;
   }
    flowVolume2 = 2.663 * Pulse2_Count;
    Serial.print(flowVolume2);
    Serial.println(" mL");
    
}

void measureVolume1(){
  Current_Time = millis();
   if(Current_Time >= (Loop_Time + 1000))
   {
      Loop_Time = Current_Time;
      Liter_per_hour = (Pulse3_Count * 60 / 7.5);
      Pulse3_Count = 0;
   }
    flowVolume3 = 2.663 * Pulse3_Count;
    Serial.print(flowVolume3);
    Serial.println(" mL");
    
}
void detectTamper1() {
    meterTampered1 = true;
    reportTamper(1);
}

void detectTamper2() {
    meterTampered2 = true;
    reportTamper(2);
}

void detectTamper3() {
    meterTampered3 = true;
    reportTamper(3);
}

void pump(int mode){
  if (mode == 1){
    digitalWrite(pump_, HIGH);
  }else if (mode == 0){
    digitalWrite(pump_, LOW);
  }
}
void valve1(int mode){
  if (mode == 1){
    digitalWrite(valve1_, HIGH);
  }else if (mode == 0){
    digitalWrite(valve1_, LOW);
  }
}
void valve2(int mode){
  if (mode == 1){
    digitalWrite(valve3_, HIGH);
  }else if (mode == 0){
    digitalWrite(valve3_, LOW);
  }
}
void valve3(int mode){
  if (mode == 1){
    digitalWrite(valve2_, HIGH);
  }else if (mode == 0){
    digitalWrite(valve2_, LOW);
  }
}

void reportTamper(int device)
{
   int tamp = digitalRead(tamper);
if (tamp==1) {
 
  mySerial.begin(9600);
  Serial.println("Initializing..."); 
  delay(1000);
  mySerial.println("AT"); 
  updateSerial();
   mySerial.println("AT+CMGF=1"); 
  updateSerial();
   mySerial.println("AT+CMGS=\"+254114133946\""); 
  updateSerial();

 mySerial.println("Tamper on device: "); 
  mySerial.println(device); 

  updateSerial();
  
  mySerial.write(26); 
}