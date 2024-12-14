#include <Wire.h>
#include "MAX30105.h"
#include <TFT_eSPI.h>
#include "heartRate.h"

MAX30105 particleSensor;
TFT_eSPI tft = TFT_eSPI();
const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;        // Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

// Graph area and parameters
int graphWidth = 240;     // Width of the graph
int graphHeight = 160;    // Height of the graph
int graphX = 40;          // X position of the graph
int graphY = 80;          // Y position of the graph
int lastY;                // Last Y position to draw a smooth line
int currentX = graphX;   // Current X position for plotting
unsigned long lastUpdate = 0; // Timestamp for the last update
const unsigned long updateInterval = 1000; // Update every second

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize sensor
  Wire.begin(8, 9);
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) { // Use default I2C port, 400kHz speed
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  
  Serial.println("Place your index finger on the sensor with steady pressure.");
  
  particleSensor.setup(); // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); // Turn off Green LED
  
  tft.init();
  tft.setRotation(1);           // Rotate the screen to landscape mode
  tft.fillScreen(TFT_BLACK);    // Fill screen with black color
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set text color and background
  
  // Initialize last point positions
  lastY = graphY + graphHeight / 2; // Start in the middle
  drawGraphAxes();
}

void loop() {
  long irValue = particleSensor.getIR();
  
  if (checkForBeat(irValue) == true) {
    // We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;

      // Ensure BPM is within 50 to 100 range
      if (beatAvg < 50) beatAvg = 50;
      if (beatAvg > 100) beatAvg = 100;

      // Serial output
      Serial.print("BPM = ");
      Serial.println(beatAvg);
      
      // Map heartbeat average to the graph height
      int mappedY = map(beatAvg, 50, 100, graphY + graphHeight, graphY); // Map BPM to Y-axis
      
      // Plot the heartbeat data on the TFT screen
      plotHeartbeatData(mappedY);
    }
  } 

  // Update the graph every second
  if (millis() - lastUpdate >= updateInterval) {
    lastUpdate = millis();
    plotHeartbeatData(lastY); // Keep the lastY for continuous display
  }
}

// Draw the graph axes (X-axis and Y-axis)
void drawGraphAxes() {
  tft.drawLine(graphX, graphY + graphHeight, graphX + graphWidth, graphY + graphHeight, TFT_WHITE); // X-axis
  tft.drawLine(graphX, graphY, graphX, graphY + graphHeight, TFT_WHITE); // Y-axis
  
  // Draw X-axis labels (time)
  for (int i = 0; i <= graphWidth; i += 10) {
    tft.setCursor(graphX + i, graphY + graphHeight + 5);
    tft.print(i / 10); // Print time in increments of 10 seconds
  }

  // Draw Y-axis labels (BPM)
  for (int i = 50; i <= 100; i += 10) {
    tft.setCursor(graphX - 30, graphY + graphHeight - map(i, 50, 100, 0, graphHeight));
    tft.print(i); // Print BPM values from 50 to 100
  }
}

// Plot heartbeat data on the graph
void plotHeartbeatData(int newY) {
  // Draw a line to the new point from the last point
  if (currentX > graphX) { // Only draw if it's not the first point
    tft.drawLine(currentX - 10, lastY, currentX, newY, TFT_GREEN); // Draw line between the last point and the new point
  }
  
  // Update the last point coordinates
  lastY = newY;

  // Increment currentX for the next data point
  currentX += 10; // Increment by 10 for 10-second gap

  // Reset if the graph is full
  if (currentX >= graphX + graphWidth) {
    currentX = graphX; // Reset to the start
    tft.fillRect(graphX, graphY, graphWidth, graphHeight, TFT_BLACK); // Clear the graph area
    drawGraphAxes(); // Redraw axes
  }
}
