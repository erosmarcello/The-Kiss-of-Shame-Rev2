/* The Kiss of Shame — Rev 2 demo.
   The original 2014 interface: every control is the real faceplate asset,
   driven as a filmstrip, laid out per positions.rtf. */
"use strict";

const ENVIRONMENTS = ["Off", "Studio Closet", "Humid Cellar", "Hot Locker", "Hurricane Sandy"];

const state = {
  input: 0.5, shame: 0, hiss: 0, age: 0, blend: 1, output: 0.5,
  flange: 0, bypass: 0, tape: 0, print: 0, env: 0, extreme: 0,
  playing: false, linked: false, collapsed: false,
};

let ctx = null, worklet = null, sourceNode = null, currentBuffer = null;
let bufferIsUpload = false, currentFileName = "audio";
const meter = { l: 0, r: 0, dispL: 0, dispR: 0 };

function send(id, value) { if (worklet) worklet.port.postMessage({ id, value }); }
function setParam(id, value) { state[id] = value; send(id, value); }

/* ============================== filmstrips ============================== */
// geometry straight from positions.rtf / the original editor
function strip(id, asset, x, y, w, h, frames, shifts = true) {
  const el = document.getElementById(id);
  el.style.left = x + "px";
  el.style.top = shifts ? `calc(${y}px + var(--shift))` : y + "px";
  el.style.width = w + "px";
  el.style.height = h + "px";
  el.style.backgroundImage = `url(assets/${asset}.webp)`;
  el.style.backgroundSize = `${w}px ${h * frames}px`;
  return {
    el, h, frames,
    setFrame(i) { el.style.backgroundPositionY = -Math.round(i) * h + "px"; },
    setValue(v) { this.setFrame(Math.round(Math.min(1, Math.max(0, v)) * (frames - 1))); },
  };
}

// knobs: 65-frame rotaries, vertical drag, like the plugin
function knob(id, asset, x, y, w, h, onSet) {
  const s = strip(id, asset, x, y, w, h, 65);
  const param = s.el.dataset.param;
  const initial = parseFloat(s.el.dataset.default || "0");
  state[param] = initial;
  s.setValue(initial);

  let startY = 0, startV = 0;
  s.el.addEventListener("pointerdown", (e) => {
    s.el.setPointerCapture(e.pointerId);
    startY = e.clientY; startV = state[param];
  });
  s.el.addEventListener("pointermove", (e) => {
    if (e.buttons !== 1) return;
    const fine = e.shiftKey ? 0.25 : 1;
    const v = Math.min(1, Math.max(0, startV + (startY - e.clientY) / 180 * fine));
    setParam(param, v);
    s.setValue(v);
    if (onSet) onSet(v);
  });
  s.el.addEventListener("dblclick", () => {
    if (param === "shame") { toggleExtreme(); return; }
    setParam(param, initial);
    s.setValue(initial);
    if (onSet) onSet(initial);
  });
  return s;
}

const shameRing = strip("shameRing", "09_alpha", 401, 491, 174, 163, 65);

const knobs = {};
knobs.input = knob("k-input", "06_alpha", 104, 521, 116, 116, (v) => {
  if (state.linked) { setParam("output", 1 - v); knobs.output.setValue(1 - v); }
});
knobs.age = knob("k-age", "03_alpha", 350, 455, 74, 72);
knobs.shame = knob("k-shame", "09_v2", 401, 491, 174, 163, (v) => shameRing.setValue(v));
knobs.hiss = knob("k-hiss", "04_alpha", 547, 455, 78, 72);
knobs.blend = knob("k-blend", "05_alpha", 705, 455, 78, 72);
knobs.output = knob("k-output", "12_alpha", 757, 521, 122, 116, (v) => {
  if (state.linked) { setParam("input", 1 - v); knobs.input.setValue(1 - v); }
});

/* VU meters: 65-frame needles */
const vuL = strip("vuL", "08", 251, 518, 108, 108, 65);
const vuR = strip("vuR", "10", 605, 518, 110, 108, 65);
vuL.setFrame(0); vuR.setFrame(0);

/* environment LED window: frames 0=header, 1=OFF, 2..5=environments */
const env = strip("env", "00", 388, 654, 183, 32, 6);
function showEnv() {
  env.setFrame(state.env + 1);
  env.el.title = "Storage environment: " + ENVIRONMENTS[state.env]
               + " — click to cycle, scroll to step. AGE sets the damage.";
}
env.el.addEventListener("click", () => { setParam("env", (state.env + 1) % 5); showEnv(); });
env.el.addEventListener("wheel", (e) => {
  e.preventDefault();
  const step = e.deltaY > 0 ? 1 : 4; // +1 or -1 mod 5
  setParam("env", (state.env + step) % 5);
  showEnv();
}, { passive: false });
showEnv();

/* bypass: the lamp (01) shows it, the lever (02) throws it */
const lamp = strip("lamp", "01", 202, 469, 34, 34, 4); // 4-frame strip; rows 0 (dark) and 2 (lit) in use
const lever = strip("lever", "02", 232, 502, 44, 37, 2);
function showBypass() {
  lamp.setFrame(state.bypass ? 0 : 2);   // row 68 = lit, row 0 = dark
  lever.setFrame(state.bypass ? 1 : 0);
  document.body.classList.toggle("bypassed", !!state.bypass);
}
function toggleBypass() { setParam("bypass", state.bypass ? 0 : 1); showBypass(); }
lamp.el.addEventListener("click", toggleBypass);
lever.el.addEventListener("click", toggleBypass);
showBypass();

/* tape formulation: S-111 / A-456 */
const tape = strip("tape", "07", 233, 610, 42, 39, 2);
tape.el.addEventListener("click", () => {
  setParam("tape", state.tape ? 0 : 1);
  tape.setFrame(state.tape);
  tape.el.title = "Tape formulation: " + (state.tape ? "A-456" : "S-111");
});

/* print through */
const print_ = strip("print", "11", 698, 609, 47, 41, 2);
print_.el.addEventListener("click", () => {
  setParam("print", state.print ? 0 : 1);
  print_.setFrame(state.print);
});

/* link input/output */
for (const [id, x] of [["linkL", 137], ["linkR", 792]]) {
  const l = strip(id, "link", x, 605, 30, 30, 1);
  l.el.addEventListener("click", () => {
    state.linked = !state.linked;
    document.getElementById("linkL").classList.toggle("on", state.linked);
    document.getElementById("linkR").classList.toggle("on", state.linked);
    if (state.linked) { setParam("output", 1 - state.input); knobs.output.setValue(1 - state.input); }
  });
}

/* ============================== EXTREME ============================== */
function toggleExtreme() {
  state.extreme = state.extreme ? 0 : 1;
  send("extreme", state.extreme);
  document.body.classList.toggle("extreme", !!state.extreme);
}

/* ============================== reels ============================== */
const reelsCanvas = document.getElementById("reels");
const rg = reelsCanvas.getContext("2d");
const wheels = new Image();
wheels.src = "assets/wheels.webp";
let reelFrame = 0, reelAcc = 0, lastT = 0;
let flangeStartY = 0, flangeStartV = 0;

wheels.decode().then(() => drawReelFrame()).catch(() => {});

function drawReelFrame() {
  if (!wheels.complete || !wheels.naturalWidth) return;
  rg.clearRect(0, 0, 960, 322);
  rg.drawImage(wheels, 0, (reelFrame % 31) * 322, 960, 322, 0, 0, 960, 322);
}

reelsCanvas.addEventListener("pointerdown", (e) => {
  reelsCanvas.setPointerCapture(e.pointerId);
  flangeStartY = e.clientY; flangeStartV = state.flange;
});
reelsCanvas.addEventListener("pointermove", (e) => {
  if (e.buttons !== 1) return;
  setParam("flange", Math.min(1, Math.max(0, flangeStartV + (e.clientY - flangeStartY) / 200)));
});
reelsCanvas.addEventListener("dblclick", (e) => { e.stopPropagation(); setParam("flange", 0); });

/* ============================== render loop ============================== */
function tick(t) {
  const dt = Math.min(60, t - lastT); lastT = t;

  if (state.playing && !state.collapsed) {
    // 50fps frame cadence, slowed while leaning on the tape
    reelAcc += dt * 0.05 * (1 - 0.6 * state.flange);
    if (reelAcc >= 1) { reelFrame += Math.floor(reelAcc); reelAcc %= 1; drawReelFrame(); }
  }

  // VU ballistics
  const tgtL = state.bypass ? 0 : Math.min(1, meter.l * 3);
  const tgtR = state.bypass ? 0 : Math.min(1, meter.r * 3);
  meter.dispL += (tgtL - meter.dispL) * 0.3;
  meter.dispR += (tgtR - meter.dispR) * 0.3;
  vuL.setValue(meter.dispL);
  vuR.setValue(meter.dispR);

  requestAnimationFrame(tick);
}
requestAnimationFrame(tick);

/* ============================== collapse (dblclick the plate) ============================== */
const stage = document.getElementById("stage");
stage.addEventListener("dblclick", (e) => {
  if (e.target.closest(".strip") || e.target.closest(".reels")) return;
  state.collapsed = !state.collapsed;
  stage.classList.toggle("collapsed", state.collapsed);
  stage.style.setProperty("--shift", state.collapsed ? "-437px" : "0px");
  layout();
});

/* ============================== responsive scale ============================== */
function layout() {
  const holder = document.querySelector(".stage-holder");
  const w = holder.parentElement.clientWidth;
  const s = Math.min(1.15, w / 960);
  stage.style.transform = `scale(${s})`;
  holder.style.height = (state.collapsed ? 266 : 703) * s + "px";
}
window.addEventListener("resize", layout);
layout();

/* ============================== transport ============================== */
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
}

function stopSource() {
  if (sourceNode) { try { sourceNode.stop(); } catch (_) {} sourceNode.disconnect(); sourceNode = null; }
}

function stop() {
  stopSource();
  state.playing = false;
  playBtn.textContent = "▶ ROLL TAPE";
  meter.l = meter.r = 0;
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
  bufferIsUpload = true;
  currentFileName = f.name.replace(/\.[^.]+$/, "") || "audio";
  downloadBtn.hidden = false;
  start();
};

/* ============================== download your master ==============================
   Renders the uploaded file through the full signal path offline, with the
   knobs exactly where you left them, and hands back a 16-bit WAV. */
const downloadBtn = document.getElementById("downloadBtn");

function encodeWav(buffer) {
  const ch = Math.min(2, buffer.numberOfChannels);
  const len = buffer.length;
  const bytes = 44 + len * ch * 2;
  const view = new DataView(new ArrayBuffer(bytes));
  const w = (o, str) => { for (let i = 0; i < str.length; i++) view.setUint8(o + i, str.charCodeAt(i)); };
  w(0, "RIFF"); view.setUint32(4, bytes - 8, true); w(8, "WAVE");
  w(12, "fmt "); view.setUint32(16, 16, true);
  view.setUint16(20, 1, true); view.setUint16(22, ch, true);
  view.setUint32(24, buffer.sampleRate, true);
  view.setUint32(28, buffer.sampleRate * ch * 2, true);
  view.setUint16(32, ch * 2, true); view.setUint16(34, 16, true);
  w(36, "data"); view.setUint32(40, len * ch * 2, true);
  const chans = [];
  for (let c = 0; c < ch; c++) chans.push(buffer.getChannelData(Math.min(c, buffer.numberOfChannels - 1)));
  let o = 44;
  for (let i = 0; i < len; i++)
    for (let c = 0; c < ch; c++) {
      const s = Math.max(-1, Math.min(1, chans[c][i]));
      view.setInt16(o, s < 0 ? s * 0x8000 : s * 0x7fff, true);
      o += 2;
    }
  return new Blob([view], { type: "audio/wav" });
}

async function renderAndDownload() {
  if (!currentBuffer || !bufferIsUpload) return;
  downloadBtn.disabled = true;
  const label = downloadBtn.textContent;
  downloadBtn.textContent = "PRINTING TAPE…";
  try {
    const src = currentBuffer;
    const tail = Math.round(0.3 * src.sampleRate); // let print-through breathe out
    const off = new OfflineAudioContext(2, src.length + tail, src.sampleRate);
    await off.audioWorklet.addModule("kos-processor.js");
    const params = {};
    for (const id of ["input", "shame", "hiss", "age", "blend", "output", "flange",
                      "bypass", "tape", "print", "env", "extreme"])
      params[id] = state[id];
    const node = new AudioWorkletNode(off, "kos", { outputChannelCount: [2], processorOptions: { params } });
    const s = off.createBufferSource();
    s.buffer = src;
    s.connect(node);
    node.connect(off.destination);
    s.start();
    const rendered = await off.startRendering();
    const url = URL.createObjectURL(encodeWav(rendered));
    const a = document.createElement("a");
    a.href = url;
    a.download = currentFileName + "_KissOfShame.wav";
    a.click();
    setTimeout(() => URL.revokeObjectURL(url), 10000);
  } catch (err) {
    console.error(err);
    alert("Render failed: " + err.message);
  }
  downloadBtn.textContent = label;
  downloadBtn.disabled = false;
}
downloadBtn.onclick = renderAndDownload;
