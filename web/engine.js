export const DEFAULTS = Object.freeze({
  mode:'snare', seed:1, character:.72, body:.68, body_freq_hz:185, crack:.72, noise:.62,
  tail_ms:260, clap_count:4, clap_spread_ms:18, width:.35, drive_db:4, tone:0,
  pitch_st:0, trim_db:0, normalize:true, output_ms:420
});

const clamp=(v,lo,hi)=>Math.min(hi,Math.max(lo,Number.isFinite(Number(v))?Number(v):lo));
const clampInt=(v,lo,hi)=>Math.round(clamp(v,lo,hi));
const finite=v=>Number.isFinite(v)?v:0;

export function sanitizeParams(input={}){
  const p={...DEFAULTS,...input};
  p.mode=['snare','clap','hybrid'].includes(p.mode)?p.mode:'snare';
  p.seed=clampInt(p.seed,0,2147483647); p.character=clamp(p.character,0,1); p.body=clamp(p.body,0,1);
  p.body_freq_hz=clamp(p.body_freq_hz,70,450); p.crack=clamp(p.crack,0,1); p.noise=clamp(p.noise,0,1);
  p.tail_ms=clamp(p.tail_ms,20,1200); p.clap_count=clampInt(p.clap_count,2,6); p.clap_spread_ms=clamp(p.clap_spread_ms,8,35);
  p.width=clamp(p.width,0,1); p.drive_db=clamp(p.drive_db,0,18); p.tone=clamp(p.tone,-1,1);
  p.pitch_st=clamp(p.pitch_st,-24,24); p.trim_db=clamp(p.trim_db,-24,12); p.normalize=Boolean(p.normalize); p.output_ms=clamp(p.output_ms,40,2000);
  return p;
}

export function analyze(samples,sampleRate){
  const n=Math.max(0,samples?.length??0);
  if(!n||!Number.isFinite(sampleRate)||sampleRate<=0) return {frames:0,duration:0,peak:0,rms:0,zeroCrossingRate:0,centroidHint:0,onsetIndex:0,bodyFreqHint:185};
  let ss=0,peak=0,zc=0,onset=0,strongest=-1,weighted=0,weight=0,prev=finite(samples[0]);
  for(let i=0;i<n;i++){
    const x=finite(samples[i]); ss+=x*x; peak=Math.max(peak,Math.abs(x));
    if(i){if((x>=0)!=(prev>=0))zc++;const d=Math.abs(x-prev);if(d>strongest){strongest=d;onset=i;}weighted+=d*i;weight+=d;} prev=x;
  }
  const zcr=zc/Math.max(1,n-1),rough=90+Math.min(1,zcr*18)*260;
  return {frames:n,duration:n/sampleRate,peak,rms:Math.sqrt(ss/n),zeroCrossingRate:zcr,centroidHint:weight?(weighted/weight)/Math.max(1,n-1):0,onsetIndex:onset,bodyFreqHint:clamp(rough,110,320)};
}

function makeRng(seed){let s=(seed>>>0)||0x6d2b79f5;return()=>{s=(s+0x6d2b79f5)>>>0;let t=s;t=Math.imul(t^(t>>>15),t|1);t^=t+Math.imul(t^(t>>>7),t|61);return((t^(t>>>14))>>>0)/4294967296;};}
function sourceAt(x,index,pitch=1){if(!x.length)return 0;const pos=Math.abs(index*pitch)%x.length,i0=Math.floor(pos),i1=(i0+1)%x.length,f=pos-i0;return finite(x[i0])*(1-f)+finite(x[i1])*f;}

export function render(samples,sampleRate,inputParams={}){
  const p=sanitizeParams(inputParams),sr=Number.isFinite(sampleRate)&&sampleRate>1000?sampleRate:48000,src=samples?.length?samples:new Float32Array([0]),a=analyze(src,sr);
  const rng=makeRng((p.seed^Math.floor(a.rms*1e9)^a.onsetIndex)>>>0),length=Math.max(1,Math.round(sr*p.output_ms/1000)),out=new Float32Array(length);
  const pitch=2**(p.pitch_st/12),bodyHz=inputParams.body_freq_hz==null?a.bodyFreqHint:p.body_freq_hz,attack=Math.max(8,Math.round(sr*.018)),noiseDecay=Math.max(1,sr*p.tail_ms/1000),spacing=sr*p.clap_spread_ms/1000,drive=10**(p.drive_db/20),bright=.35+(p.tone+1)*.325,widthJitter=.08+p.width*.48;
  let lastNoise=0;
  for(let i=0;i<length;i++){
    const t=i/sr,bodyEnv=Math.exp(-t/(.045+p.body*.22)),source=sourceAt(src,a.onsetIndex+i,pitch),transient=i<attack?Math.exp(-i/Math.max(2,attack*.22)):0;
    const body=Math.sin(2*Math.PI*bodyHz*t)*bodyEnv*p.body*.55,crack=source*transient*p.crack*(.45+.55*p.character),white=rng()*2-1,hp=white-lastNoise*(1-bright);lastNoise=white;
    const noise=hp*Math.exp(-i/noiseDecay)*p.noise*.38; let clap=0;
    if(p.mode!=='snare'){
      for(let c=0;c<p.clap_count;c++){
        const jitter=(rng()-.5)*spacing*widthJitter,local=i-(c*spacing+jitter);
        if(local>=0){const env=Math.exp(-local/Math.max(1,sr*(.010+c*.002)));if(env>.001){const texture=sourceAt(src,a.onsetIndex+local*(1+c*.013),pitch);const polarity=(c&1)&&p.width>.5?-1:1;clap+=(texture*p.character+(rng()*2-1)*(1-p.character))*env*.24*polarity;}}
      }
      const tailStart=(p.clap_count-1)*spacing;if(i>=tailStart)clap+=hp*Math.exp(-(i-tailStart)/Math.max(1,noiseDecay*.75))*p.noise*(.18+.08*p.width);
    }
    const raw=p.mode==='clap'?clap+crack*.35:p.mode==='hybrid'?body+crack+noise+clap*.8:body+crack+noise;
    out[i]=Math.tanh(raw*drive);
  }
  const trim=10**(p.trim_db/20);let peak=0;for(let i=0;i<out.length;i++){out[i]=Math.max(-1,Math.min(1,out[i]*trim));peak=Math.max(peak,Math.abs(out[i]));}
  if(p.normalize&&peak>0){const gain=Math.min(1/peak,8)*.98;for(let i=0;i<out.length;i++)out[i]=Math.max(-1,Math.min(1,out[i]*gain));}
  return out;
}

export function randomizeParams(current=DEFAULTS,seed=Date.now()&0x7fffffff){const r=makeRng(seed);return sanitizeParams({...current,seed,body:.35+r()*.6,crack:.35+r()*.65,noise:.3+r()*.68,tail_ms:90+r()*610,clap_count:2+Math.floor(r()*5),clap_spread_ms:9+r()*24,width:r(),drive_db:r()*13,tone:r()*1.7-.85,pitch_st:Math.round(r()*24-12),character:.35+r()*.65});}
export function mutateParams(current,seed=((current?.seed??1)+1)&0x7fffffff){const r=makeRng(seed),factor=()=>.82+r()*.36;return sanitizeParams({...current,seed,body:current.body*factor(),crack:current.crack*factor(),noise:current.noise*factor(),tail_ms:current.tail_ms*factor(),clap_spread_ms:current.clap_spread_ms*factor(),width:current.width+(r()-.5)*.2,drive_db:current.drive_db*factor(),tone:current.tone+(r()-.5)*.25});}
