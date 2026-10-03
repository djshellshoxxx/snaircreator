import test from 'node:test';
import assert from 'node:assert/strict';
import { analyze, render, sanitizeParams } from '../engine.js';

function impulse(length = 4800) {
  const x = new Float32Array(length);
  x[20] = 1;
  for (let i = 21; i < Math.min(length, 700); i++) x[i] = Math.exp(-(i - 20) / 100) * ((i & 1) ? 0.35 : -0.35);
  return x;
}

function differs(a, b) {
  if (a.length !== b.length) return true;
  let delta = 0;
  for (let i = 0; i < a.length; i++) delta += Math.abs(a[i] - b[i]);
  return delta > 0.001;
}

test('identical input seed and params render identically', () => {
  const source = impulse();
  const p = { mode: 'snare', seed: 42, output_ms: 180, normalize: true };
  assert.deepEqual(Array.from(render(source, 48000, p)), Array.from(render(source, 48000, p)));
});

test('different seeds change the rendered hit', () => {
  const source = impulse();
  assert.equal(differs(render(source, 48000, { seed: 1 }), render(source, 48000, { seed: 2 })), true);
});

test('silent input analysis and render remain finite', () => {
  const source = new Float32Array(256);
  const a = analyze(source, 48000);
  assert.equal(Number.isFinite(a.rms), true);
  const out = render(source, 48000, { mode: 'hybrid', seed: 7 });
  assert.equal(out.length > 0, true);
  assert.equal(Array.from(out).every(Number.isFinite), true);
});

test('very short input renders without reading outside source', () => {
  const out = render(Float32Array.from([0.2, -0.2]), 48000, { mode: 'clap', seed: 3, clap_count: 6, clap_spread_ms: 35 });
  assert.equal(out.length > 2, true);
  assert.equal(Array.from(out).every(Number.isFinite), true);
});

test('parameters are clamped at the engine boundary', () => {
  const p = sanitizeParams({ body_freq_hz: -50, clap_count: 99, clap_spread_ms: -1, drive_db: 999, tone: 4, output_ms: 99999 });
  assert.equal(p.body_freq_hz, 70);
  assert.equal(p.clap_count, 6);
  assert.equal(p.clap_spread_ms, 8);
  assert.equal(p.drive_db, 18);
  assert.equal(p.tone, 1);
  assert.equal(p.output_ms, 2000);
});

test('normalized output never exceeds full scale', () => {
  const out = render(impulse(), 48000, { mode: 'hybrid', seed: 99, drive_db: 18, normalize: true });
  const peak = out.reduce((m, v) => Math.max(m, Math.abs(v)), 0);
  assert.equal(peak <= 1.0, true);
});
