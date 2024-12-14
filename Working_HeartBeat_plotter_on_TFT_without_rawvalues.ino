#include <Wire.h>
#include "MAX30105.h"
#include <TFT_eSPI.h>
#include "heartRate.h"

MAX30105 particleSensor;
TFT_eSPI tft = TFT_eSPI();

const byte RATE_SIZE = 4; // Size of [rate] array
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;        // Index for storing heart beat readings
long lastBeat = 0;        // Time at which the last beat occurred

float beatsPerMinute = 0; // Initialize BPM to zero
int beatAvg = 0;          // Initialize average BPM
int lastTwoBeatsAvg = 0;  // Moving average of the last two beats

// Graph area and parameters
int graphWidth = 240;     // Width of the graph
int graphHeight = 160;    // Height of the graph
int graphX = 40;          // X position of the graph
int graphY = 40;          // Y position of the graph
int lastY;                // Last Y position to draw a smooth line
int currentX = graphX;    // Current X position for plotting
unsigned long lastUpdate = 0; // Timestamp for the last update

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
  particleSensor.setPulseAmplitudeGreen(0);  // Turn off Green LED

  tft.init();
  tft.setRotation(1);           // Rotate the screen to landscape mode
  tft.fillScreen(TFT_BLACK);    // Fill screen with black color
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // Set text color and background
  
  // Display "Initializing..." for 10 seconds
  tft.setCursor(50, 120); // Center text on screen
  tft.setTextSize(2);
  tft.print("Initializing...");
  delay(10000);

  // Clear screen and draw graph axes
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

    // Check if BPM is within a realistic range
    if (beatsPerMinute < 255 && beatsPerMinute > 20 && delta > 300) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;

      // Calculate moving average of the last two BPM values
      lastTwoBeatsAvg = (rates[(rateSpot - 1 + RATE_SIZE) % RATE_SIZE] + rates[(rateSpot - 2 + RATE_SIZE) % RATE_SIZE]) / 2;

      // Serial output
      Serial.print("BPM = ");
      Serial.println(beatAvg);
      Serial.print("Moving Avg of Last 2 Beats = ");
      Serial.println(lastTwoBeatsAvg);

      // Map heartbeat average to the graph height
      int mappedY = map(beatAvg, 50, 150, graphY + graphHeight, graphY); // Map BPM to Y-axis

      // Plot the heartbeat data on the TFT screen
      plotHeartbeatData(mappedY);

      // Plot moving average on the graph
      plotMovingAvg(lastTwoBeatsAvg);
    }
  }
}

// Draw the graph axes (X-axis and Y-axis) with labels
void drawGraphAxes() {
  tft.fillScreen(TFT_BLACK); // Clear the entire screen
  
  // Draw axes
  tft.drawLine(graphX, graphY + graphHeight, graphX + graphWidth, graphY + graphHeight, TFT_WHITE); // X-axis
  tft.drawLine(graphX, graphY, graphX, graphY + graphHeight, TFT_WHITE); // Y-axis

  // Draw axis labels
  tft.setTextSize(1);
  tft.setCursor(graphX + graphWidth + 10, graphY + graphHeight - 20);
  tft.print("t (s)"); // Label for time on the X-axis
  tft.setCursor(graphX - 35, graphY - 20);
  tft.print("BPM"); // Label for Y-axis

  // Draw X-axis labels (time)
  for (int i = 1; i <= 10; i++) {
    int xLabelPos = graphX + (i * (graphWidth / 10)); // Calculate position
    tft.setCursor(xLabelPos - 5, graphY + graphHeight + 10); // Adjust for alignment
    tft.print(i); // Print time in seconds
  }

  // Draw Y-axis labels (BPM)
  for (int i = 50; i <= 150; i += 10) {
    int yLabelPos = graphY + graphHeight - map(i, 50, 150, 0, graphHeight);
    tft.setCursor(graphX - 30, yLabelPos - 5);
    tft.print(i); // Print BPM values from 50 to 150
  }
}

// Plot heartbeat data on the graph
void plotHeartbeatData(int newY) {
  // Draw a line to the new point from the last point
  if (currentX > graphX) { // Only draw if it's not the first point
    tft.drawLine(currentX - 10, lastY, currentX, newY, TFT_GREEN); // Draw line between the last point and the new point
  }
  
  // Update the last point coordinates
  lastY = newY; // Update lastY to the current newY without resetting to zero

  // Increment currentX for the next data point
  currentX += 10; // Increment by 10 pixels for each data point

  // Reset if the graph is full
  if (currentX >= graphX + graphWidth) {
    currentX = graphX; // Reset to the start
    drawGraphAxes(); // Redraw axes and clear screen
  }
}

// Plot the moving average of the last two beats on the graph
void plotMovingAvg(int movingAvg) {
  int mappedY = map(movingAvg, 50, 150, graphY + graphHeight, graphY); // Map moving average to Y-axis
  tft.fillCircle(currentX - 5, mappedY, 3, TFT_RED); // Plot the moving average as a red dot
}
