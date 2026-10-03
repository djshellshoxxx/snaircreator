export function encodeWav(samples, sampleRate = 48000, bitDepth = 16) {
  const depth = bitDepth === 24 ? 24 : 16;
  const bytesPerSample = depth / 8;
  const dataSize = samples.length * bytesPerSample;
  const buffer = new ArrayBuffer(44 + dataSize);
  const view = new DataView(buffer);
  const writeText = (offset, text) => { for (let i = 0; i < text.length; i++) view.setUint8(offset + i, text.charCodeAt(i)); };
  writeText(0, 'RIFF'); view.setUint32(4, 36 + dataSize, true); writeText(8, 'WAVE');
  writeText(12, 'fmt '); view.setUint32(16, 16, true); view.setUint16(20, 1, true); view.setUint16(22, 1, true);
  view.setUint32(24, sampleRate, true); view.setUint32(28, sampleRate * bytesPerSample, true);
  view.setUint16(32, bytesPerSample, true); view.setUint16(34, depth, true); writeText(36, 'data'); view.setUint32(40, dataSize, true);
  let o = 44;
  for (const raw of samples) {
    const x = Math.max(-1, Math.min(1, Number.isFinite(raw) ? raw : 0));
    if (depth === 16) { view.setInt16(o, x < 0 ? Math.round(x * 32768) : Math.round(x * 32767), true); o += 2; }
    else { let v = x < 0 ? Math.round(x * 8388608) : Math.round(x * 8388607); if (v < 0) v += 0x1000000; view.setUint8(o++, v & 255); view.setUint8(o++, (v >> 8) & 255); view.setUint8(o++, (v >> 16) & 255); }
  }
  return buffer;
}
