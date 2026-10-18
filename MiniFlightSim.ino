#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


// ==================================================
// JOYSTICK
// ==================================================

#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 2


// ==================================================
// THROTTLE
// ==================================================

#define THROTTLE A2


// ==================================================
// JOYSTICK CENTER
// ==================================================

int centerX;
int centerY;


// ==================================================
// FLIGHT VARIABLES
// ==================================================

float roll = 0;
float pitch = 0;

float throttle = 0;
float speed = 0;

float altitude = 0;

float heading = 0;

float cameraX = 0;
float cameraZ = 0;

float smoothRoll = 0;
float smoothPitch = 0;
float smoothSpeed = 0;


// ==================================================
// TAKEOFF
// ==================================================

bool airborne = false;

float runwayDistance = 0;

float takeoffProgress = 0;


// ==================================================
// SMOOTH LIFTOFF TRANSITION
// ==================================================

// 0 = runway
// 1 = normal flight

float flightTransition = 0;

bool transitioning = false;


// ==================================================
// AIRCRAFT TAKEOFF ANIMATION
// ==================================================

float aircraftPitch = 0;
float aircraftY = 32;


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(9600);

  pinMode(JOY_SW, INPUT_PULLUP);


  // OLED
  if (!display.begin(0x3C, true)) {

    while (1);
  }


  delay(500);


  // Center joystick
  centerX = analogRead(JOY_X);
  centerY = analogRead(JOY_Y);


  // Startup screen
  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);

  display.setTextSize(2);

  display.setCursor(5, 10);
  display.println("TINY");

  display.setCursor(5, 35);
  display.println("FLIGHT");

  display.display();

  delay(1200);
}


// ==================================================
// MAIN LOOP
// ==================================================

void loop() {


  // =================================================
  // READ JOYSTICK
  // =================================================

  int x = analogRead(JOY_X);
  int y = analogRead(JOY_Y);


  int xOffset = x - centerX;
  int yOffset = y - centerY;


  // Deadzone
  if (abs(xOffset) < 40) {
    xOffset = 0;
  }

  if (abs(yOffset) < 40) {
    yOffset = 0;
  }


  // =================================================
  // SWITCHED JOYSTICK AXES
  // =================================================

  // X = PITCH
  pitch =
    -(xOffset / 512.0) * 25.0;


  // Y = ROLL
  roll =
    (yOffset / 512.0) * 45.0;


  pitch =
    constrain(
      pitch,
      -25,
      25
    );


  roll =
    constrain(
      roll,
      -45,
      45
    );


  // =================================================
  // THROTTLE
  // =================================================

  int throttleRaw =
    analogRead(THROTTLE);


  throttle =
    map(
      throttleRaw,
      0,
      1023,
      0,
      100
    );


  throttle =
    constrain(
      throttle,
      0,
      100
    );


  // =================================================
  // SPEED
  // =================================================

  float targetSpeed =
    throttle * 1.5;


  speed +=
    (targetSpeed - speed)
    * 0.035;


  // =================================================
  // SMOOTH ATTITUDE
  // =================================================

  smoothRoll +=
    (roll - smoothRoll)
    * 0.12;


  smoothPitch +=
    (pitch - smoothPitch)
    * 0.12;


  smoothSpeed +=
    (speed - smoothSpeed)
    * 0.08;


  // =================================================
  // TAKEOFF / FLIGHT
  // =================================================

  if (!airborne) {

    updateTakeoff();

  } else {

    updateFlight();
  }


  // =================================================
  // TRANSITION
  // =================================================

  updateFlightTransition();


  // =================================================
  // AIRCRAFT ANIMATION
  // =================================================

  updateAircraftAnimation();


  // =================================================
  // DRAW
  // =================================================

  drawFlightDisplay();


  delay(20);
}


// ==================================================
// TAKEOFF LOGIC
// ==================================================

void updateTakeoff() {

  // Keep aircraft on ground
  altitude = 0;


  // Move down runway
  runwayDistance +=
    smoothSpeed * 0.025;


  // =================================================
  // LIFTOFF CONDITION
  // =================================================

  if (
    speed > 65 &&
    pitch > 8
  ) {

    airborne = true;

    transitioning = true;

    takeoffProgress = 0;

    flightTransition = 0;

    // Keep camera continuous
    cameraZ = runwayDistance;

    altitude = 2;
  }
}


// ==================================================
// SMOOTH FLIGHT TRANSITION
// ==================================================

void updateFlightTransition() {

  if (!transitioning)
    return;


  // Smooth transition
  flightTransition += 0.025;


  if (flightTransition >= 1.0) {

    flightTransition = 1.0;

    transitioning = false;
  }
}


// ==================================================
// AIRCRAFT ANIMATION
// ==================================================

void updateAircraftAnimation() {

  // =================================================
  // NORMAL FLIGHT
  // =================================================

  if (!transitioning) {

    // Return smoothly to normal attitude
    aircraftPitch +=
      (0 - aircraftPitch)
      * 0.10;


    aircraftY +=
      (32 - aircraftY)
      * 0.10;


    return;
  }


  // =================================================
  // TAKEOFF PROGRESS
  // =================================================

  float t =
    flightTransition;


  // -------------------------------------------------
  // PHASE 1: ROTATION
  // -------------------------------------------------

  if (t < 0.35) {

    float p =
      t / 0.35;


    // Smoothstep
    p =
      p * p *
      (3.0 - 2.0 * p);


    // Nose rises to about 16 degrees
    aircraftPitch =
      p * 16.0;


    // Slight movement upward
    aircraftY =
      32.0
      - p * 1.5;
  }


  // -------------------------------------------------
  // PHASE 2: CLIMB
  // -------------------------------------------------

  else if (t < 0.65) {

    float p =
      (t - 0.35)
      / 0.30;


    p =
      p * p *
      (3.0 - 2.0 * p);


    // Hold nose-up attitude
    aircraftPitch =
      16.0
      + p * 3.0;


    // Rise slightly more
    aircraftY =
      30.5
      - p * 1.5;
  }


  // -------------------------------------------------
  // PHASE 3: SETTLE INTO FLIGHT
  // -------------------------------------------------

  else {

    float p =
      (t - 0.65)
      / 0.35;


    p =
      p * p *
      (3.0 - 2.0 * p);


    // Smoothly return to normal
    aircraftPitch =
      19.0
      * (1.0 - p);


    aircraftY =
      29.0
      + p * 3.0;
  }
}


// ==================================================
// FLIGHT PHYSICS
// ==================================================

void updateFlight() {


  // =================================================
  // TURNING
  // =================================================

  heading +=
    smoothRoll * 0.018;


  if (heading >= 360)
    heading -= 360;


  if (heading < 0)
    heading += 360;


  // =================================================
  // WORLD MOVEMENT
  // =================================================

  float headingRad =
    heading * PI / 180.0;


  cameraX +=
    sin(headingRad)
    * smoothSpeed
    * 0.018;


  cameraZ +=
    cos(headingRad)
    * smoothSpeed
    * 0.018;


  // Small sideways effect
  cameraX +=
    smoothRoll * 0.002;


  // =================================================
  // ALTITUDE
  // =================================================

  float climbRate =
    sin(
      pitch * PI / 180.0
    )
    * speed
    * 0.025;


  altitude +=
    climbRate;


  // =================================================
  // GRAVITY / DESCENT
  // =================================================

  if (pitch < -3) {

    altitude -=
      abs(pitch)
      * 0.008;
  }


  // Prevent negative altitude
  if (altitude < 0) {

    altitude = 0;

    airborne = false;

    transitioning = false;

    flightTransition = 0;

    aircraftPitch = 0;

    aircraftY = 32;
  }
}


// ==================================================
// MAIN DISPLAY
// ==================================================

void drawFlightDisplay() {

  display.clearDisplay();


  // =================================================
  // TAKEOFF
  // =================================================

  if (!airborne) {

    drawTakeoff();
  }


  // =================================================
  // TRANSITION
  // =================================================

  else if (transitioning) {

    drawTransition();
  }


  // =================================================
  // NORMAL FLIGHT
  // =================================================

  else {

    drawTerrain();
  }


  // Aircraft always drawn last
  // so it stays visible above terrain.
  drawAircraft();


  drawHUD();


  display.display();
}


// ==================================================
// SMOOTH TAKEOFF TRANSITION
// ==================================================

void drawTransition() {

  float t =
    flightTransition;


  // =================================================
  // RUNWAY
  // =================================================

  if (t < 0.55) {

    float runwayAmount =
      1.0 - (t / 0.55);


    drawTakeoffTransition(
      runwayAmount
    );
  }


  // =================================================
  // TERRAIN
  // =================================================

  if (t > 0.25) {

    float terrainAmount =
      (t - 0.25) / 0.75;


    terrainAmount =
      constrain(
        terrainAmount,
        0,
        1
      );


    drawTransitionTerrain(
      terrainAmount
    );
  }
}


// ==================================================
// TAKEOFF TRANSITION RENDERER
// ==================================================

void drawTakeoffTransition(
  float amount
) {

  float r =
    smoothRoll * PI / 180.0;


  float horizon =
    30.0
    + smoothPitch * 0.45
    - flightTransition * 5.0;


  float slope =
    tan(r);


  // =================================================
  // HORIZON
  // =================================================

  int leftH =
    horizon
    - 64 * slope;


  int rightH =
    horizon
    + 64 * slope;


  display.drawLine(
    0,
    leftH,
    127,
    rightH,
    SH110X_WHITE
  );


  // =================================================
  // RUNWAY EDGES
  // =================================================

  int leftEdge =
    10
    + (1.0 - amount) * 18;


  int rightEdge =
    118
    - (1.0 - amount) * 18;


  display.drawLine(
    64,
    horizon,
    leftEdge,
    64,
    SH110X_WHITE
  );


  display.drawLine(
    64,
    horizon,
    rightEdge,
    64,
    SH110X_WHITE
  );


  // =================================================
  // CENTER MARKINGS
  // =================================================

  float runwayMotion =
    fmod(
      runwayDistance * 0.8,
      18.0
    );


  for (
    int i = 0;
    i < 8;
    i++
  ) {

    float depth =
      i * 0.13
      + 0.10;


    depth -=
      runwayMotion * 0.004;


    if (depth < 0.04)
      depth = 0.04;


    float depthScale =
      0.25
      + amount * 0.75;


    float y =
      horizon
      + pow(depth, 0.65)
      * 42
      * depthScale;


    float width =
      (
        1.5
        + depth * 8
      )
      * amount;


    if (width > 0.5) {

      display.drawLine(
        64 - width,
        y,
        64 + width,
        y,
        SH110X_WHITE
      );
    }
  }


  // =================================================
  // SIDE MARKERS
  // =================================================

  for (
    int i = 0;
    i < 7;
    i++
  ) {

    float depth =
      i * 0.15
      + 0.08;


    float y =
      horizon
      + pow(depth, 0.65)
      * 45;


    float width =
      depth * 45;


    int lx =
      64 - width;


    int rx =
      64 + width;


    lx =
      64
      + (lx - 64)
      * amount;


    rx =
      64
      + (rx - 64)
      * amount;


    if (amount > 0.08) {

      display.drawLine(
        lx - 2,
        y,
        lx + 2,
        y,
        SH110X_WHITE
      );


      display.drawLine(
        rx - 2,
        y,
        rx + 2,
        y,
        SH110X_WHITE
      );
    }
  }


  // =================================================
  // DISTANT MOUNTAINS
  // =================================================

  drawTakeoffMountains(
    horizon,
    slope
  );
}


// ==================================================
// TRANSITION TERRAIN
// ==================================================

void drawTransitionTerrain(
  float amount
) {

  float r =
    smoothRoll * PI / 180.0;


  float normalHorizon =
    32.0
    + smoothPitch * 0.72;


  float takeoffHorizon =
    30.0
    + smoothPitch * 0.45
    - 5.0;


  float horizon =
    takeoffHorizon
    + (
      normalHorizon
      - takeoffHorizon
    )
    * amount;


  float slope =
    tan(r);


  const float CX = 64.0;


  // =================================================
  // HORIZON
  // =================================================

  int horizonL =
    horizon
    + (0 - CX)
    * slope;


  int horizonR =
    horizon
    + (127 - CX)
    * slope;


  display.drawLine(
    0,
    horizonL,
    127,
    horizonR,
    SH110X_WHITE
  );


  // =================================================
  // MOUNTAINS
  // =================================================

  if (amount > 0.08) {

    drawMountainRangeTransition(
      horizon,
      slope,
      0,
      0,
      amount
    );
  }


  if (amount > 0.30) {

    drawMountainRangeTransition(
      horizon + 3,
      slope,
      1,
      18,
      amount
    );
  }


  // =================================================
  // GROUND
  // =================================================

  if (amount > 0.35) {

    for (
      int i = 0;
      i < 10;
      i++
    ) {

      float depth =
        0.12
        + i * 0.11;


      float motion =
        fmod(
          cameraZ * 0.06,
          0.11
        );


      depth -= motion;


      if (depth <= 0.04)
        depth = 0.04;


      float y =
        horizon
        + pow(depth, 0.72)
        * 75;


      if (
        y < horizon ||
        y > 68
      )
        continue;


      int leftY =
        y
        - 40 * slope;


      int rightY =
        y
        + 40 * slope;


      display.drawLine(
        0,
        leftY,
        127,
        rightY,
        SH110X_WHITE
      );
    }
  }


  // =================================================
  // GROUND RAYS
  // =================================================

  if (amount > 0.45) {

    for (
      int i = -10;
      i <= 10;
      i++
    ) {

      if (i == 0)
        continue;


      float worldX =
        i * 14.0;


      worldX -=
        fmod(
          cameraX * 0.35,
          14.0
        );


      float farX =
        64
        + worldX * 0.18;


      float nearX =
        64
        + worldX * 2.5;


      float farY =
        horizon
        + (farX - 64)
        * slope;


      float nearY =
        64
        + (nearX - 64)
        * slope;


      float currentNearX =
        farX
        + (
          nearX - farX
        )
        * amount;


      float currentNearY =
        farY
        + (
          nearY - farY
        )
        * amount;


      display.drawLine(
        farX,
        farY,
        currentNearX,
        currentNearY,
        SH110X_WHITE
      );
    }
  }


  // =================================================
  // TERRAIN DETAILS
  // =================================================

  if (amount > 0.60) {

    drawTerrainDetails(
      horizon,
      slope
    );
  }
}


// ==================================================
// TRANSITION MOUNTAIN RANGE
// ==================================================

void drawMountainRangeTransition(
  float horizon,
  float slope,
  int layer,
  float offset,
  float amount
) {

  int baseY =
    horizon
    + 9
    + layer * 5;


  int lastX = 0;


  int lastY =
    baseY -
    mountainHeight(
      0,
      layer,
      offset
    );


  for (
    int x = 4;
    x <= 132;
    x += 4
  ) {

    float world =
      x * 0.10
      + cameraX * 0.008
      + offset;


    int h =
      mountainHeight(
        world,
        layer,
        offset
      );


    h *= amount;


    int y =
      baseY - h;


    float rolledY =
      y
      + (x - 64)
      * slope;


    float oldRolledY =
      lastY
      + (lastX - 64)
      * slope;


    display.drawLine(
      lastX,
      oldRolledY,
      x,
      rolledY,
      SH110X_WHITE
    );


    lastX = x;
    lastY = y;
  }


  // =================================================
  // RIDGE DETAILS
  // =================================================

  if (
    layer == 0 &&
    amount > 0.65
  ) {

    for (
      int x = 10;
      x < 125;
      x += 22
    ) {

      float world =
        x * 0.10
        + cameraX * 0.008;


      int h =
        mountainHeight(
          world,
          layer,
          0
        );


      h *= amount;


      int peakY =
        baseY - h;


      if (h > 8) {

        int peakX =
          x;


        int p1x =
          peakX - 3;


        int p2x =
          peakX + 4;


        int p1y =
          peakY + 5
          + (p1x - 64)
          * slope;


        int p2y =
          peakY + 5
          + (p2x - 64)
          * slope;


        int py =
          peakY
          + (peakX - 64)
          * slope;


        display.drawLine(
          peakX,
          py,
          p1x,
          p1y,
          SH110X_WHITE
        );


        display.drawLine(
          peakX,
          py,
          p2x,
          p2y,
          SH110X_WHITE
        );
      }
    }
  }
}


// ==================================================
// TAKEOFF RUNWAY
// ==================================================

void drawTakeoff() {

  float r =
    smoothRoll * PI / 180.0;


  float horizon =
    30.0
    + smoothPitch * 0.45;


  float slope =
    tan(r);


  // =================================================
  // HORIZON
  // =================================================

  int leftH =
    horizon
    - 64 * slope;


  int rightH =
    horizon
    + 64 * slope;


  display.drawLine(
    0,
    leftH,
    127,
    rightH,
    SH110X_WHITE
  );


  // =================================================
  // RUNWAY
  // =================================================

  float runwayMotion =
    fmod(
      runwayDistance * 0.8,
      18.0
    );


  int leftEdge = 10;
  int rightEdge = 118;


  display.drawLine(
    64,
    horizon,
    leftEdge,
    64,
    SH110X_WHITE
  );


  display.drawLine(
    64,
    horizon,
    rightEdge,
    64,
    SH110X_WHITE
  );


  // =================================================
  // CENTER MARKINGS
  // =================================================

  for (
    int i = 0;
    i < 8;
    i++
  ) {

    float depth =
      i * 0.13
      + 0.10;


    depth -=
      runwayMotion * 0.004;


    if (depth < 0.04)
      depth = 0.04;


    float y =
      horizon
      + pow(depth, 0.65)
      * 42;


    float width =
      1.5
      + depth * 8;


    display.drawLine(
      64 - width,
      y,
      64 + width,
      y,
      SH110X_WHITE
    );
  }


  // =================================================
  // SIDE MARKERS
  // =================================================

  for (
    int i = 0;
    i < 7;
    i++
  ) {

    float depth =
      i * 0.15
      + 0.08;


    float y =
      horizon
      + pow(depth, 0.65)
      * 45;


    float width =
      depth * 45;


    int lx =
      64 - width;


    int rx =
      64 + width;


    display.drawLine(
      lx - 2,
      y,
      lx + 2,
      y,
      SH110X_WHITE
    );


    display.drawLine(
      rx - 2,
      y,
      rx + 2,
      y,
      SH110X_WHITE
    );
  }


  // =================================================
  // MOUNTAINS
  // =================================================

  drawTakeoffMountains(
    horizon,
    slope
  );


  // =================================================
  // MESSAGE
  // =================================================

  display.setTextSize(1);

  display.setCursor(
    42,
    8
  );

  display.print("TAKEOFF");


  display.setCursor(
    4,
    48
  );


  if (speed < 65) {

    display.print("ACCELERATE");

  } else if (pitch <= 8) {

    display.print("PULL UP");

  } else {

    display.print("LIFTOFF!");
  }
}


// ==================================================
// TAKEOFF MOUNTAINS
// ==================================================

void drawTakeoffMountains(
  float horizon,
  float slope
) {

  int base =
    horizon + 5;


  int lastX = 0;

  int lastY =
    base -
    mountainHeight(
      0,
      1,
      0
    );


  for (
    int x = 4;
    x <= 132;
    x += 4
  ) {

    float world =
      x * 0.10;


    int h =
      mountainHeight(
        world,
        1,
        0
      );


    int y =
      base - h;


    float rotatedY =
      y
      + (x - 64)
      * slope;


    float oldRotatedY =
      lastY
      + (lastX - 64)
      * slope;


    display.drawLine(
      lastX,
      oldRotatedY,
      x,
      rotatedY,
      SH110X_WHITE
    );


    lastX = x;
    lastY = y;
  }
}


// ==================================================
// FLIGHT TERRAIN
// ==================================================

void drawTerrain() {

  float r =
    smoothRoll * PI / 180.0;


  float horizon =
    32.0
    + smoothPitch * 0.72;


  float slope =
    tan(r);


  const float CX = 64.0;


  // =================================================
  // HORIZON
  // =================================================

  int horizonL =
    horizon
    + (0 - CX)
    * slope;


  int horizonR =
    horizon
    + (127 - CX)
    * slope;


  display.drawLine(
    0,
    horizonL,
    127,
    horizonR,
    SH110X_WHITE
  );


  // =================================================
  // MOUNTAINS
  // =================================================

  drawMountainRange(
    horizon,
    slope,
    0,
    0
  );


  drawMountainRange(
    horizon + 3,
    slope,
    1,
    18
  );


  // =================================================
  // PERSPECTIVE GROUND
  // =================================================

  for (
    int i = 0;
    i < 10;
    i++
  ) {

    float depth =
      0.12
      + i * 0.11;


    float motion =
      fmod(
        cameraZ * 0.06,
        0.11
      );


    depth -= motion;


    if (depth <= 0.04)
      depth = 0.04;


    float y =
      horizon
      + pow(depth, 0.72)
      * 75;


    if (
      y < horizon ||
      y > 68
    )
      continue;


    int leftY =
      y
      - 40 * slope;


    int rightY =
      y
      + 40 * slope;


    display.drawLine(
      0,
      leftY,
      127,
      rightY,
      SH110X_WHITE
    );
  }


  // =================================================
  // GROUND RAYS
  // =================================================

  for (
    int i = -10;
    i <= 10;
    i++
  ) {

    if (i == 0)
      continue;


    float worldX =
      i * 14.0;


    worldX -=
      fmod(
        cameraX * 0.35,
        14.0
      );


    float farX =
      64
      + worldX * 0.18;


    float nearX =
      64
      + worldX * 2.5;


    float farY =
      horizon
      + (farX - 64)
      * slope;


    float nearY =
      64
      + (nearX - 64)
      * slope;


    display.drawLine(
      farX,
      farY,
      nearX,
      nearY,
      SH110X_WHITE
    );
  }


  // =================================================
  // TERRAIN DETAILS
  // =================================================

  drawTerrainDetails(
    horizon,
    slope
  );
}


// ==================================================
// MOUNTAIN RANGE
// ==================================================

void drawMountainRange(
  float horizon,
  float slope,
  int layer,
  float offset
) {

  int baseY =
    horizon
    + 9
    + layer * 5;


  int lastX = 0;


  int lastY =
    baseY -
    mountainHeight(
      0,
      layer,
      offset
    );


  for (
    int x = 4;
    x <= 132;
    x += 4
  ) {

    float world =
      x * 0.10
      + cameraX * 0.008
      + offset;


    int h =
      mountainHeight(
        world,
        layer,
        offset
      );


    int y =
      baseY - h;


    float rolledY =
      y
      + (x - 64)
      * slope;


    float oldRolledY =
      lastY
      + (lastX - 64)
      * slope;


    display.drawLine(
      lastX,
      oldRolledY,
      x,
      rolledY,
      SH110X_WHITE
    );


    lastX = x;
    lastY = y;
  }


  // =================================================
  // MOUNTAIN SNOW / RIDGE DETAIL
  // =================================================

  if (layer == 0) {

    for (
      int x = 10;
      x < 125;
      x += 22
    ) {

      float world =
        x * 0.10
        + cameraX * 0.008;


      int h =
        mountainHeight(
          world,
          layer,
          0
        );


      int peakY =
        baseY - h;


      if (h > 8) {

        int peakX =
          x;


        int p1x =
          peakX - 3;


        int p2x =
          peakX + 4;


        int p1y =
          peakY + 5
          + (p1x - 64)
          * slope;


        int p2y =
          peakY + 5
          + (p2x - 64)
          * slope;


        int py =
          peakY
          + (peakX - 64)
          * slope;


        display.drawLine(
          peakX,
          py,
          p1x,
          p1y,
          SH110X_WHITE
        );


        display.drawLine(
          peakX,
          py,
          p2x,
          p2y,
          SH110X_WHITE
        );
      }
    }
  }
}


// ==================================================
// MOUNTAIN HEIGHT
// ==================================================

int mountainHeight(
  float x,
  int layer,
  float offset
) {

  float a =
    sin(
      x * 1.3
    );


  float b =
    sin(
      x * 2.7 + 1.4
    );


  float c =
    sin(
      x * 0.55 + 2.0
    );


  float height =
    4
    + abs(a) * 7
    + abs(b) * 4
    + abs(c) * 6;


  if (layer == 1) {

    height *= 0.65;
  }


  return (int)height;
}


// ==================================================
// TERRAIN DETAILS
// ==================================================

void drawTerrainDetails(
  float horizon,
  float slope
) {

  for (
    int i = -7;
    i <= 7;
    i++
  ) {

    float worldX =
      i * 20.0
      - fmod(
          cameraX * 0.25,
          20.0
        );


    int x =
      64 + worldX;


    if (
      x < 2 ||
      x > 125
    )
      continue;


    int y =
      horizon
      + 22
      + abs(i) * 3;


    // Apply roll
    y +=
      (x - 64)
      * slope;


    int size =
      2
      + (abs(i) % 3);


    int leftX =
      x - size;


    int rightX =
      x + size;


    int peakX =
      x;


    int leftY =
      y;


    int rightY =
      y;


    int peakY =
      y - size;


    // Roll peak too
    peakY +=
      (peakX - 64)
      * slope;


    display.drawLine(
      leftX,
      leftY,
      peakX,
      peakY,
      SH110X_WHITE
    );


    display.drawLine(
      peakX,
      peakY,
      rightX,
      rightY,
      SH110X_WHITE
    );
  }
}


// ==================================================
// AIRCRAFT
// ==================================================

void drawAircraft() {

  // =================================================
  // AIRCRAFT POSITION
  // =================================================

  float cx = 64.0;
  float cy = aircraftY;


  // Aircraft pitch angle
  float angle =
    aircraftPitch
    * PI / 180.0;


  float c =
    cos(angle);


  float s =
    sin(angle);


  // =================================================
  // HELPER FUNCTION
  // =================================================
  //
  // Rotate a point around the aircraft center.
  //
  // x' = x*c - y*s
  // y' = x*s + y*c
  //
  // =================================================


  // =================================================
  // MAIN WINGS
  // =================================================

  drawAircraftLine(
    -19, 0,
    -6, 0,
    cx, cy,
    c, s
  );


  drawAircraftLine(
    6, 0,
    19, 0,
    cx, cy,
    c, s
  );


  // =================================================
  // NOSE
  // =================================================

  drawAircraftLine(
    -6, 0,
    0, 4,
    cx, cy,
    c, s
  );


  drawAircraftLine(
    6, 0,
    0, 4,
    cx, cy,
    c, s
  );


  // =================================================
  // CENTER / FUSELAGE
  // =================================================

  drawAircraftLine(
    0, -5,
    0, 5,
    cx, cy,
    c, s
  );


  // =================================================
  // WING TIPS
  // =================================================

  drawAircraftPoint(
    -20, 0,
    cx, cy,
    c, s
  );


  drawAircraftPoint(
    20, 0,
    cx, cy,
    c, s
  );
}


// ==================================================
// AIRCRAFT LINE ROTATION
// ==================================================

void drawAircraftLine(
  float x1,
  float y1,
  float x2,
  float y2,
  float cx,
  float cy,
  float c,
  float s
) {

  float rx1 =
    x1 * c
    - y1 * s;


  float ry1 =
    x1 * s
    + y1 * c;


  float rx2 =
    x2 * c
    - y2 * s;


  float ry2 =
    x2 * s
    + y2 * c;


  display.drawLine(
    (int)(cx + rx1),
    (int)(cy + ry1),
    (int)(cx + rx2),
    (int)(cy + ry2),
    SH110X_WHITE
  );
}


// ==================================================
// AIRCRAFT POINT ROTATION
// ==================================================

void drawAircraftPoint(
  float x,
  float y,
  float cx,
  float cy,
  float c,
  float s
) {

  float rx =
    x * c
    - y * s;


  float ry =
    x * s
    + y * c;


  display.drawPixel(
    (int)(cx + rx),
    (int)(cy + ry),
    SH110X_WHITE
  );
}


// ==================================================
// HUD
// ==================================================

void drawHUD() {

  display.setTextSize(1);

  display.setTextColor(
    SH110X_WHITE
  );


  // =================================================
  // SPEED
  // =================================================

  display.setCursor(
    0,
    0
  );

  display.print("SPD ");

  display.print(
    (int)speed
  );


  // =================================================
  // ALTITUDE
  // =================================================

  display.setCursor(
    78,
    0
  );

  display.print("ALT ");

  display.print(
    (int)altitude
  );


  // =================================================
  // THROTTLE
  // =================================================

  display.setCursor(
    0,
    56
  );

  display.print("THR ");

  display.print(
    (int)throttle
  );

  display.print("%");


  // =================================================
  // PITCH / ROLL
  // =================================================

  display.setCursor(
    72,
    56
  );

  display.print("P");

  display.print(
    (int)pitch
  );

  display.print(" R");

  display.print(
    (int)roll
  );
}
