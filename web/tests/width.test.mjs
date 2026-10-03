import test from 'node:test';
import assert from 'node:assert/strict';
import { render } from '../engine.js';

const source = Float32Array.from({length: 1200}, (_, i) => i === 20 ? 1 : Math.sin(i * .17) * Math.exp(-i / 300));

test('width changes clap micro-timing texture', () => {
  const a = render(source, 48000, {mode:'clap', seed:77, width:0, output_ms:180});
  const b = render(source, 48000, {mode:'clap', seed:77, width:1, output_ms:180});
  const delta = a.reduce((sum, x, i) => sum + Math.abs(x - b[i]), 0);
  assert.ok(delta > 0.001);
});
