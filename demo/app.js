/* The Kiss of Shame — Rev 2 web demo. UI + transport. */
"use strict";

const ACCENT = "rgb(255,55,98)";
const ACCENT_HOT = "rgb(255,26,41)";
const ENVIRONMENTS = [
  ["Off", "Fresh reel. No storage damage."],
  ["Environs", "Decades on a shelf. Gentle wear."],
  ["Studio Closet", "Dry darkness. Dulling, light dropouts, stray crackle."],
  ["Humid Cellar", "Sticky-shed. Drowned highs, slow swell, damp rumble."],
  ["Hot Locker", "Heat warp. Drifting pitch, sagging level, hard print-through."],
  ["Hurricane Sandy", "The flood reel. Bursts, grit, and survival."],
];

const state = {
  input: 0.5, shame: 0, hiss: 0, age: 0, blend: 1, output: 0.5,
  flange: 0, bypass: 0, tape: 0, print: 0, env: 0, extreme: 0,
  playing: false, linked: false,
};

let ctx = null, worklet = null, sourceNode = null, currentBuffer = null;
let meter = { l: 0, r: 0 };

function send(id, value) { if (worklet) worklet.port.postMessage({ id, value }); }

function setParam(id, value) {
  state[id] = value;
  send(id, value);
  refreshSpine();
}

/* ============================== Knobs ============================== */
const START = -2.356, END = 2.356;

class Knob {
  constructor(el) {
    this.el = el;
    this.param = el.dataset.param;
    this.value = parseFloat(el.dataset.default || "0");
    this.isCross = el.classList.contains("cross");
    this.svg = el.querySelector("svg");
    this.svg.setAttribute("viewBox", "0 0 100 100");
    this.build();
    this.bind();
    this.render();
    state[this.param] = this.value;
  }

  build() {
    const ns = "http://www.w3.org/2000/svg";
    const mk = (tag, attrs) => {
      const n = document.createElementNS(ns, tag);
      for (const k in attrs) n.setAttribute(k, attrs[k]);
      this.svg.appendChild(n);
      return n;
    };

    const defsId = "g" + Math.random().toString(36).slice(2, 7);
    this.svg.innerHTML = `
      <defs>
        <radialGradient id="${defsId}b" cx="50%" cy="32%" r="75%">
          <stop offset="0%" stop-color="#3a3a3f"/><stop offset="100%" stop-color="#141416"/>
        </radialGradient>
        <radialGradient id="${defsId}h" cx="50%" cy="50%" r="50%">
          <stop offset="0%" stop-color="rgba(255,55,98,.5)"/><stop offset="100%" stop-color="rgba(255,55,98,0)"/>
        </radialGradient>
      </defs>`;

    // ticks
    for (let t = 0; t <= 10; t++) {
      const a = START + (END - START) * t / 10, major = t % 5 === 0;
      const r1 = 47, r2 = major ? 42 : 44.4;
      mk("line", {
        x1: 50 + r1 * Math.sin(a), y1: 50 - r1 * Math.cos(a),
        x2: 50 + r2 * Math.sin(a), y2: 50 - r2 * Math.cos(a),
        stroke: `rgba(255,255,255,${major ? 0.28 : 0.13})`, "stroke-width": major ? 1.6 : 1,
      });
    }

    this.halo = mk("circle", { cx: 50, cy: 50, r: 49, fill: `url(#${defsId}h)`, opacity: 0 });
    mk("ellipse", { cx: 50, cy: 54, rx: 36, ry: 34, fill: "rgba(0,0,0,.45)" });   // shadow
    mk("circle", { cx: 50, cy: 50, r: 35, fill: `url(#${defsId}b)`, stroke: "#0a0a0b", "stroke-width": 1.4 });
    mk("circle", { cx: 50, cy: 50, r: 30, fill: "none", stroke: "rgba(0,0,0,.5)", "stroke-width": 1 });
    mk("ellipse", { cx: 44, cy: 38, rx: 18, ry: 10, fill: "rgba(255,255,255,.06)" });

    this.track = mk("path", { fill: "none", stroke: "rgba(0,0,0,.6)", "stroke-width": 3.4, "stroke-linecap": "round" });
    this.track.setAttribute("d", this.arcPath(START, END, 41));
    this.glowArc = mk("path", { fill: "none", stroke: ACCENT, "stroke-width": 7, "stroke-linecap": "round", opacity: 0.22 });
    this.arc = mk("path", { fill: "none", stroke: ACCENT, "stroke-width": 2.6, "stroke-linecap": "round" });

    if (this.isCross) {
      // the Infernal Love cross — rotates with the value
      this.cross = mk("g", {});
      const c = (a) => { const n = document.createElementNS(ns, "rect");
        for (const k in a) n.setAttribute(k, a[k]); this.cross.appendChild(n); return n; };
      c({ x: 46.5, y: 24, width: 7, height: 52, rx: 3, fill: ACCENT,
          filter: "drop-shadow(0 0 6px rgba(255,55,98,.8))" });
      c({ x: 33, y: 53.5, width: 34, height: 7, rx: 3, fill: ACCENT });
      const heart = document.createElementNS(ns, "path");
      heart.setAttribute("d", "M50 62 C42.5 56.6 44.3 49.6 50 52 C55.7 49.6 57.5 56.6 50 62 Z");
      heart.setAttribute("fill", "#1c1c1e");
      this.cross.appendChild(heart);
    } else {
      this.pointer = mk("rect", { x: 48.6, y: 19, width: 2.8, height: 12, rx: 1.4, fill: "#e8e8ea" });
    }
  }

  arcPath(a0, a1, r) {
    const p = (a) => `${50 + r * Math.sin(a)} ${50 - r * Math.cos(a)}`;
    const large = a1 - a0 > Math.PI ? 1 : 0;
    return `M ${p(a0)} A ${r} ${r} 0 ${large} 1 ${p(a1)}`;
  }

  bind() {
    let startY = 0, startV = 0;
    this.svg.addEventListener("pointerdown", (e) => {
      this.svg.setPointerCapture(e.pointerId);
      startY = e.clientY; startV = this.value;
    });
    this.svg.addEventListener("pointermove", (e) => {
      if (e.buttons !== 1) return;
      const fine = e.shiftKey ? 0.25 : 1;
      this.set(startV + (startY - e.clientY) / 180 * fine);
    });
    this.svg.addEventListener("dblclick", () => {
      if (this.param === "shame") toggleExtreme();
      else this.set(parseFloat(this.el.dataset.default || "0"));
    });
    this.svg.addEventListener("pointerenter", () => { this.halo.setAttribute("opacity", state.extreme && this.isCross ? 1 : 0.55); });
    this.svg.addEventListener("pointerleave", () => { this.halo.setAttribute("opacity", this.isCross && state.extreme ? 0.8 : 0); });
  }

  set(v) {
    this.value = Math.min(1, Math.max(0, v));
    setParam(this.param, this.value);
    if (this.param === "input" && state.linked) knobs.output.setQuiet(1 - this.value);
    if (this.param === "output" && state.linked) knobs.input.setQuiet(1 - this.value);
    this.render();
  }

  setQuiet(v) { this.value = Math.min(1, Math.max(0, v)); setParam(this.param, this.value); this.render(); }

  render() {
    const a = START + this.value * (END - START);
    const colour = state.extreme && this.isCross ? ACCENT_HOT : ACCENT;
    this.arc.setAttribute("stroke", colour);
    this.glowArc.setAttribute("stroke", colour);
    if (this.value > 0.002) {
      const d = this.arcPath(START, a, 41);
      this.arc.setAttribute("d", d); this.glowArc.setAttribute("d", d);
      this.arc.removeAttribute("opacity");
    } else { this.arc.setAttribute("d", ""); this.glowArc.setAttribute("d", ""); }
    if (this.pointer) this.pointer.setAttribute("transform", `rotate(${a * 180 / Math.PI} 50 50)`);
    if (this.cross) this.cross.setAttribute("transform", `rotate(${a * 180 / Math.PI * 0.9} 50 50)`);
  }
}

const knobs = {};
document.querySelectorAll(".knob").forEach((el) => { knobs[el.dataset.param] = new Knob(el); });

/* ============================== EXTREME ============================== */
function toggleExtreme() {
  state.extreme = state.extreme ? 0 : 1;
  send("extreme", state.extreme);
  document.body.classList.toggle("extreme", !!state.extreme);
  document.getElementById("shameCap").textContent = state.extreme ? "SHAME — EXTREME" : "SHAME";
  knobs.shame.render();
  refreshSpine();
}

/* ============================== Buttons ============================== */
const bypassBtn = document.getElementById("bypassBtn");
bypassBtn.onclick = () => {
  setParam("bypass", state.bypass ? 0 : 1);
  bypassBtn.classList.toggle("on", !!state.bypass);
  bypassBtn.textContent = state.bypass ? "BYP" : "IN";
};

const tapeBtn = document.getElementById("tapeBtn");
tapeBtn.onclick = () => {
  setParam("tape", state.tape ? 0 : 1);
  tapeBtn.classList.toggle("on", !!state.tape);
  tapeBtn.textContent = state.tape ? "A-456" : "S-111";
};

const printBtn = document.getElementById("printBtn");
printBtn.onclick = () => {
  setParam("print", state.print ? 0 : 1);
  printBtn.classList.toggle("on", !!state.print);
};

const linkBtn = document.getElementById("linkBtn");
linkBtn.onclick = () => {
  state.linked = !state.linked;
  linkBtn.classList.toggle("on", state.linked);
  if (state.linked) knobs.output.setQuiet(1 - state.input);
};

/* ============================== Environment menu ============================== */
const envPill = document.getElementById("envPill");
const envMenu = document.getElementById("envMenu");
ENVIRONMENTS.forEach(([name, blurb], i) => {
  const opt = document.createElement("div");
  opt.className = "opt";
  opt.innerHTML = `<span class="dot"></span><span><b>${name}</b><small>${blurb}</small></span>`;
  opt.onclick = () => { selectEnv(i); envMenu.hidden = true; };
  envMenu.appendChild(opt);
});

function selectEnv(i) {
  setParam("env", i);
  envPill.innerHTML = `${ENVIRONMENTS[i][0].toUpperCase()} <i>▾</i>`;
  envPill.classList.toggle("active", i > 0);
  envMenu.querySelectorAll(".opt").forEach((o, j) => o.classList.toggle("sel", j === i));
}
selectEnv(0);

envPill.onclick = (e) => {
  e.stopPropagation();
  const r = envPill.getBoundingClientRect();
  envMenu.style.left = Math.max(8, Math.min(window.innerWidth - 328, r.left + r.width / 2 - 160)) + "px";
  envMenu.style.top = Math.max(8, r.top - 8 - 330) + "px";
  envMenu.hidden = !envMenu.hidden;
};
document.addEventListener("click", () => { envMenu.hidden = true; });

/* ============================== Status spine ============================== */
function refreshSpine() {
  document.getElementById("transportText").textContent = state.bypass ? "BYPASS" : state.playing ? "ROLLING" : "STANDBY";
  document.getElementById("lamp").classList.toggle("on", state.playing && !state.bypass);
  document.getElementById("tapeText").textContent = state.tape ? "A-456" : "S-111";
  document.getElementById("envText").textContent = ENVIRONMENTS[state.env][0].toUpperCase();
  document.getElementById("ageText").textContent = Math.round(state.age * 100) + "%";
  document.getElementById("ageBar").style.width = state.age * 100 + "%";
  document.getElementById("printText").textContent = state.print ? "ECHOING" : "OFF";
  document.getElementById("shameText").textContent = Math.round(state.shame * 100) + "%";
}
refreshSpine();

/* ============================== NAB reels (canvas) ============================== */
const reelCanvas = document.getElementById("reels");
const rg = reelCanvas.getContext("2d");
let reelAngle = 0, lastT = 0, flangeStartY = 0, flangeStartV = 0, draggingReel = false;

function drawReel(cx, cy, R, angle, pack) {
  const g = rg;
  // backplate
  g.fillStyle = "#070708";
  g.beginPath(); g.arc(cx, cy, R * 0.96, 0, 7); g.fill();

  // tape pack
  const packR = R * (0.36 + 0.54 * pack);
  const pg = g.createLinearGradient(cx, cy - packR, cx, cy + packR);
  pg.addColorStop(0, "#1b1410"); pg.addColorStop(1, "#0c0908");
  g.fillStyle = pg;
  g.beginPath(); g.arc(cx, cy, packR, 0, 7); g.fill();
  for (let i = 1; i <= 6; i++) {
    g.strokeStyle = `rgba(255,255,255,${0.025 + 0.005 * i})`;
    g.beginPath(); g.arc(cx, cy, packR * (0.42 + 0.095 * i), 0, 7); g.stroke();
  }

  // flange with three kidney windows (even-odd)
  const winO = R * 0.86, winI = R * 0.38, half = 0.92;
  g.save();
  g.beginPath();
  g.arc(cx, cy, R * 0.985, 0, 7);
  for (let s = 0; s < 3; s++) {
    const a = angle + s * (Math.PI * 2 / 3);
    g.moveTo(cx + winO * Math.sin(a - half), cy - winO * Math.cos(a - half));
    g.arc(cx, cy, winO, a - half - Math.PI / 2, a + half - Math.PI / 2);
    g.arc(cx, cy, winI, a + half - Math.PI / 2, a - half - Math.PI / 2, true);
    g.closePath();
  }
  g.clip("evenodd");
  const mg = g.createLinearGradient(cx, cy - R, cx, cy + R);
  mg.addColorStop(0, "#45454c"); mg.addColorStop(1, "#17171a");
  g.fillStyle = mg;
  g.fillRect(cx - R, cy - R, R * 2, R * 2);
  for (let i = 0; i < 7; i++) {
    g.strokeStyle = "rgba(0,0,0,.16)";
    g.beginPath(); g.arc(cx, cy, R * (0.42 + 0.085 * i), 0, 7); g.stroke();
  }
  const sg = g.createLinearGradient(cx - R * 0.7, cy - R, cx + R * 0.2, cy);
  sg.addColorStop(0, "rgba(255,255,255,.085)"); sg.addColorStop(1, "rgba(255,255,255,0)");
  g.fillStyle = sg;
  g.fillRect(cx - R, cy - R, R * 2, R * 1.1);
  g.restore();

  // rim
  g.strokeStyle = "rgba(255,255,255,.14)"; g.lineWidth = 2;
  g.beginPath(); g.arc(cx, cy, R * 0.985, 0, 7); g.stroke();
  g.strokeStyle = "rgba(0,0,0,.65)"; g.lineWidth = 1.4;
  g.beginPath(); g.arc(cx, cy, R, 0, 7); g.stroke();

  // NAB trilobe hub
  const hubR = R * 0.30;
  g.fillStyle = "#3c3c42";
  g.beginPath(); g.arc(cx, cy, hubR * 0.78, 0, 7); g.fill();
  for (let l = 0; l < 3; l++) {
    const a = angle + l * (Math.PI * 2 / 3) + Math.PI / 3;
    g.beginPath();
    g.arc(cx + hubR * 0.62 * Math.sin(a), cy - hubR * 0.62 * Math.cos(a), hubR * 0.42, 0, 7);
    g.fill();
  }
  g.fillStyle = "#222226";
  g.beginPath(); g.arc(cx, cy, hubR * 0.6, 0, 7); g.fill();
  g.strokeStyle = "rgba(255,55,98,.55)"; g.lineWidth = 1.6;
  g.beginPath(); g.arc(cx, cy, hubR * 0.6, 0, 7); g.stroke();
  g.fillStyle = "#0c0c0d";
  g.beginPath(); g.arc(cx, cy, hubR * 0.16, 0, 7); g.fill();
  for (let b = 0; b < 3; b++) {
    const a = angle + b * (Math.PI * 2 / 3);
    g.fillStyle = "#101011";
    g.beginPath();
    g.arc(cx + hubR * 0.4 * Math.sin(a), cy - hubR * 0.4 * Math.cos(a), Math.max(2.4, hubR * 0.09), 0, 7);
    g.fill();
  }
}

function drawReelBay(t) {
  const W = reelCanvas.width, H = reelCanvas.height;
  rg.clearRect(0, 0, W, H);

  const R = H * 0.44, ly = H * 0.455;
  const lx = W * 0.255, rx = W * 0.745;
  const tapeY = ly + R * 0.96;

  // tape band
  rg.fillStyle = "#060607";
  rg.fillRect(lx, tapeY - 4, rx - lx, 8);
  rg.fillStyle = "rgba(255,255,255,.1)";
  rg.fillRect(lx, tapeY - 4, rx - lx, 1.4);
  if (state.playing) {
    const gx = lx + ((t / 900) % 1) * (rx - lx);
    const gl = rg.createRadialGradient(gx, tapeY, 0, gx, tapeY, 60);
    gl.addColorStop(0, "rgba(255,255,255,.3)"); gl.addColorStop(1, "rgba(255,255,255,0)");
    rg.fillStyle = gl;
    rg.fillRect(gx - 60, tapeY - 4, 120, 8);
  }

  // head block
  const hb = { x: W / 2 - 70, y: tapeY - 52, w: 140, h: 56 };
  rg.fillStyle = "rgba(0,0,0,.5)"; rg.fillRect(hb.x, hb.y + 4, hb.w, hb.h);
  const hg = rg.createLinearGradient(0, hb.y, 0, hb.y + hb.h);
  hg.addColorStop(0, "#37373c"); hg.addColorStop(1, "#141416");
  rg.fillStyle = hg; rg.fillRect(hb.x, hb.y, hb.w, hb.h);
  rg.fillStyle = "#0a0a0b";
  for (let i = 0; i < 3; i++) rg.fillRect(hb.x + 18 + i * 38, hb.y + 16, 26, 26);

  drawReel(lx, ly, R, reelAngle, 0.85);
  drawReel(rx, ly, R, -reelAngle, 0.48);

  // ember
  const eg = rg.createRadialGradient(W / 2, H + 60, 0, W / 2, H + 60, H * 0.8);
  const emberA = state.extreme ? 0.16 : 0.09;
  eg.addColorStop(0, `rgba(255,55,98,${emberA + 0.04 * Math.sin(t / 700)})`);
  eg.addColorStop(1, "rgba(255,55,98,0)");
  rg.fillStyle = eg;
  rg.fillRect(0, H * 0.55, W, H * 0.45);
}

function tick(t) {
  const dt = Math.min(50, t - lastT); lastT = t;
  if (state.playing) reelAngle += dt * 0.0011 * (1 - 0.6 * state.flange);
  drawReelBay(t);
  drawVU(vuL, meter.l * 3); drawVU(vuR, meter.r * 3);
  requestAnimationFrame(tick);
}

const bay = document.getElementById("reelBay");
bay.addEventListener("pointerdown", (e) => {
  draggingReel = true; flangeStartY = e.clientY; flangeStartV = state.flange;
  bay.setPointerCapture(e.pointerId);
});
bay.addEventListener("pointermove", (e) => {
  if (!draggingReel) return;
  setParam("flange", Math.min(1, Math.max(0, flangeStartV + (e.clientY - flangeStartY) / 200)));
});
bay.addEventListener("pointerup", () => { draggingReel = false; });
bay.addEventListener("dblclick", () => setParam("flange", 0));

/* ============================== VU (ember glass) ============================== */
const vuL = document.getElementById("vuL").getContext("2d");
const vuR = document.getElementById("vuR").getContext("2d");
const needleState = new WeakMap();

function drawVU(g, target) {
  const W = g.canvas.width, H = g.canvas.height;
  let v = needleState.get(g) || 0;
  v += (Math.min(1, target) - v) * 0.25;
  needleState.set(g, v);

  g.clearRect(0, 0, W, H);
  const r = 8;

  // bezel + smoked face
  g.fillStyle = "#0e0e10";
  g.beginPath(); g.roundRect(0, 0, W, H, 16); g.fill();
  const face = { x: 8, y: 8, w: W - 16, h: H - 16 };
  const fg = g.createLinearGradient(0, face.y, 0, face.y + face.h);
  fg.addColorStop(0, "#221511"); fg.addColorStop(1, "#0c0807");
  g.fillStyle = fg;
  g.beginPath(); g.roundRect(face.x, face.y, face.w, face.h, 10); g.fill();

  const px = W / 2, py = face.y + face.h * 0.84, len = face.h * 0.62;

  // lamp rides the level
  const lamp = g.createRadialGradient(px, py, 0, px, py, len * 1.3);
  lamp.addColorStop(0, `rgba(255,110,80,${0.16 + 0.45 * v})`);
  lamp.addColorStop(1, "rgba(255,110,80,0)");
  g.fillStyle = lamp;
  g.beginPath(); g.roundRect(face.x, face.y, face.w, face.h, 10); g.fill();

  // ticks + red zone
  for (let t = 0; t <= 12; t++) {
    const a = -0.85 + 1.7 * t / 12, major = t % 3 === 0;
    g.strokeStyle = t / 12 > 0.78 ? ACCENT : `rgba(233,220,192,${(major ? 0.85 : 0.45) * (0.55 + 0.45 * v)})`;
    g.lineWidth = major ? 2.4 : 1.4;
    g.beginPath();
    g.moveTo(px + len * Math.sin(a), py - len * Math.cos(a));
    g.lineTo(px + (len - (major ? 11 : 6)) * Math.sin(a), py - (len - (major ? 11 : 6)) * Math.cos(a));
    g.stroke();
  }
  g.strokeStyle = ACCENT; g.lineWidth = 4;
  g.beginPath();
  g.arc(px, py, len + 3, -Math.PI / 2 + (-0.85 + 1.7 * 0.78), -Math.PI / 2 + 0.85);
  g.stroke();

  // needle: pink bloom + ivory blade
  const na = -0.85 + 1.7 * v;
  const tx = px + (len - 4) * Math.sin(na), ty = py - (len - 4) * Math.cos(na);
  g.strokeStyle = `rgba(255,55,98,${0.2 + 0.3 * v})`; g.lineWidth = 7;
  g.beginPath(); g.moveTo(px, py); g.lineTo(tx, ty); g.stroke();
  g.strokeStyle = "#e9dcc0"; g.lineWidth = 2.6;
  g.beginPath(); g.moveTo(px, py); g.lineTo(tx, ty); g.stroke();

  g.fillStyle = "#55555a";
  g.beginPath(); g.arc(px, py, 7, 0, 7); g.fill();
  g.strokeStyle = "rgba(255,55,98,.5)"; g.lineWidth = 1.5;
  g.beginPath(); g.arc(px, py, 7, 0, 7); g.stroke();

  g.fillStyle = `rgba(233,220,192,${0.35 + 0.3 * v})`;
  g.font = "700 13px system-ui";
  g.textAlign = "center";
  g.fillText("VU", px, face.y + face.h * 0.42);
}

/* ============================== Transport ============================== */
const playBtn = document.getElementById("playBtn");
const sourceSel = document.getElementById("source");
const fileInput = document.getElementById("fileInput");
const bufferCache = new Map();

async function ensureAudio() {
  if (ctx) return;
  ctx = new (window.AudioContext || window.webkitAudioContext)();
  await ctx.audioWorklet.addModule("kos-processor.js");
  worklet = new AudioWorkletNode(ctx, "kos", { outputChannelCount: [2] });
  worklet.connect(ctx.destination);
  worklet.port.onmessage = (e) => { meter.l = e.data.rmsL; meter.r = e.data.rmsR; };
  for (const id of ["input", "shame", "hiss", "age", "blend", "output", "flange",
                    "bypass", "tape", "print", "env", "extreme"])
    send(id, state[id]);
}

async function loadBuffer(url) {
  if (bufferCache.has(url)) return bufferCache.get(url);
  const data = await (await fetch(url)).arrayBuffer();
  const buf = await ctx.decodeAudioData(data);
  bufferCache.set(url, buf);
  return buf;
}

async function start() {
  await ensureAudio();
  await ctx.resume();
  const sel = sourceSel.value;
  if (sel === "upload") {
    if (!currentBuffer) { fileInput.click(); return; }
  } else {
    currentBuffer = await loadBuffer(sel);
  }
  stopSource();
  sourceNode = ctx.createBufferSource();
  sourceNode.buffer = currentBuffer;
  sourceNode.loop = true;
  sourceNode.connect(worklet);
  sourceNode.start();
  state.playing = true;
  playBtn.textContent = "■ STOP";
  refreshSpine();
}

function stopSource() {
  if (sourceNode) { try { sourceNode.stop(); } catch (_) {} sourceNode.disconnect(); sourceNode = null; }
}

function stop() {
  stopSource();
  state.playing = false;
  playBtn.textContent = "▶ ROLL TAPE";
  meter.l = meter.r = 0;
  refreshSpine();
}

playBtn.onclick = () => (state.playing ? stop() : start());
sourceSel.onchange = () => {
  if (sourceSel.value === "upload") fileInput.click();
  else if (state.playing) start();
};
fileInput.onchange = async () => {
  const f = fileInput.files[0];
  if (!f) return;
  await ensureAudio();
  currentBuffer = await ctx.decodeAudioData(await f.arrayBuffer());
  start();
};

requestAnimationFrame(tick);
