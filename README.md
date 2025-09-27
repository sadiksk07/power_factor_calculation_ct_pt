XOR-Based Power Factor & Phase Meter (Arduino)

Measure the absolute phase angle |φ| between voltage and current using an XOR pulse, and compute power factor PF = |cos φ|. The sketch prints stable readings to the Serial Monitor and includes robust noise filtering (trimmed mean, outlier gate, minimum pulse width, EMA smoothing). Optional CT/PT zero-cross inputs are supported if you later want signed lead/lag.

Features

Reads XOR pulse width on D9 and converts it to |phase| (degrees).

Computes PF = |cos φ|.

Noise-hardened: multi-sample averaging, trimmed mean, outlier rejection, min-pulse deglitch, EMA smoothing.

Optional PT period estimate for 50/60 Hz mains (defaults to 50 Hz).

Serial output @ 115200 baud (works with Proteus Virtual Terminal too).

Hardware

Arduino Uno (or compatible 5V board)

Voltage zero-cross + Current zero-cross → XOR logic (CMOS/TTL 0–5 V output)

Common GND between comparators/XOR and Arduino

⚠️ Mains safety: Use proper isolation (PT/CT, optos, comparators with hysteresis). Do not connect mains directly to Arduino.

Pinout
Signal	Pin	Notes
XOR output	D9	Required. HIGH pulse width encodes |phase|
PT (optional)	D3	Optional square wave used to estimate period
CT (unused here)	D2	Reserve if you add signed CT/PT timing later
How it works

XOR outputs a HIGH pulse whose width is proportional to the phase difference between V and I.

The sketch measures the HIGH time (pulseIn) and converts it to degrees:
phase = 360 * (pulseWidth / period) then wrapped to [0..180].

PF is computed as PF = |cos(phase)|.

Filtering stack:

Read N pulses per update (default 9)

Reject too-short/too-long pulses and outliers vs running value

Trimmed mean (drops extremes) → EMA smoothing

Quick Start

Wire XOR → D9 (0–5 V), common ground.

Open the sketch and set your mains frequency:

const float GRID_FREQ = 50.0f; // set 60.0f for 60 Hz


Upload to Arduino, open Serial Monitor @ 115200.

Sample output
samples=8  phase(deg)=44.95  PF=0.707  timeouts=0  glitches=1
samples=9  phase(deg)=45.03  PF=0.707  timeouts=0  glitches=1

Configuration (top of sketch)

GRID_FREQ — 50.0 or 60.0 Hz

SAMPLES_PER_UPDATE — pulses per reading (↑ = steadier, slower)

EMA_ALPHA — 0..1, smoothing (lower = calmer)

OUTLIER_GATE_DEG — drop any single sample jumping past this (deg)

MIN_HIGH_US, MIN_PHASE_DEG, MAX_PHASE_DEG — deglitch bounds

Troubleshooting

“No XOR pulse”: XOR pin stuck HIGH/LOW, no common GND, or not 0–5 V logic.

Jitter: Add Schmitt-trigger comparators/hysteresis, increase SAMPLES_PER_UPDATE, lower EMA_ALPHA.

Wrong frequency: Set GRID_FREQ correctly or enable PT-based period estimate.
