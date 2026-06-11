#!/usr/bin/env python3
"""Synthesizes the demo source loops for The Kiss of Shame web demo.

Pure stdlib (no numpy): three seamless musical loops, 44.1k/16-bit stereo,
written to demo/audio/. Deliberately clean and modern-sounding, so the
plugin's tape degradation has something pristine to ruin.
"""

import array
import math
import os
import random
import wave

SR = 44100
random.seed(0x510)


def silence(seconds):
    n = int(round(seconds * SR))
    return [0.0] * n, [0.0] * n


def add(bus, start_s, mono, pan=0.0, gain=1.0):
    """Mix a mono event into the stereo bus at start (seconds), constant-power pan."""
    left, right = bus
    i0 = int(round(start_s * SR))
    gl = gain * math.cos((pan + 1) * math.pi / 4)
    gr = gain * math.sin((pan + 1) * math.pi / 4)
    n = len(left)
    for i, s in enumerate(mono):
        j = i0 + i
        if 0 <= j < n:
            left[j] += s * gl
            right[j] += s * gr


def env_exp(n, t60):
    k = math.log(1000.0) / (t60 * SR)
    return [math.exp(-k * i) for i in range(n)]


def kick(dur=0.22, f0=120.0, f1=46.0, punch=1.0):
    n = int(dur * SR)
    out = []
    phase = 0.0
    for i in range(n):
        t = i / n
        f = f0 * (f1 / f0) ** (t ** 0.5)
        phase += 2 * math.pi * f / SR
        e = math.exp(-5.2 * t)
        s = math.sin(phase) * e
        if i < 220:
            s += punch * 0.6 * (1 - i / 220.0) * (random.random() * 2 - 1) * 0.3
        out.append(math.tanh(1.8 * s))
    return out


def snare(dur=0.20, tone=190.0):
    n = int(dur * SR)
    out = []
    lp = 0.0
    phase = 0.0
    for i in range(n):
        t = i / n
        e = math.exp(-9.0 * t)
        w = random.random() * 2 - 1
        lp += 0.25 * (w - lp)          # tame the noise a little
        body = 0.45 * math.sin(phase) * math.exp(-14.0 * t)
        phase += 2 * math.pi * tone / SR
        out.append((0.8 * (w - lp) + body) * e)
    return out


def hat(dur=0.05, open_=False):
    n = int((0.16 if open_ else dur) * SR)
    out = []
    prev = 0.0
    for i in range(n):
        t = i / n
        w = random.random() * 2 - 1
        hp = w - prev                   # crude highpass
        prev = w
        out.append(hp * math.exp(-(6.0 if open_ else 22.0) * t) * 0.5)
    return out


def clap(dur=0.18):
    n = int(dur * SR)
    out = [0.0] * n
    for burst, bgain in ((0.0, 1.0), (0.012, 0.8), (0.026, 0.65)):
        b0 = int(burst * SR)
        prev = 0.0
        for i in range(b0, n):
            t = (i - b0) / SR
            w = random.random() * 2 - 1
            hp = w - 0.7 * prev
            prev = w
            out[i] += hp * math.exp(-34.0 * t) * 0.6 * bgain
    return out


def epiano(freq, dur, vel=1.0):
    """FM-flavoured electric piano: warm carrier + bell partial, slow tremolo."""
    n = int(dur * SR)
    out = []
    p1 = p2 = 0.0
    for i in range(n):
        t = i / SR
        e = math.exp(-2.1 * t) * (1 - math.exp(-90.0 * t / (n / SR)))
        trem = 1.0 + 0.10 * math.sin(2 * math.pi * 4.3 * t)
        p1 += 2 * math.pi * freq / SR
        p2 += 2 * math.pi * freq * 3.98 / SR
        s = math.sin(p1 + 0.35 * math.sin(p2) * math.exp(-5.0 * t))
        s += 0.18 * math.sin(p2) * math.exp(-7.0 * t)
        out.append(s * e * trem * vel * 0.5)
    return out


def bass(freq, dur, vel=1.0, drive=1.6):
    n = int(dur * SR)
    out = []
    p = 0.0
    for i in range(n):
        t = i / SR
        e = min(1.0, t * 200) * math.exp(-1.4 * t)
        p += 2 * math.pi * freq / SR
        s = math.sin(p) + 0.22 * math.sin(2 * p)
        out.append(math.tanh(drive * s) * e * vel * 0.6)
    return out


def saw(freq, dur, vel=1.0, detune=0.0, attack=0.004, release=4.0, harmonics=7):
    n = int(dur * SR)
    out = []
    f = freq * (1 + detune)
    phases = [0.0] * harmonics
    for i in range(n):
        t = i / SR
        e = min(1.0, t / max(attack, 1e-4)) * math.exp(-release * max(0.0, t - (dur - 0.35)) if t > dur - 0.35 else 0.0)
        s = 0.0
        for h in range(1, harmonics + 1):
            phases[h - 1] += 2 * math.pi * f * h / SR
            s += math.sin(phases[h - 1]) / h
        out.append(s * e * vel * 0.30)
    return out


NOTES = {"C": 0, "Db": 1, "D": 2, "Eb": 3, "E": 4, "F": 5, "Gb": 6,
         "G": 7, "Ab": 8, "A": 9, "Bb": 10, "B": 11}


def hz(name, octave):
    return 440.0 * 2 ** ((NOTES[name] - 9) / 12 + (octave - 4))


def normalize(bus, peak=0.86):
    left, right = bus
    m = max(1e-9, max(max(abs(s) for s in left), max(abs(s) for s in right)))
    g = peak / m
    return [s * g for s in left], [s * g for s in right]


def write_wav(path, bus):
    left, right = bus
    data = array.array("h")
    for l_, r_ in zip(left, right):
        data.append(int(max(-1.0, min(1.0, l_)) * 32767))
        data.append(int(max(-1.0, min(1.0, r_)) * 32767))
    with wave.open(path, "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    print(f"wrote {path} ({os.path.getsize(path) // 1024} KB, {len(left) / SR:.2f}s)")


# ============================================================ Dust & Keys
def dust_and_keys():
    bpm, bars = 90.0, 4
    beat = 60.0 / bpm
    bus = silence(bars * 4 * beat)

    chords = [
        [("F", 3), ("A", 3), ("C", 4), ("E", 4)],   # Fmaj7
        [("A", 3), ("C", 4), ("E", 4), ("G", 4)],   # Am7
        [("D", 3), ("F", 3), ("A", 3), ("C", 4)],   # Dm7
        [("Bb", 2), ("D", 3), ("F", 3), ("A", 3)],  # Bbmaj7
    ]
    roots = [("F", 1), ("A", 1), ("D", 1), ("Bb", 0)]

    for bar in range(bars):
        t0 = bar * 4 * beat
        chord = chords[bar % 4]

        for hit, vel in ((0.0, 1.0), (1.5, 0.7), (3.5, 0.55)):
            for vi, (note, octv) in enumerate(chord):
                pan = -0.35 + 0.23 * vi
                add(bus, t0 + hit * beat + vi * 0.011,
                    epiano(hz(note, octv), beat * 2.2, vel), pan=pan, gain=0.5)

        root = roots[bar % 4]
        add(bus, t0, bass(hz(*root), beat * 1.8), gain=0.65)
        add(bus, t0 + 2 * beat, bass(hz(*root), beat * 1.6, vel=0.8), gain=0.6)

        add(bus, t0, kick(), gain=0.9)
        add(bus, t0 + 1.75 * beat, kick(punch=0.6), gain=0.7)
        add(bus, t0 + 2.5 * beat, kick(punch=0.5), gain=0.75)
        add(bus, t0 + 1 * beat, snare(), gain=0.55)
        add(bus, t0 + 3 * beat, snare(), gain=0.6)
        for e in range(8):
            add(bus, t0 + e * beat / 2, hat(), pan=0.3, gain=0.30 if e % 2 else 0.42)
        add(bus, t0 + 3.5 * beat, hat(open_=True), pan=0.3, gain=0.3)

    write_wav("audio/dust_and_keys.wav", normalize(bus))


# ============================================================ Night Drive
def night_drive():
    bpm, bars = 100.0, 4
    beat = 60.0 / bpm
    bus = silence(bars * 4 * beat)

    prog = [("A", 1, [("A", 2), ("C", 3), ("E", 3)]),
            ("F", 1, [("F", 2), ("A", 2), ("C", 3)]),
            ("C", 2, [("C", 3), ("E", 3), ("G", 3)]),
            ("G", 1, [("G", 2), ("B", 2), ("D", 3)])]

    for bar in range(bars):
        t0 = bar * 4 * beat
        root_name, root_oct, pad_notes = prog[bar % 4]

        # arpeggiated saw bass: root, root, +5th, octave pattern in 8ths
        pattern = [0, 0, 7, 0, 12, 0, 7, 5]
        base = hz(root_name, root_oct + 1)
        for e, semis in enumerate(pattern):
            f = base * 2 ** (semis / 12)
            add(bus, t0 + e * beat / 2, saw(f, beat * 0.46, vel=0.9 if e % 2 == 0 else 0.7),
                gain=0.5)

        # pad: detuned saws, whole bar
        for vi, (note, octv) in enumerate(pad_notes):
            f = hz(note, octv + 1)
            add(bus, t0, saw(f, beat * 4, vel=0.30, detune=0.004, attack=0.35), pan=-0.4 + 0.4 * vi, gain=0.4)
            add(bus, t0, saw(f, beat * 4, vel=0.30, detune=-0.004, attack=0.35), pan=0.4 - 0.4 * vi, gain=0.4)

        for b in range(4):
            add(bus, t0 + b * beat, kick(dur=0.2, f0=100, f1=42), gain=0.95)
        add(bus, t0 + 1 * beat, clap(), pan=0.1, gain=0.5)
        add(bus, t0 + 3 * beat, clap(), pan=-0.1, gain=0.5)
        for e in range(8):
            if e % 2 == 1:
                add(bus, t0 + e * beat / 2, hat(), pan=0.35, gain=0.4)

    write_wav("audio/night_drive.wav", normalize(bus))


# ============================================================ Drum Bus
def drum_bus():
    bpm, bars = 95.0, 4
    beat = 60.0 / bpm
    bus = silence(bars * 4 * beat)

    for bar in range(bars):
        t0 = bar * 4 * beat
        add(bus, t0, kick(punch=1.2), gain=1.0)
        add(bus, t0 + 0.75 * beat, kick(punch=0.5), gain=0.6)
        add(bus, t0 + 2.25 * beat, kick(punch=0.8), gain=0.85)
        add(bus, t0 + 1 * beat, snare(), gain=0.7)
        add(bus, t0 + 3 * beat, snare(), gain=0.72)
        if bar == 3:
            for f, fgain in ((3.5, 0.5), (3.625, 0.55), (3.75, 0.62), (3.875, 0.7)):
                add(bus, t0 + f * beat, snare(dur=0.12, tone=150 + 60 * f), gain=fgain * 0.6)
        for e in range(16):
            g = 0.34 if e % 4 == 2 else (0.22 if e % 2 else 0.28)
            add(bus, t0 + e * beat / 4, hat(dur=0.035), pan=0.3 if e % 2 else -0.2, gain=g)
        add(bus, t0 + 2.5 * beat, hat(open_=True), pan=0.25, gain=0.35)

    write_wav("audio/drum_bus.wav", normalize(bus))


if __name__ == "__main__":
    os.makedirs("audio", exist_ok=True)
    dust_and_keys()
    night_drive()
    drum_bus()
