import test from 'node:test';
import assert from 'node:assert/strict';
import { encodeWav } from '../wav.js';

test('encodes mono PCM WAV with a valid RIFF header', () => {
  const data = encodeWav(Float32Array.from([0, 0.5, -0.5, 1, -1]), 48000, 16);
  const view = new DataView(data);
  const text = (o, n) => String.fromCharCode(...new Uint8Array(data, o, n));
  assert.equal(text(0, 4), 'RIFF');
  assert.equal(text(8, 4), 'WAVE');
  assert.equal(view.getUint16(22, true), 1);
  assert.equal(view.getUint32(24, true), 48000);
  assert.equal(view.getUint16(34, true), 16);
  assert.equal(text(36, 4), 'data');
  assert.equal(view.getUint32(40, true), 10);
});
