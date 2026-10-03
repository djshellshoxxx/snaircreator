import { DEFAULTS, analyze, render, sanitizeParams, randomizeParams, mutateParams } from './engine.js';
import { encodeWav } from './wav.js';

const $ = s => document.querySelector(s);
const status = $('#status');
const controls = $('#controls');
const detailDefs = [
  ['character','Character',0,1,.01],['body','Body',0,1,.01],['body_freq_hz','Body Hz',70,450,1],['crack','Crack',0,1,.01],
  ['noise','Noise',0,1,.01],['tail_ms','Tail ms',20,1200,1],['clap_count','Clap count',2,6,1],['clap_spread_ms','Clap spread ms',8,35,.5],
  ['width','Width',0,1,.01],['drive_db','Drive dB',0,18,.1],['tone','Tone',-1,1,.01],['pitch_st','Pitch st',-24,24,1],
  ['trim_db','Trim dB',-24,12,.1],['output_ms','Output ms',40,2000,1]
];
for (const [id,label,min,max,step] of detailDefs) controls.insertAdjacentHTML('beforeend', `<label>${label}<input data-param="${id}" type="range" min="${min}" max="${max}" step="${step}"><output data-out="${id}"></output></label>`);

let ctx, sourceSamples, sourceRate = 48000, sourceName = 'snair', params = { ...DEFAULTS }, rendered, undoParams = null;
const setStatus = (m, error=false) => { status.textContent=m; status.classList.toggle('error',error); };

function syncUI(){
  document.querySelectorAll('[data-mode]').forEach(b=>b.classList.toggle('active',b.dataset.mode===params.mode));
  document.querySelectorAll('[data-param]').forEach(el=>{el.value=params[el.dataset.param];$(`[data-out="${el.dataset.param}"]`).textContent=Number(params[el.dataset.param]).toFixed(el.step.includes('.')?1:0);});
  $('#punch').value=(params.body+params.crack)/2; $('#snap').value=(params.crack+params.noise)/2; $('#dirt').value=params.drive_db/18; $('#size').value=params.tail_ms/1200;
}
function updateRender(){ if(!sourceSamples)return; rendered=render(sourceSamples,sourceRate,params); setStatus(`${params.mode.toUpperCase()} rendered • seed ${params.seed} • ${Math.round(params.output_ms)} ms`); }
function macros(){
  const punch=+$('#punch').value,snap=+$('#snap').value,dirt=+$('#dirt').value,size=+$('#size').value;
  params=sanitizeParams({...params,body:punch,crack:Math.min(1,punch*.65+snap*.55),noise:snap,drive_db:dirt*18,tail_ms:40+size*1160}); syncUI(); updateRender();
}

async function loadAudioFile(file){
  try{
    ctx ??= new AudioContext(); const decoded=await ctx.decodeAudioData(await file.arrayBuffer());
    const n=decoded.length, mono=new Float32Array(n); for(let c=0;c<decoded.numberOfChannels;c++){const ch=decoded.getChannelData(c);for(let i=0;i<n;i++)mono[i]+=ch[i]/decoded.numberOfChannels;}
    sourceSamples=mono; sourceRate=decoded.sampleRate; sourceName=file.name.replace(/\.[^.]+$/,'');
    const a=analyze(mono,sourceRate); params=sanitizeParams({...params,body_freq_hz:Math.round(a.bodyFreqHint)}); drawWave(mono);
    $('#analysis').textContent=`${file.name} • ${(n/sourceRate).toFixed(2)} s • ${sourceRate} Hz • RMS ${a.rms.toFixed(3)} • peak ${a.peak.toFixed(3)} • body hint ${Math.round(a.bodyFreqHint)} Hz`;
    syncUI(); updateRender();
  }catch(e){setStatus(`Could not decode this audio file in this browser: ${e.message}`,true);}
}
function drawWave(x){const c=$('#wave'),g=c.getContext('2d');g.clearRect(0,0,c.width,c.height);g.strokeStyle='#4de4ff';g.beginPath();const step=Math.max(1,Math.floor(x.length/c.width));for(let px=0;px<c.width;px++){let peak=0;for(let i=px*step;i<Math.min(x.length,(px+1)*step);i++)peak=Math.max(peak,Math.abs(x[i]));const y=(1-peak)*c.height/2;g.moveTo(px,y);g.lineTo(px,c.height-y);}g.stroke();}
async function preview(){if(!rendered){setStatus('Load an audio file first.',true);return;}ctx??=new AudioContext();await ctx.resume();const b=ctx.createBuffer(1,rendered.length,sourceRate);b.copyToChannel(rendered,0);const s=ctx.createBufferSource();s.buffer=b;s.connect(ctx.destination);s.start();}
function download(blob,name){const a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=name;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);}

$('#file').addEventListener('change',e=>e.target.files[0]&&loadAudioFile(e.target.files[0]));
const drop=$('#drop');['dragenter','dragover'].forEach(ev=>drop.addEventListener(ev,e=>{e.preventDefault();drop.classList.add('drag')}));['dragleave','drop'].forEach(ev=>drop.addEventListener(ev,e=>{e.preventDefault();drop.classList.remove('drag')}));drop.addEventListener('drop',e=>e.dataTransfer.files[0]&&loadAudioFile(e.dataTransfer.files[0]));
document.querySelectorAll('[data-mode]').forEach(b=>b.addEventListener('click',()=>{params.mode=b.dataset.mode;syncUI();updateRender();}));
document.querySelectorAll('[data-param]').forEach(el=>el.addEventListener('input',()=>{params=sanitizeParams({...params,[el.dataset.param]:+el.value});syncUI();updateRender();}));
['punch','snap','dirt','size'].forEach(id=>$(`#${id}`).addEventListener('input',macros));
$('#preview').addEventListener('click',preview);
$('#randomize').addEventListener('click',()=>{undoParams={...params};params=randomizeParams(params);$('#undo').disabled=false;syncUI();updateRender();});
$('#mutate').addEventListener('click',()=>{undoParams={...params};params=mutateParams(params);$('#undo').disabled=false;syncUI();updateRender();});
$('#undo').addEventListener('click',()=>{if(undoParams){params=undoParams;undoParams=null;$('#undo').disabled=true;syncUI();updateRender();}});
$('#reset').addEventListener('click',()=>{params={...DEFAULTS};undoParams=null;$('#undo').disabled=true;syncUI();updateRender();});
$('#export').addEventListener('click',()=>{if(!rendered){setStatus('Load an audio file first.',true);return;}download(new Blob([encodeWav(rendered,sourceRate,24)],{type:'audio/wav'}),`${sourceName}-${params.mode}-seed${params.seed}.wav`);});
$('#savePreset').addEventListener('click',()=>download(new Blob([JSON.stringify({schema:1,product:'SnairCreator',parameters:params,seed:params.seed},null,2)],{type:'application/json'}),`snaircreator-${params.mode}-${params.seed}.json`));
$('#loadPreset').addEventListener('change',async e=>{try{const j=JSON.parse(await e.target.files[0].text());if(j.product!=='SnairCreator'||j.schema!==1)throw new Error('unsupported preset');params=sanitizeParams(j.parameters||{});syncUI();updateRender();}catch(err){setStatus(`Preset error: ${err.message}`,true);}});
syncUI();
