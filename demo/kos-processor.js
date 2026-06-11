// The Kiss of Shame — web demo DSP.
// A faithful AudioWorklet port of the Rev 2 signal graph:
//   input drive -> [saturation -> flange -> environment -> hiss -> shame
//   -> print-through] -> blend -> output
// Ported from Source/AudioProcessing (Brian Hansen's 2014 algorithms,
// Rev 2 modernization), including Shame EXTREME.

class KosProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    const sr = sampleRate;
    this.p = {
      input: 0.5, shame: 0, hiss: 0, age: 0, blend: 1, output: 0.5,
      flange: 0, bypass: 0, tape: 0, print: 0, env: 0, extreme: 0,
    };

    // saturation
    this.satPrior = [0, 0];
    this.evenGain = 0.3; this.satRateOdd = 2.0; this.satRateEven = 0.272;
    this.rolloffCoef = Math.min(1, 4000 * 2 * Math.PI / sr);

    // shame (wow/flutter/drift)
    this.shameLen = Math.round(sr);
    this.shameBuf = [new Float32Array(this.shameLen), new Float32Array(this.shameLen)];
    this.shamePos = 0; this.playPos = 0;
    this.tablePhase = 0; this.rateFluct = 0; this.scrapePhase = 0;
    this.sDepth = 0; this.sRate = 7; this.sPeriod = 0.5;
    this.scrapeDepth = 0; this.scrapeRate = 38;

    // flange
    this.flangeLen = Math.max(64, Math.round(2000 * sr / 44100));
    this.flangeBuf = [new Float32Array(this.flangeLen), new Float32Array(this.flangeLen)];
    this.flangePos = 0; this.flangePlay = 0; this.flangeCur = 0;

    // hiss: filtered noise bed
    this.hissLP = 0; this.hissHP = 0; this.hissPrev = 0;

    // print-through
    this.printLen = Math.round(0.11 * sr);
    this.printBuf = [new Float32Array(this.printLen), new Float32Array(this.printLen)];
    this.printPos = 0; this.printLP = [0, 0]; this.printAmt = 0;

    // environment
    this.envLP = [0, 0]; this.envLPCoef = 1;
    this.dipPoints = []; this.dipPos = 0; this.dipLen = sr; this.makeDips();
    this.crackleEnv = 0; this.cracklePrev = 0;
    this.burstPhase = 0; this.grainLP = 0; this.rumbleLP = 0;

    // smoothed gains
    this.bypassMix = 0; this.driveSm = 1; this.outSm = 1;

    // metering
    this.meterCount = 0; this.meterL = 0; this.meterR = 0;

    this.port.onmessage = (e) => {
      const { id, value } = e.data;
      this.p[id] = value;
      if (id === 'shame' || id === 'extreme') this.updateShame();
      if (id === 'tape') this.updateTape();
    };
    this.updateShame(); this.updateTape();
  }

  updateTape() {
    const a456 = this.p.tape > 0.5;
    this.evenGain = a456 ? 0.15 : 0.3;
    this.satRateOdd = a456 ? 1.6 : 2.0;
    this.rolloffCoef = Math.min(1, (a456 ? 6500 : 4000) * 2 * Math.PI / sampleRate);
    this.hissScale = a456 ? 0.6 : 1.0;
  }

  // The original three-segment macro mapping + the EXTREME overlay.
  updateShame() {
    const x = Math.min(1, Math.max(0, this.p.shame));
    let depth, period, rate;
    if (x <= 0.5) { depth = 5 * x / 0.5; period = 0.5; rate = 7; }
    else if (x <= 0.85) {
      const f = (x - 0.5) / 0.35;
      depth = 5 + 25 * f; period = 0.5 - 0.25 * f; rate = 7 + 70 * f;
    } else {
      const f = (x - 0.85) / 0.15;
      depth = 30 + 30 * f; period = 0.25 + 0.5 * f; rate = 77 - 20 * f;
    }
    if (this.p.extreme > 0.5) {
      depth *= 2.5; rate = Math.min(rate * 1.6, 120);
      period = Math.min(1, period * 1.5 + 0.15);
      this.scrapeDepth = 4 * x * sampleRate / 44100;
      this.scrapeRate = 38 + 30 * x;
    } else this.scrapeDepth = 0;
    this.sDepth = depth * sampleRate / 44100;
    this.sRate = rate; this.sPeriod = period;
  }

  makeDips() {
    const extremity = this.dipExtremity || 0;
    const pts = [[0, 1]];
    const n = 6 + Math.floor(Math.random() * 8);
    for (let i = 0; i < n; i++)
      pts.push([Math.min(0.999, Math.max(0.001, (i + 1) / (n + 1) + (Math.random() - 0.5) * 0.1)),
                1 - extremity * Math.random()]);
    pts.push([1, 1]);
    this.dipPoints = pts;
  }

  dipValue() {
    const t = this.dipPos / this.dipLen;
    const pts = this.dipPoints;
    let a = pts[0], b = pts[pts.length - 1];
    for (let i = pts.length - 1; i >= 0; i--)
      if (t >= pts[i][0]) { a = pts[i]; b = pts[i + 1] || pts[i]; break; }
    const span = b[0] - a[0];
    const f = span > 0 ? (t - a[0]) / span : 0;
    if (++this.dipPos >= this.dipLen) { this.dipPos = 0; this.makeDips(); }
    return a[1] * (1 - f) + b[1] * f;
  }

  process(inputs, outputs) {
    const inp = inputs[0], out = outputs[0];
    if (!out || out.length === 0) return true;
    const n = out[0].length;
    const sr = sampleRate;
    const ch = Math.min(2, out.length);

    const P = this.p;
    const bypassTarget = P.bypass > 0.5 ? 1 : 0;
    const driveTarget = Math.pow(10, (P.input * 36 - 18) / 20);
    const outTarget = Math.pow(10, (P.output * 36 - 18) / 20);
    const hissGain = P.hiss * 0.012 * (this.hissScale || 1);
    const printTarget = P.print > 0.5 ? 0.0158 : 0;
    const blend = P.blend;
    const flangeTargetSamples = P.flange * 1000 * sr / 44100;

    // environment configuration (intensity = age)
    const env = P.env | 0, age = P.age;
    let lpHz = 20000, dipDepth = 0, crackleP = 0, burstAmt = 0, rumbleAmt = 0;
    if (env > 0 && age > 0.001) {
      if (env === 1) { lpHz = 14000 * (1 - age) + 6500 * age; dipDepth = 0.25 * age; crackleP = 6 * age / sr; }
      if (env === 2) { lpHz = 10000 * (1 - age) + 2800 * age; dipDepth = 0.45 * age; rumbleAmt = 0.08 * age; }
      if (env === 3) { lpHz = 12000 * (1 - age) + 6000 * age; dipDepth = 0.30 * age; }
      if (env === 4) { lpHz = 20050 * (1 - age) + 2000; dipDepth = 0.5 * age; burstAmt = age > 0.5 ? 2 * (age - 0.5) : 0; crackleP = 14 * age / sr; }
    }
    this.dipExtremity = dipDepth;
    this.envLPCoef = Math.min(1, lpHz * 2 * Math.PI / sr);

    for (let i = 0; i < n; i++) {
      this.bypassMix += (bypassTarget - this.bypassMix) * 0.0008;
      this.driveSm += (driveTarget - this.driveSm) * 0.002;
      this.outSm += (outTarget - this.outSm) * 0.002;
      this.printAmt += (printTarget - this.printAmt) * 0.001;

      // shared modulators for this frame
      // shame wavetable: 0.5*(cos-1), rate in Hz with random per-cycle fluctuation
      this.tablePhase += (this.sRate + this.rateFluct) / sr;
      if (this.tablePhase >= 1) {
        this.tablePhase -= 1;
        this.rateFluct = (Math.random() * 2 - 1) * this.sRate * this.sPeriod;
      }
      const wav = 0.5 * (Math.cos(2 * Math.PI * this.tablePhase) - 1);
      let modPos = this.sDepth * wav;
      if (this.scrapeDepth > 0) {
        this.scrapePhase += 2 * Math.PI * this.scrapeRate / sr;
        modPos += this.scrapeDepth * Math.sin(this.scrapePhase);
      }

      // flange depth smoothing (original time constant)
      const fdiff = flangeTargetSamples - this.flangeCur;
      this.flangeCur += Math.abs(fdiff) < 0.01 ? fdiff : fdiff * 0.001 * (44100 / sr);

      const dip = dipDepth > 0 ? this.dipValue() : 1;

      // env extras (mono modulators, shared across channels)
      let crackle = 0, burst = 0, rumble = 0;
      if (crackleP > 0) {
        if (Math.random() < crackleP) this.crackleEnv = 0.12 * age * (0.4 + 0.6 * Math.random());
        this.crackleEnv *= 0.992;
        const w = Math.random() * 2 - 1;
        crackle = this.crackleEnv * (w - this.cracklePrev); this.cracklePrev = w;
      }
      if (burstAmt > 0) {
        this.burstPhase += 1 / (0.55 * sr);
        if (this.burstPhase >= 1) this.burstPhase -= 1;
        const be = this.burstPhase < 0.35 ? Math.sin(Math.PI * this.burstPhase / 0.35) : 0;
        burst = burstAmt * 0.05 * be * (Math.random() * 2 - 1);
        const g = Math.random() * 2 - 1;
        this.grainLP += 0.08 * (g - this.grainLP);
        burst += burstAmt * 0.12 * this.grainLP;
      }
      if (rumbleAmt > 0) {
        const w = Math.random() * 2 - 1;
        this.rumbleLP += 0.004 * (w - this.rumbleLP);
        rumble = rumbleAmt * this.rumbleLP * 3;
      }

      // hiss bed: shaped noise, two-pole-ish
      const hw = Math.random() * 2 - 1;
      this.hissLP += 0.45 * (hw - this.hissLP);
      const hiss = (this.hissLP - this.hissPrev * 0.2);
      this.hissPrev = this.hissLP;

      const readPos = ((this.shamePos + modPos) % this.shameLen + this.shameLen) % this.shameLen;
      const r0 = readPos | 0, r1 = (r0 + 1) % this.shameLen, rf = readPos - r0;

      let fPlay = this.flangePos - this.flangeCur;
      fPlay = ((fPlay % this.flangeLen) + this.flangeLen) % this.flangeLen;
      const f0 = fPlay | 0, f1 = (f0 + 1) % this.flangeLen, ff = fPlay - f0;

      for (let c = 0; c < ch; c++) {
        const dry = inp && inp[c] ? inp[c][i] : 0;
        let s = dry * this.driveSm;
        const driven = s;

        // --- saturation: odd/even tanh + rolloff (original constants)
        const odd = s > 0
          ? Math.tanh(this.satRateOdd * s)
          : -Math.tanh(this.satRateOdd * -s);
        const even = Math.tanh(this.satRateEven * Math.abs(s));
        s = (1.0 * odd + this.evenGain * this.evenGain * even) / (1.0 + this.evenGain);
        this.satPrior[c] = this.rolloffCoef * s + (1 - this.rolloffCoef) * this.satPrior[c];
        s = this.satPrior[c];

        // --- flange (reel touch)
        const fb = this.flangeBuf[c];
        fb[this.flangePos] = s;
        s = 0.5 * s + 0.5 * (fb[f0] * (1 - ff) + fb[f1] * ff);

        // --- environment
        this.envLP[c] += this.envLPCoef * (s - this.envLP[c]);
        if (env > 0 && age > 0.001) s = this.envLP[c];
        s = s * ((1 - dipDepth) + dipDepth * dip) + crackle + burst + rumble;

        // --- hiss
        s += hiss * hissGain;

        // --- shame: modulated tape playback
        const sb = this.shameBuf[c];
        sb[this.shamePos] = s;
        s = sb[r0] * (1 - rf) + sb[r1] * rf;

        // --- print-through: dulled post-echo
        const pb = this.printBuf[c];
        const echo = pb[this.printPos];
        this.printLP[c] += 0.5 * (echo - this.printLP[c]);
        pb[this.printPos] = s;
        s += this.printAmt * this.printLP[c];

        // --- blend against the driven dry, output gain, bypass crossfade
        s = (1 - blend) * driven + blend * s;
        s *= this.outSm;
        s = (1 - this.bypassMix) * s + this.bypassMix * dry;

        out[c][i] = s;
        if (c === 0) this.meterL += s * s; else this.meterR += s * s;
      }

      this.shamePos = (this.shamePos + 1) % this.shameLen;
      this.flangePos = (this.flangePos + 1) % this.flangeLen;
      this.printPos = (this.printPos + 1) % this.printLen;
    }

    this.meterCount += n;
    if (this.meterCount >= 2048) {
      this.port.postMessage({
        rmsL: Math.sqrt(this.meterL / this.meterCount),
        rmsR: Math.sqrt(this.meterR / Math.max(1, this.meterCount)),
      });
      this.meterCount = 0; this.meterL = 0; this.meterR = 0;
    }

    return true;
  }
}

registerProcessor('kos', KosProcessor);
