#include <Arduino.h>
#include <math.h>

// ---------- Pins & mains ----------
const uint8_t PIN_XOR = 9;                 // XOR pulse input
const float   GRID_FREQ = 50.0f;           // set 60.0f if needed
const unsigned long PERIOD_US = (unsigned long)(1000000.0f / GRID_FREQ);

// ---------- Filters & guards ----------
const int   SAMPLES_PER_UPDATE = 9;        // pulses read per print
const float EMA_ALPHA          = 0.25f;    // 0..1 (lower = smoother)
const float OUTLIER_GATE_DEG   = 15.0f;    // reject sample if it jumps > this from EMA
const float MIN_PHASE_DEG      = 1.5f;     // ignore tiny pulses (< ~1.5°)
const float MAX_PHASE_DEG      = 178.0f;   // ignore near-180° glitches
const unsigned long MIN_HIGH_US = 120UL;   // reject high time <120us (~2.16° at 50Hz)

// ---------- State ----------
float ema_phase_deg = NAN;
float ema_pf        = NAN;
uint32_t timeout_ct = 0, glitch_ct = 0;

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.01745329251994329577f
#endif

static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Convert XOR pulse width to |phase| in degrees (wrapped to [0..180])
float widthToPhaseDeg(unsigned long pw_us) {
  float phase = 360.0f * (float)pw_us / (float)PERIOD_US;
  if (phase > 180.0f) phase = 360.0f - phase;  // XOR encodes absolute phase
  if (phase < 0.0f)   phase = -phase;
  return phase;
}

void setup() {
  pinMode(PIN_XOR, INPUT);
  Serial.begin(115200);
  delay(150);
  Serial.println(F("XOR PF/Phase (noise-filtered)"));
  Serial.print (F("GRID_FREQ=")); Serial.print(GRID_FREQ); Serial.println(F(" Hz"));
}

void loop() {
  float buf[SAMPLES_PER_UPDATE];
  int m = 0;

  // 1) Gather multiple pulses
  for (int i = 0; i < SAMPLES_PER_UPDATE; i++) {
    unsigned long pw = pulseIn(PIN_XOR, HIGH, PERIOD_US * 2UL);  // up to 2 cycles
    if (pw == 0) { timeout_ct++; continue; }

    // Deglitch by width
    if (pw < MIN_HIGH_US || pw > (PERIOD_US - MIN_HIGH_US)) continue;

    float phase = widthToPhaseDeg(pw);

    // Bound check in degrees
    if (phase < MIN_PHASE_DEG || phase > MAX_PHASE_DEG) continue;

    // Outlier gate vs current EMA (if initialized)
    if (!isnan(ema_phase_deg) && fabs(phase - ema_phase_deg) > OUTLIER_GATE_DEG) {
      glitch_ct++;
      continue;
    }

    buf[m++] = phase;
  }

  if (m == 0) {
    Serial.print(F("[NO/GLITCH] timeouts=")); Serial.print(timeout_ct);
    Serial.print(F("  glitches="));           Serial.println(glitch_ct);
    delay(180);
    return;
  }

  // 2) Robust average: trimmed mean (drop extremes)
  // simple insertion sort (small n)
  for (int i = 1; i < m; i++) {
    float key = buf[i];
    int j = i - 1;
    while (j >= 0 && buf[j] > key) { buf[j+1] = buf[j]; j--; }
    buf[j+1] = key;
  }
  int trim = (m >= 7) ? 2 : (m >= 5 ? 1 : 0);   // trim 2 each side if enough samples
  float sum = 0.0f;
  int   k0  = trim;
  int   k1  = m - trim;
  for (int k = k0; k < k1; k++) sum += buf[k];
  float trimmed_avg = sum / (float)(k1 - k0);

  // 3) Smooth with EMA
  if (isnan(ema_phase_deg)) {
    ema_phase_deg = trimmed_avg;
  } else {
    ema_phase_deg = EMA_ALPHA * trimmed_avg + (1.0f - EMA_ALPHA) * ema_phase_deg;
  }
  float pf = fabs(cos(ema_phase_deg * DEG_TO_RAD));
  pf = clampf(pf, 0.0f, 1.0f);

  // Keep PF consistent with EMA of phase (no need to EMA PF separately)
  ema_pf = pf;

  // 4) Print stable reading
  Serial.print(F("samples="));   Serial.print(m);
  Serial.print(F("  phase(deg)=")); Serial.print(ema_phase_deg, 2);
  Serial.print(F("  PF="));        Serial.print(ema_pf, 3);
  Serial.print(F("  timeouts="));  Serial.print(timeout_ct);
  Serial.print(F("  glitches="));  Serial.println(glitch_ct);

  delay(160);  // modest update rate helps stability in Proteus/hardware
}
