// Normie Display — mobile web UI
#pragma once

const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>Normie Display</title>
<style>
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
body{font-family:"Courier New",Courier,monospace;
background:#1A1B1C;color:#E3E5E4;min-height:100vh;min-height:100dvh;
display:flex;flex-direction:column;align-items:center;padding:2rem 1rem;
-webkit-tap-highlight-color:transparent;font-size:13px;line-height:1.6}

h1{font-size:1.4rem;font-weight:700;letter-spacing:.15em;
margin-bottom:2rem;text-align:center;text-transform:uppercase}

.tabs{display:flex;width:100%;max-width:360px;margin-bottom:1.5rem}
.tab{flex:1;padding:.6rem;text-align:center;font-family:"Courier New",monospace;
font-size:11px;letter-spacing:.1em;text-transform:uppercase;cursor:pointer;
border:1px solid #48494B;background:transparent;color:#48494B;transition:all .15s}
.tab.active{background:#E3E5E4;color:#1A1B1C;border-color:#E3E5E4}
.tab:first-child{border-right:none}

.panel{display:none;width:100%;max-width:360px;flex-direction:column;align-items:center}
.panel.active{display:flex}

.input-group{display:flex;flex-direction:column;gap:1px;width:100%;max-width:360px}
.input-group input[type=number]{width:100%;padding:.75rem;border:1px solid #48494B;
background:transparent;color:#E3E5E4;font-family:"Courier New",monospace;font-size:14px;
border-radius:0;outline:none;-moz-appearance:textfield;text-align:center;letter-spacing:.1em}
.input-group input::-webkit-outer-spin-button,
.input-group input::-webkit-inner-spin-button{-webkit-appearance:none}
.input-group input::placeholder{color:#48494B}
.input-group input:focus{border-color:#E3E5E4}

button{width:100%;max-width:360px;padding:.75rem 1.25rem;border:1px solid #E3E5E4;border-radius:0;
font-family:"Courier New",monospace;font-size:12px;font-weight:400;
letter-spacing:.08em;text-transform:uppercase;cursor:pointer;
background:#E3E5E4;color:#1A1B1C;
transition:opacity .15s}
button:active{opacity:.7}
button:disabled{opacity:.3;cursor:not-allowed}
button.outline{background:transparent;color:#E3E5E4}

.upload-zone{width:100%;max-width:360px;border:1px dashed #48494B;padding:2rem 1rem;
text-align:center;cursor:pointer;transition:border-color .15s;margin-bottom:1px}
.upload-zone:hover,.upload-zone.dragover{border-color:#E3E5E4}
.upload-zone p{font-size:11px;color:#48494B;letter-spacing:.05em;text-transform:uppercase}
.upload-zone input{display:none}

.preview-wrap{margin-top:1.5rem;display:flex;justify-content:center}
.preview-frame{width:210px;height:210px;background:#E3E5E4;
display:flex;align-items:center;justify-content:center;border:1px solid #48494B}
.preview-frame img,.preview-frame canvas{width:200px;height:200px;image-rendering:pixelated;display:none}
.preview-frame .ph{color:#48494B;font-family:"Courier New",monospace;font-size:11px}

.ctrl-group{width:100%;max-width:360px;margin-top:.75rem;display:none}
.ctrl-group label{font-size:10px;color:#48494B;letter-spacing:.05em;text-transform:uppercase;
display:flex;justify-content:space-between;align-items:center}
.ctrl-group input[type=range]{width:100%;margin-top:4px;accent-color:#E3E5E4}
.ctrl-group select{width:100%;margin-top:4px;padding:.5rem;border:1px solid #48494B;
background:#1A1B1C;color:#E3E5E4;font-family:"Courier New",monospace;font-size:11px;
letter-spacing:.05em;text-transform:uppercase;outline:none;border-radius:0}
.ctrl-group select:focus{border-color:#E3E5E4}

.status{margin-top:1rem;text-align:center;font-size:11px;min-height:1.4rem;
letter-spacing:.05em;text-transform:uppercase}
.status.ok{color:#E3E5E4}.status.err{color:#E3E5E4;opacity:.5}.status.busy{color:#48494B}
.sp{display:inline-block;width:10px;height:10px;border:1px solid #48494B;
border-top-color:#E3E5E4;border-radius:0;animation:s .8s linear infinite;
vertical-align:middle;margin-right:6px}
@keyframes s{to{transform:rotate(360deg)}}

.tagline{margin-top:2rem;font-size:11px;color:#48494B;
letter-spacing:.08em;text-align:center;text-transform:uppercase}
</style>
</head>
<body>
<h1>Normie Display</h1>

<div class="tabs">
<div class="tab active" data-panel="normie-panel">Normie</div>
<div class="tab" data-panel="upload-panel">Upload</div>
</div>

<div class="panel active" id="normie-panel">
<div class="input-group">
<input type="number" id="nid" min="0" max="9999" placeholder="normie id (0 - 9999)">
</div>
<button id="btnN" style="margin-top:1px">Display</button>
</div>

<div class="panel" id="upload-panel">
<div class="upload-zone" id="dropZone">
<input type="file" id="fileIn" accept="image/*">
<p>tap to select image<br>or drag & drop</p>
</div>
<div class="ctrl-group" id="ctrlGrp">
<label>mode</label>
<select id="ditherMode">
<option value="threshold">Threshold</option>
<option value="floyd">Floyd-Steinberg</option>
<option value="ordered">Ordered (Bayer)</option>
</select>
<label style="margin-top:.5rem">threshold <span id="threshVal">128</span></label>
<input type="range" id="threshSlider" min="1" max="255" value="128">
</div>
<button id="btnU" style="margin-top:1px" disabled>Upload & Display</button>
</div>

<div class="preview-wrap">
<div class="preview-frame">
<img id="pimg" alt="normie preview">
<canvas id="ucanvas" width="200" height="200"></canvas>
<span class="ph" id="ph">200 x 200</span>
</div>
</div>

<div class="status" id="st"></div>

<div class="tagline">Built by Normies, for Normies</div>

<script>
const btnN=document.getElementById('btnN'),
btnU=document.getElementById('btnU'),
nid=document.getElementById('nid'),
st=document.getElementById('st'),
pimg=document.getElementById('pimg'),
ph=document.getElementById('ph'),
fileIn=document.getElementById('fileIn'),
dropZone=document.getElementById('dropZone'),
ucanvas=document.getElementById('ucanvas'),
ctrlGrp=document.getElementById('ctrlGrp'),
threshSlider=document.getElementById('threshSlider'),
threshVal=document.getElementById('threshVal'),
ditherMode=document.getElementById('ditherMode');
let busy=false,srcImage=null;

function status(t,c){st.className='status '+(c||'');st.innerHTML=t}
function loading(t){status('<span class="sp"></span>'+t,'busy');btnN.disabled=true;btnU.disabled=true;busy=true}
function done(){btnN.disabled=false;btnU.disabled=!srcImage;busy=false}

document.querySelectorAll('.tab').forEach(function(tab){
  tab.addEventListener('click',function(){
    document.querySelectorAll('.tab').forEach(function(t){t.classList.remove('active')});
    document.querySelectorAll('.panel').forEach(function(p){p.classList.remove('active')});
    tab.classList.add('active');
    document.getElementById(tab.dataset.panel).classList.add('active');
  });
});

btnN.addEventListener('click',async function(){
  if(busy)return;
  const id=parseInt(nid.value);
  if(isNaN(id)||id<0||id>9999){status('enter id 0 - 9999','err');return}
  loading('displaying normie #'+id+'...');
  try{
    const r=await fetch('/normie?id='+id);
    const j=await r.json();
    status(j.message||j.error,j.ok?'ok':'err');
  }catch(e){status('connection error','err')}
  done();
});

nid.addEventListener('keydown',function(e){
  if(e.key==='Enter'){e.preventDefault();btnN.click()}
});

dropZone.addEventListener('click',function(){fileIn.click()});
dropZone.addEventListener('dragover',function(e){e.preventDefault();dropZone.classList.add('dragover')});
dropZone.addEventListener('dragleave',function(){dropZone.classList.remove('dragover')});
dropZone.addEventListener('drop',function(e){
  e.preventDefault();dropZone.classList.remove('dragover');
  if(e.dataTransfer.files.length)loadFile(e.dataTransfer.files[0]);
});
fileIn.addEventListener('change',function(){if(fileIn.files.length)loadFile(fileIn.files[0])});

function loadFile(f){
  if(!f.type.startsWith('image/')){status('not an image file','err');return}
  const reader=new FileReader();
  reader.onload=function(e){
    const img=new Image();
    img.onload=function(){
      srcImage=img;
      ctrlGrp.style.display='block';
      renderPreview();
      btnU.disabled=false;
      status('image loaded - adjust threshold & upload','ok');
    };
    img.src=e.target.result;
  };
  reader.readAsDataURL(f);
}

const BAYER4=[
  [ 0, 8, 2,10],
  [12, 4,14, 6],
  [ 3,11, 1, 9],
  [15, 7,13, 5]
];

function getGrayscale(id){
  const g=new Float32Array(200*200);
  for(let i=0;i<g.length;i++){
    const p=i*4;
    g[i]=0.299*id.data[p]+0.587*id.data[p+1]+0.114*id.data[p+2];
  }
  return g;
}

function ditherThreshold(gray,thr){
  const out=new Uint8Array(200*200);
  for(let i=0;i<gray.length;i++) out[i]=gray[i]>=thr?255:0;
  return out;
}

function ditherFloydSteinberg(gray,thr){
  const err=Float32Array.from(gray);
  const out=new Uint8Array(200*200);
  const W=200,H=200;
  for(let y=0;y<H;y++){
    for(let x=0;x<W;x++){
      const i=y*W+x;
      const old=err[i];
      const nv=old>=thr?255:0;
      out[i]=nv;
      const e=old-nv;
      if(x+1<W)       err[i+1]      +=e*7/16;
      if(y+1<H){
        if(x-1>=0)    err[i+W-1]    +=e*3/16;
                       err[i+W]      +=e*5/16;
        if(x+1<W)     err[i+W+1]    +=e*1/16;
      }
    }
  }
  return out;
}

function ditherOrdered(gray,thr){
  const out=new Uint8Array(200*200);
  const W=200;
  const bias=(thr-128)/255;
  for(let i=0;i<gray.length;i++){
    const x=i%W,y=(i/W)|0;
    const b=(BAYER4[y%4][x%4]/16-0.5)+bias;
    out[i]=(gray[i]/255+b)>=0.5?255:0;
  }
  return out;
}

function renderPreview(){
  if(!srcImage)return;
  const ctx=ucanvas.getContext('2d');
  ctx.fillStyle='#E3E5E4';
  ctx.fillRect(0,0,200,200);
  const s=Math.min(200/srcImage.width,200/srcImage.height);
  const w=srcImage.width*s,h=srcImage.height*s;
  const ox=(200-w)/2,oy=(200-h)/2;
  ctx.drawImage(srcImage,ox,oy,w,h);
  const id=ctx.getImageData(0,0,200,200);
  const thr=parseInt(threshSlider.value);
  const gray=getGrayscale(id);
  const mode=ditherMode.value;
  let bw;
  if(mode==='floyd') bw=ditherFloydSteinberg(gray,thr);
  else if(mode==='ordered') bw=ditherOrdered(gray,thr);
  else bw=ditherThreshold(gray,thr);
  for(let i=0;i<bw.length;i++){
    const v=bw[i]?0xE3:0x48;
    const p=i*4;
    id.data[p]=v;id.data[p+1]=v;id.data[p+2]=v;id.data[p+3]=255;
  }
  ctx.putImageData(id,0,0);
  pimg.style.display='none';
  ucanvas.style.display='block';
  ph.style.display='none';
}

threshSlider.addEventListener('input',function(){
  threshVal.textContent=threshSlider.value;
  renderPreview();
});
ditherMode.addEventListener('change',renderPreview);

function buildBitmap(){
  const ctx=ucanvas.getContext('2d');
  const id=ctx.getImageData(0,0,200,200);
  const buf=new Uint8Array(5000);
  for(let y=0;y<200;y++){
    for(let x=0;x<200;x++){
      const pi=(y*200+x)*4;
      const bright=id.data[pi]>0x80;
      if(!bright){
        const byteIdx=Math.floor((y*200+x)/8);
        const bitIdx=7-((y*200+x)%8);
        buf[byteIdx]|=(1<<bitIdx);
      }
    }
  }
  return buf;
}

btnU.addEventListener('click',async function(){
  if(busy||!srcImage)return;
  loading('uploading image...');
  try{
    const bitmap=buildBitmap();
    const blob=new Blob([bitmap],{type:'application/octet-stream'});
    const fd=new FormData();
    fd.append('file',blob,'image.bin');
    const r=await fetch('/upload',{method:'POST',body:fd});
    const j=await r.json();
    status(j.message||j.error,j.ok?'ok':'err');
  }catch(e){status('connection error','err')}
  done();
});
</script>
</body>
</html>
)rawhtml";
