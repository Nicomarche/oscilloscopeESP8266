/*
 * WiFi Oscilloscope – ESP8266
 * Rev 3.0 · V/DIV, calibración FS y TIME/DIV hasta Tx4
 *
 * TIME/DIV (Txn):
 *   Tx1 ➜ 100 % del buffer actual
 *   Tx2 ➜ 50 % del buffer actual  + 50 % del buffer anterior
 *   Tx3 ➜ ⅓  del buffer actual   + ⅓  del buffer anterior   + ⅓  del buffer −2
 *   Tx4 ➜ ¼  del buffer actual   + ¼  del buffer anterior   + ¼  del buffer −2 + ¼ del buffer −3
 *
 * Cada fragmento se obtiene decimando (tomando 1 de n muestras) y luego concatenando
 * para mantener siempre el mismo número total de puntos en pantalla.
 */

#ifndef webapp_H_
#define webapp_H_

#include <ESP8266WebServer.h>
extern ESP8266WebServer Server;

void webapp()
{
  static const char page[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="es"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Osciloscopio WiFi - WebSocket</title>
<style>
 body{margin:0;background:#8F8;font-family:sans-serif;}
 .inner{background:#BFF;min-height:400px;padding:10px;}
 canvas{border:1px solid #d3d3d3;background:#FFF;display:block;
        margin:10px auto;width:100%;max-width:800px;height:auto;}
 .tools{margin-top:10px;text-align:center;}
 button{margin:5px;padding:8px 12px;background:#006;color:#FFF;border:0;
        border-radius:4px;cursor:pointer;}
 .cal{margin:10px;text-align:center;font-size:14px;}
 .cal input{width:70px;text-align:right;padding:4px;}
</style></head><body><div class="inner">

<canvas id="scope" width="800" height="400"></canvas>
<canvas id="info"  width="800" height="50"></canvas>

<div class="cal">
  Full scale (V): <input id="vmax" type="number" value="10" min="0.1" step="0.1">
  <span id="wsStatus" style="margin-left:20px; color:#666;">WebSocket: Connecting...</span>
</div>

<div class="tools">
 <button id="h">HOLD</button><button id="c">CLEAR</button><button id="r">RESET</button>
 <button id="d">DIGITAL</button>
 <button id="fu">FREQ -</button><button id="fd">FREQ +</button>
 <button id="vu">V/DIV +</button><button id="vd">V/DIV −</button>
 <button id="tu">T/DIV +</button><button id="td">T/DIV −</button>
</div></div>

<script>
/* ═════════ CONFIG ═════════ */
let adcVmax = 10;                                       // V a ADC=1023
const vSteps = [0.01,0.02,0.05,0.1,0.2,0.5,1,2,5];
const tSteps = [1,2,3,4];                               // Tx1-Tx4

let hold=false, digital=false, speed=0;
let vIdx=5, vDiv=vSteps[vIdx];
let tIdx=0;                                             // índice T/DIV
let buf=null, busy=false;
const prev = [];                                        // cola de buffers (máx 3)

let cvs, ctx, inf, ictx;
let websocket = null;

/* ═════════ UTIL ═════════ */
const $ = id => document.getElementById(id);
const setBg = (el,on)=>el.style.background=on?'#003':'#006';

/* ═════════ GRILLA ═════════ */
function grid(){
  ctx.fillStyle='#FFF'; ctx.fillRect(0,0,cvs.width,cvs.height);
  ctx.strokeStyle='#CCC'; ctx.beginPath();
  for(let i=0;i<=10;i++){
    const y=i*cvs.height/10, x=i*cvs.width/10;
    ctx.moveTo(0,y); ctx.lineTo(cvs.width,y);           // horizontales
    ctx.moveTo(x,0); ctx.lineTo(x,cvs.height);          // verticales
  }
  ctx.stroke();
}

/* ═════════ INFO BAR ═════════ */
function showInfo(){
  ictx.fillStyle='#000'; ictx.fillRect(0,0,inf.width,inf.height);
  ictx.font='16px Arial'; ictx.fillStyle='#3F3';
  const mode = digital ? 'Digital' : 'Analógico';
  const rate = (digital?['3 Msps','100 Ksps','3 Ksps']:['70 Ksps','15 Ksps','0.3 Ksps'])[speed];
  const tx   = `Tx${tSteps[tIdx]}`;
  const wsStatus = websocket && websocket.readyState === WebSocket.OPEN ? 'WS✓' : 'WS✗';
  ictx.fillText(`${mode} – ${rate}${hold?' – HOLD':''} – ${vDiv} V/div – ${tx} – FS ${adcVmax} V – ${wsStatus}`,10,20);
}

/* ═════════ ESCALA ANALÓGICA ═════════ */
function yAnalog(s){
  const volt = (s/1023)*adcVmax;
  const pxDiv = cvs.height/10;
  return cvs.height - (volt/vDiv)*pxDiv;                // 0 V en la base
}

/* ═════════ DIBUJO DIGITAL ═════════ */
function drawDigital(data){
  const dx = cvs.width/(data.length-1);
  const yHigh = cvs.height*0.25, yLow = cvs.height-2, TH = 512;
  ctx.strokeStyle='#55F'; ctx.beginPath();

  for(let i=0;i<data.length-1;i++){
    const cur = data[i]   >= TH ? yHigh : yLow;
    const nxt = data[i+1] >= TH ? yHigh : yLow;
    ctx.moveTo(i*dx,cur); ctx.lineTo((i+1)*dx,cur);
    if(cur!==nxt){ ctx.moveTo((i+1)*dx,cur); ctx.lineTo((i+1)*dx,nxt); }
  }
  ctx.stroke();
}

/* ═════════ BUILD DATA TxN ═════════
   • n   = tSteps[tIdx]
   • rec = últimos n buffers (si hay menos se reutiliza el más viejo)
   • decimate(buffer,n) = toma una de cada n muestras   */
function decimate(b,n){
  const outLen = Math.floor(b.length/n);
  const out = new Uint16Array(outLen);
  for(let i=0;i<outLen;i++) out[i]=b[i*n];
  return out;
}
function buildData(){
  const n = tSteps[tIdx];
  if(n===1 || !buf) return buf;                          // Tx1
  // garantiza n buffers
  const sources = [];
  const all = [...prev,buf];                            // más antiguo → actual
  while(sources.length<n){
    sources.unshift(all[Math.max(0, all.length-1-sources.length)]);
  }
  // decima y concatena
  const partLen = Math.floor(buf.length/n);
  const merged  = new Uint16Array(partLen*n);
  let offset=0;
  sources.slice(-n).forEach(b=>{
    merged.set(decimate(b,n),offset);
    offset += partLen;
  });
  return merged;
}

/* ═════════ DRAW ═════════ */
function draw(){
  if(!buf) return;
  grid();
  const data = buildData();
  if(digital){
    drawDigital(data);
  }else{
    ctx.strokeStyle='#FB4'; ctx.beginPath();
    const dx=cvs.width/(data.length-1);
    for(let i=0;i<data.length-1;i++){
      const y1=yAnalog(data[i]), y2=yAnalog(data[i+1]);
      ctx.moveTo(i*dx,y1); ctx.lineTo((i+1)*dx,y2);
    }
    ctx.stroke();
  }
}

/* ═════════ WEBSOCKET ═════════ */
function initWebSocket() {
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = `${protocol}//${window.location.hostname}:81/`;
  
  websocket = new WebSocket(wsUrl);
  
  websocket.onopen = function(event) {
    console.log('WebSocket connected');
    document.getElementById('wsStatus').textContent = 'WebSocket: Connected';
    document.getElementById('wsStatus').style.color = '#0A0';
  };
  
  websocket.onmessage = function(event) {
    if (event.data instanceof Blob || event.data instanceof ArrayBuffer) {
      // Binary data (oscilloscope samples)
      const reader = new FileReader();
      reader.onload = function() {
        const arrayBuffer = reader.result;
        if (prev.length === 3) prev.shift();
        if (buf) prev.push(buf);
        buf = new Uint16Array(arrayBuffer);
        draw();
        // busy is still true here from the initial requestData call
        if (!hold) {
          busy = false; // Allow next request
          requestData(); // Request new data immediately
        } else {
          busy = false; // Clear busy flag if on hold, no new request
        }
      };
      reader.readAsArrayBuffer(event.data instanceof Blob ? event.data : new Blob([event.data]));
    } else {
      // Text data (JSON messages)
      try {
        const data = JSON.parse(event.data);
        if (data.type === 'connected') {
          console.log('WebSocket handshake complete');
          $('wsStatus').textContent = 'WebSocket: Connected & Ready';
          $('wsStatus').style.color = '#0A0'; // Green color for connected
          if (!hold) {
            busy = false; // Ensure not busy before first data request
            requestData(); // Request data immediately
          }
        } else if (data.error) {
          console.error('WebSocket server error:', data.error);
          $('wsStatus').textContent = 'WebSocket Error: ' + data.error;
          $('wsStatus').style.color = '#A00'; // Red color for error
        } else {
          console.log('Received unhandled text message:', data);
        }
      } catch (e) {
        console.error('Error parsing WebSocket message:', e);
        $('wsStatus').textContent = 'WebSocket: Comms Error';
        $('wsStatus').style.color = '#A00'; // Red color for error
      }
    }
  };
  
  websocket.onclose = function(event) {
    console.log('WebSocket disconnected');
    document.getElementById('wsStatus').textContent = 'WebSocket: Disconnected';
    document.getElementById('wsStatus').style.color = '#A00';
    busy = false;
    setTimeout(initWebSocket, 2000); // Reconnect after 2 seconds
  };
  
  websocket.onerror = function(error) {
    console.error('WebSocket error:', error);
    document.getElementById('wsStatus').textContent = 'WebSocket: Error';
    document.getElementById('wsStatus').style.color = '#A00';
    busy = false;
  };
}

function requestData() {
  if (busy || hold || !websocket || websocket.readyState !== WebSocket.OPEN) return;
  
  busy = true;
  const message = {
    cmd: 'getData',
    mode: digital ? 1 : 0,
    speed: speed,
    trigger: 0
  };
  
  websocket.send(JSON.stringify(message));
}

/* ═════════ HANDLERS ═════════ */
$('h').onclick = ()=>{hold=!hold; setBg($('h'),hold); showInfo(); if(!hold)setTimeout(requestData,100);};
$('d').onclick = ()=>{digital=!digital; setBg($('d'),digital); showInfo();};
$('fu').onclick=()=>{if(speed<2)speed++; showInfo();};
$('fd').onclick=()=>{if(speed>0)speed--; showInfo();};
$('vu').onclick=()=>{if(vIdx<vSteps.length-1){vIdx++; vDiv=vSteps[vIdx]; showInfo(); draw();}};
$('vd').onclick=()=>{if(vIdx>0){vIdx--; vDiv=vSteps[vIdx]; showInfo(); draw();}};
$('tu').onclick=()=>{if(tIdx<tSteps.length-1){tIdx++; showInfo(); draw();}};
$('td').onclick=()=>{if(tIdx>0){tIdx--; showInfo(); draw();}};
$('c').onclick = grid;
$('r').onclick = ()=>{
  hold=false; digital=false; speed=0; vIdx=5; vDiv=vSteps[vIdx]; tIdx=0;
  ['h','d'].forEach(id=>setBg($(id),false));
  prev.length=0; buf=null; grid(); showInfo();
};
/* calibración FS */
$('vmax').onchange = ()=>{
  const v=parseFloat($('vmax').value);
  if(!isNaN(v)&&v>0){adcVmax=v; showInfo(); draw();}
};

/* ═════════ INIT ═════════ */
window.addEventListener('load',()=>{
  cvs=$('scope'); ctx=cvs.getContext('2d');
  inf=$('info');  ictx=inf.getContext('2d');
  grid(); showInfo(); 
  initWebSocket();
});
</script></body></html>
)rawliteral";

  Server.send(200, "text/html", page);
}
#endif // webapp_H_
