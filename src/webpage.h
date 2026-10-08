#pragma once
#include <pgmspace.h>

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Llavero amigo</title>
<style>
:root{--bg:#ece8e1;--fg:#1d1b18;--mut:#6b655c;--card:#fff;--ln:#cfc9bf}
@media(prefers-color-scheme:dark){:root{--bg:#181715;--fg:#ece8e1;--mut:#9a948a;--card:#25231f;--ln:#3a3731}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:16px system-ui,sans-serif;padding:16px;max-width:760px;margin:auto}
h1{font-size:22px;margin:4px 0 2px}h2{font-size:13px;letter-spacing:.1em;text-transform:uppercase;color:var(--mut);margin:26px 0 10px;border-top:1px solid var(--ln);padding-top:16px}
p{color:var(--mut);margin:0 0 12px;font-size:14px}
.row{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:10px;align-items:center}
button{font:inherit;font-size:14px;padding:8px 14px;border-radius:999px;border:1px solid var(--fg);background:transparent;color:var(--fg);cursor:pointer}
button.on{background:var(--fg);color:var(--bg)}
input,select{font:inherit;font-size:15px;padding:8px 10px;border-radius:10px;border:1px solid var(--ln);background:var(--card);color:var(--fg);min-width:0}
input[type=range]{padding:0;flex:1;min-width:140px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:12px}
.live{display:block;width:240px;height:240px;border-radius:6px;background:#000;margin:0 0 6px;image-rendering:pixelated}
.th{display:block;width:100%;aspect-ratio:1;border-radius:6px;background:#000;image-rendering:pixelated}
.card{background:var(--card);border-radius:16px;padding:10px;border:2px solid transparent;cursor:pointer;text-align:center}
.card.on{border-color:var(--fg)}
.sw{height:70px;border-radius:10px;display:flex;align-items:center;justify-content:center;gap:12px}
.sw i{display:block;width:14px;height:24px;border-radius:7px}
.nm{font-weight:700;margin-top:8px}.st{font-size:12px;color:var(--mut)}
#msg{position:fixed;left:16px;right:16px;bottom:16px;background:var(--fg);color:var(--bg);padding:10px 14px;border-radius:12px;display:none}
</style></head><body>
<h1>Llavero amigo</h1><p id="info">Cargando...</p>

<h2>Pantalla ahora</h2><img class="live" id="live" alt="pantalla del llavero"><p>Es lo que muestra el llavero en este momento (se actualiza solo).</p>

<h2>Mascota</h2><div class="grid" id="gr"></div>

<h2>Expresion</h2>
<p>En automatico cambia sola: bateria baja, notificacion nueva, o al azar entre feliz, parpadeo y enamorado. Aqui puedes forzar una para probar.</p>
<div class="row" id="ex"></div>

<h2>Bateria y notificaciones</h2>
<div class="row"><span>Bateria</span><input type="range" id="bat" min="0" max="100" oninput="batv.textContent=this.value+'%'" onchange="post('/api/battery',{v:this.value})"><b id="batv">-</b></div>
<p id="batnote"></p>
<div class="row"><input id="nf" placeholder="De (ej. Ana)" style="width:130px"><input id="nt" placeholder="Mensaje" style="flex:1">
<button onclick="post('/api/notify',{from:nf.value||'Prueba',text:nt.value||'Hola!'})">Enviar notificacion</button>
<button onclick="post('/api/clear',{})">Borrar</button></div>

<h2>WiFi</h2>
<p id="wifi"></p>
<div id="nets"></div>
<p>Agregar otra red (se guardan hasta 6; el llavero se une solo a la mejor que vea):</p>
<div class="row"><select id="ss" style="flex:1"></select><button onclick="scan()">Buscar redes</button></div>
<div class="row"><input id="ssid" placeholder="Nombre de la red" style="flex:1"><input id="pw" type="password" placeholder="Contrasena" style="flex:1"></div>
<div class="row"><button class="on" onclick="saveWifi()">Guardar y conectar</button></div>

<h2>Hora</h2>
<p id="time"></p>
<div class="row"><button class="on" onclick="setNow()">Usar la hora de este telefono</button></div>
<div class="row"><input type="datetime-local" id="dt"><button onclick="setManual()">Poner esta hora</button></div>

<div id="msg"></div>
<script>
const $=id=>document.getElementById(id),E=["Feliz","Parpadeo","Enamorado","Bateria baja","Notificacion"];
let L=[],S={c:"",e:-1};
function toast(t){const m=$("msg");m.textContent=t;m.style.display="block";clearTimeout(toast.h);toast.h=setTimeout(()=>m.style.display="none",3500)}
async function post(u,o){try{const r=await fetch(u,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams(o)});const j=await r.json();refresh();return j}catch(e){toast("Sin conexion con el llavero")}}
function draw(){
 $("ex").innerHTML=['<button class="'+(S.e<0?"on":"")+'" onclick="api(\'e=-1\')">Auto</button>'].concat(E.map((n,i)=>`<button class="${i==S.e?"on":""}" onclick="api('e=${i}')">${n}</button>`)).join("");
 if(!$("gr").children.length)$("gr").innerHTML=L.map(c=>`<div class="card" id="cd-${c.id}" onclick="api('c=${c.id}')"><img class="th" id="th-${c.id}" alt="${c.name}"><div class="nm">${c.name}</div><div class="st">${c.id} · ${c.style}</div></div>`).join("");
 L.forEach(c=>$("cd-"+c.id).classList.toggle("on",c.id==S.c));
 $("info").textContent="Notificaciones guardadas: "+S.n;
 if(document.activeElement!==$("bat")){$("bat").value=S.bat;$("batv").textContent=S.bat+"%"}
 $("bat").disabled=!S.batSim;
 const pin=Math.round(S.batMv/2);
 $("batnote").textContent=S.batOver?"⚠ El pin GPIO0 recibe "+pin+" mV (tope del ADC): demasiado voltaje. Si hay pila conectada, falta el divisor 100k+100k o esta mal puesto. Se ignora esa lectura.":S.batSim?"Sin pila detectada (GPIO0 = "+pin+" mV): valor simulado para probar las caras.":"Midiendo la pila: "+(S.batMv/1000).toFixed(2)+" V (pin "+pin+" mV)";
 $("batnote").style.color=S.batOver?"#d9480f":"";
 $("wifi").textContent=S.wifi.conn?("Conectado a "+S.wifi.ssid+" · IP "+S.wifi.ip):(S.wifi.setup?"Modo configuracion activo (red Llavero-Setup).":"Sin conexion. Red guardada: "+(S.wifi.ssid||"ninguna"));
 $("nets").innerHTML=(S.nets.length?"<p>Redes guardadas:</p>":"<p>No hay redes guardadas.</p>")+S.nets.map(n=>`<div class="row"><b style="flex:1">${esc(n)}${n==S.wifi.ssid&&S.wifi.conn?" · conectada":""}</b><button onclick="useNet(decodeURIComponent('${encodeURIComponent(n)}'))">Conectar</button><button onclick="forgetNet(decodeURIComponent('${encodeURIComponent(n)}'))">Olvidar</button></div>`).join("");
 $("time").textContent="Hora del llavero: "+(S.time||"sin ajustar")+(S.ntp?" (por internet)":"");
}
function esc(t){return t.replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]))}
async function useNet(n){await post("/api/wifi",{ssid:n});toast("Conectando a "+n+"...")}
async function forgetNet(n){if(confirm("Olvidar la red "+n+"?"))await post("/api/forget",{ssid:n})}
async function refresh(){try{S=await (await fetch("/api/state")).json();draw()}catch(e){}}
async function api(q){S=await (await fetch("/api/set?"+q)).json();draw()}
async function scan(){toast("Buscando redes...");try{const r=await (await fetch("/api/scan")).json();$("ss").innerHTML='<option value="">-- elige una red --</option>'+r.map(n=>`<option>${n}</option>`).join("");toast(r.length+" redes")}catch(e){toast("Fallo la busqueda")}}
$("ss").onchange=()=>{if($("ss").value)$("ssid").value=$("ss").value};
async function saveWifi(){if(!$("ssid").value)return toast("Escribe el nombre de la red");await post("/api/wifi",{ssid:$("ssid").value,pass:$("pw").value});toast("Conectando... si esta pagina deja de responder, el llavero cambio de red. Mira su pantalla para ver la IP.")}
async function setNow(){await post("/api/time",{t:Math.floor(Date.now()/1000)});toast("Hora actualizada")}
async function setManual(){if(!$("dt").value)return toast("Elige fecha y hora");await post("/api/time",{t:Math.floor(new Date($("dt").value).getTime()/1000)});toast("Hora actualizada")}
// las miniaturas las dibuja el propio llavero (misma cara que en pantalla); se piden de una en una
function loadThumbs(i){
 if(i>=L.length){liveLoop();return}
 const im=$("th-"+L[i].id),next=()=>loadThumbs(i+1);
 im.onload=next;im.onerror=next;im.src="/api/frame?c="+L[i].id+"&e=0&t=1&s=2&v="+encodeURIComponent(S.build||"");
}
function liveLoop(){
 const im=$("live");
 const go=()=>setTimeout(()=>{if(document.hidden)return go();im.src="/api/screen?"+Date.now()},350);
 im.onload=go;im.onerror=()=>setTimeout(()=>{im.src="/api/screen?"+Date.now()},2000);
 im.src="/api/screen?"+Date.now();
}
(async()=>{L=await (await fetch("/api/list")).json();await refresh();loadThumbs(0);setInterval(refresh,5000)})();
</script></body></html>)HTML";
