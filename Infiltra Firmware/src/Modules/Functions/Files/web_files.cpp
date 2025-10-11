#include "web_files.h"
#include "UserInterface/menus/menu_submenus.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <FS.h>
#include <DNSServer.h>

#if defined(M5CARDPUTER)
  static constexpr uint8_t ROT_TOP = 4;
#else
  static constexpr uint8_t ROT_TOP = 2;
#endif
static constexpr uint8_t ROT_ALT = (ROT_TOP + 1) & 0x3;
static const char* kWriteRoot = "/integrated";   
static const char* kMainRoot  = "/main";         
static WebServer* sSrv = nullptr;
static DNSServer* sDns = nullptr;
static bool       sRunning    = false;
static bool       sFrameDrawn = false;
static bool       sStaticDrawn= false;   
static bool       sDirty      = true;    
static IPAddress  sStaIp;
static IPAddress  sApIp;
static uint16_t   sPort = 80;
static const uint16_t DNS_PORT = 53;
static String     sApSsid   = "";
static String     sApPass   = "";   
static bool       sApStarted= false;
static bool       sHasSta   = false;
static uint32_t   sLastReqMs= 0;    
static bool       sShowSta  = false;
static int  W=0,H=0;
static int  bx=0,by=0,bw=0,bh=0;   
static int  cx=0,cy=0,cw=0;        
static int  rAccessX=0, rAccessY=0, rAccessW=0, rAccessH=0;
static int  rIfaceX =0, rIfaceY =0, rIfaceW =0, rIfaceH =0;
static int  rTrafficX=0, rTrafficY=0, rTrafficW=0, rTrafficH=0;
static uint64_t   sULTotal  = 0;
static uint64_t   sDLTotal  = 0;
static uint64_t   sLastUL   = 0;
static uint64_t   sLastDL   = 0;
static uint32_t   sLastRateMs = 0;
static float      sULRateBps = 0.0f;
static float      sDLRateBps = 0.0f;
static String L_mode, L_url, L_ip, L_dlRate, L_ulRate, L_dlTot, L_ulTot, L_join;
static void applySmallUi(TFT_eSPI& tft){
  tft.setTextFont(1);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}
static String fitText(TFT_eSPI& tft, String s, int maxW){
  if (tft.textWidth(s) <= maxW) return s;
  while (s.length() > 3 && tft.textWidth(s + "...") > maxW) s.remove(s.length()-1);
  return s + "...";
}
static String ipToStr(IPAddress ip){ char b[32]; snprintf(b,sizeof(b),"%u.%u.%u.%u", ip[0],ip[1],ip[2],ip[3]); return String(b); }
static String humanBytes(uint64_t n){
  char buf[32];
  if (n >= (1ULL<<30)) { snprintf(buf,sizeof(buf),"%.2f GB", n/1073741824.0); }
  else if (n >= (1ULL<<20)) { snprintf(buf,sizeof(buf),"%.2f MB", n/1048576.0); }
  else if (n >= (1ULL<<10)) { snprintf(buf,sizeof(buf),"%.2f KB", n/1024.0); }
  else { snprintf(buf,sizeof(buf),"%llu B", (unsigned long long)n); }
  return String(buf);
}
static String humanRate(float bps){
  char buf[32];
  if (bps >= (1024.0f*1024.0f)) snprintf(buf,sizeof(buf),"%.2f MB/s", bps/1048576.0f);
  else if (bps >= 1024.0f)      snprintf(buf,sizeof(buf),"%.2f KB/s", bps/1024.0f);
  else                          snprintf(buf,sizeof(buf),"%.0f B/s",  bps);
  return String(buf);
}

static void updateRates(){
  uint32_t now = millis();
  uint32_t dt = now - sLastRateMs;
  if (dt < 600) return; 
  float dt_s = dt / 1000.0f;
  uint64_t dUL = sULTotal - sLastUL;
  uint64_t dDL = sDLTotal - sLastDL;
  float instUL = dUL / dt_s;
  float instDL = dDL / dt_s;
  const float a = 0.35f;  
  sULRateBps = a*instUL + (1-a)*sULRateBps;
  sDLRateBps = a*instDL + (1-a)*sDLRateBps;
  sLastUL = sULTotal;
  sLastDL = sDLTotal;
  sLastRateMs = now;
}

static void markReq(){ sLastReqMs = millis(); }
static const char* kIndexHTML PROGMEM = R"HTML(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"/><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Infiltra — Web Files</title>
<style>
:root{
  --bg:#0a0f14; --panel:#0f141b; --edge:#212a36; --ink:#e8edf6; --mut:#9aa3ad; --acc:#30c7c7;
  --chip:#0b1118; --ok:#22c55e; --bad:#ef4444; --row:#10161f; --row2:#0c121a;
}
*{box-sizing:border-box} html,body{height:100%;margin:0;background:linear-gradient(180deg,#0a0f14, #0b1117 35%, #0a0f14);
  color:var(--ink); font:14px/1.5 ui-sans-serif,system-ui,-apple-system,Segoe UI,Roboto,Inter}
a{color:var(--acc);text-decoration:none} a:hover{opacity:.9}
.wrap{max-width:1000px;margin:22px auto;padding:0 14px}

.header{display:flex;align-items:center;gap:10px;margin:8px 0 12px}
.logo{width:28px;height:28px;border-radius:8px;background:radial-gradient(circle at 30% 30%, #4ff, #0af 60%, #036);
      border:1px solid #134; box-shadow:0 0 0 3px #0a141b inset}
.h1{font-size:18px;font-weight:800;margin:0 4px}
.badge{display:inline-flex;align-items:center;gap:6px;background:var(--chip);border:1px solid var(--edge);color:var(--mut);
  padding:3px 9px;border-radius:999px}

.grid{display:grid;grid-template-columns:1fr;gap:12px}
@media(min-width:900px){ .grid{grid-template-columns:1.1fr .9fr} }

.card{background:rgba(16,22,31,.85);backdrop-filter:saturate(1.25) blur(4px); border:1px solid var(--edge);
  border-radius:14px;padding:14px;box-shadow:0 10px 30px rgba(0,0,0,.25)}

.kv{display:grid;grid-template-columns:120px 1fr;gap:6px 10px}
.kv b{color:var(--mut)}

.toolbar{display:flex;align-items:center;gap:10px;justify-content:space-between;flex-wrap:wrap}
.breadcrumb{color:var(--mut)}
.controls{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
input[type=file],input[type=text],button,textarea{
  border:1px solid var(--edge);border-radius:10px;background:#0b0f15;color:var(--ink);padding:8px 10px}
button{cursor:pointer;transition:.15s} button:hover{border-color:#3b4456; transform:translateY(-1px)}
button.pri{background:linear-gradient(180deg, #35d, #248); color:#fff; border-color:#247}
button.warn{background:linear-gradient(180deg, #a33, #812); color:#fff; border-color:#612}
input[type=text]{min-width:200px}

table{width:100%;border-collapse:separate;border-spacing:0 6px}
tbody tr{background:linear-gradient(180deg,var(--row),var(--row2)); border:1px solid var(--edge)}
tbody td, thead th{padding:10px 10px}
tbody tr td:first-child{border-top-left-radius:10px;border-bottom-left-radius:10px}
tbody tr td:last-child{border-top-right-radius:10px;border-bottom-right-radius:10px}
thead th{color:var(--mut);font-weight:600}
.action{display:inline-flex;gap:6px}

small.mut{color:var(--mut)}
.tag{display:inline-flex;gap:6px;align-items:center;background:var(--chip);border:1px solid var(--edge);padding:2px 8px;border-radius:999px}
pre.path{margin:0;padding:6px 8px;background:#0b1016;border:1px solid var(--edge);border-radius:8px;color:#cfe}
</style></head><body>
<div class="wrap">

  <div class="header">
    <div class="logo"></div>
    <div class="h1">Web Files</div>
    <span class="badge" id="net">starting…</span>
  </div>

  <div class="grid">
    <div class="card">
      <div class="kv">
        <b>Interface</b><span id="ifcMode">AP</span>
        <b>URL</b><span id="ifcUrl">http:
        <b>IP</b><span id="ifcIp">192.168.4.1</span>
        <b>Status</b><span id="ifcStatus"><span style="color:var(--ok)">●</span> ready</span>
        <b>Traffic</b><span><span id="dl">0 B/s</span> ↓ &nbsp; <span id="ul">0 B/s</span> ↑</span>
        <b>Totals</b><span><span id="dlT">0 B</span> • <span id="ulT">0 B</span></span>
      </div>
    </div>

    <div class="card">
      <div style="display:flex;gap:8px;align-items:center;justify-content:space-between;flex-wrap:wrap">
        <div class="breadcrumb" id="crumb">Home / integrated</div>
        <div class="tag">scope: <b>/integrated</b> (writable)</div>
      </div>
      <div class="controls" style="margin-top:8px">
        <input id="filePick" type="file" multiple>
        <input id="newPath" placeholder="/notes/readme.txt" style="width:260px">
        <button class="pri" onclick="mk()">New</button>
        <button onclick="mkdir()">New Folder</button>
      </div>
      <small class="mut">“Infiltra Main” shows here as read-only — new items go to <code>/integrated</code> and appear on-device under Infiltra Main.</small>
    </div>
  </div>

  <div class="card">
    <table id="tbl"><thead><tr><th>Name</th><th>Size</th><th>Actions</th></tr></thead><tbody></tbody></table>
  </div>

  <div class="card">
    <div style="display:flex;gap:8px;align-items:center;margin-bottom:8px;flex-wrap:wrap">
      <pre class="path" id="epLab">/integrated/</pre>
      <input id="editPath" style="flex:1;min-width:240px" placeholder="/integrated/notes/readme.txt">
      <button onclick="loadEdit()">Open</button>
      <button class="pri" onclick="saveEdit()">Save</button>
      <button class="warn" onclick="delFromEdit()">Delete</button>
    </div>
    <textarea id="editor" placeholder="
  </div>

</div>
<script>
let CURR="/integrated/"; 

function crumb(p){
  const parts=p.replace(/\/+/g,'/').split('/').filter(Boolean);
  let acc="/";
  const links=parts.map(seg=>{ acc += seg + "/"; return `<a href="#" onclick="goto('${acc}');return false;">${seg}</a>`; });
  return 'Home / ' + links.join(' / ');
}
function goto(p){ CURR=p; refresh(); }

function cell(c){ const td=document.createElement('td'); if (c && c.nodeType) td.appendChild(c); else td.textContent=(c??''); return td; }

async function refresh(){
  document.getElementById('crumb').innerHTML = crumb(CURR);
  const r=await fetch('/api/list?path='+encodeURIComponent(CURR));
  const data=await r.json();
  const tb=document.querySelector('#tbl tbody'); tb.innerHTML='';
  if (CURR !== "/integrated/"){
    const tr=document.createElement('tr');
    const a=document.createElement('a'); a.textContent="← Back"; a.href="#"; a.onclick=()=>{const up=CURR.replace(/\/+$/,'').split('/'); up.pop(); const parent=up.join('/')+'/'; goto(parent.startsWith('/integrated')?parent:'/integrated/');};
    tr.append(cell(a), cell(''), cell('')); tb.append(tr);
  }
  data.forEach(it=>{
    const tr=document.createElement('tr');
    const a=document.createElement('a'); a.textContent=it.name; a.href="#";
    a.onclick=()=>{ if (it.type==='dir') goto(it.path); else setEditPath(it.path); };
    const acts=document.createElement('div'); acts.className='action';
    const edit=document.createElement('button'); edit.textContent='Edit';
    edit.onclick=()=>{ if (it.path.startsWith('/main')) { alert('Read-only: Infiltra Main'); return; } setEditPath(it.path); loadEdit(); };
    const dl=document.createElement('button'); dl.textContent='Download';
    dl.onclick=()=>{ if (it.type==='dir') return; window.location='/download?path='+encodeURIComponent(it.path); };
    const del=document.createElement('button'); del.textContent='Delete';
    del.onclick=()=>{ if (it.path.startsWith('/main')) { alert('Read-only: Infiltra Main'); return; } delFile(it.path); };
    if (it.type==='dir'){ del.disabled=true; dl.disabled=true; }
    acts.append(edit, dl, del);
    tr.append(cell(a), cell(it.type==='dir'?'—':it.size), cell(acts));
    tb.append(tr);
  });
  document.getElementById('net').textContent=window.location.href;
}
async function upload(){
  const files=document.getElementById('filePick').files;
  for(const f of files){
    const fd=new FormData(); fd.append('file',f,f.name);
    await fetch('/upload?dir='+encodeURIComponent(CURR), {method:'POST', body:fd});
  }
  refresh();
}
document.getElementById('filePick').addEventListener('change', upload);

function setEditPath(p){ document.getElementById('editPath').value = p; document.getElementById('epLab').textContent = p; }

async function delFromEdit(){
  const p=document.getElementById('editPath').value || '';
  if (!p || p.startsWith('/main')) { alert('Read-only or empty path.'); return; }
  if (!confirm('Delete '+p+' ?')) return;
  await delFile(p); refresh();
}
async function delFile(p){
  await fetch('/api/delete', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'path='+encodeURIComponent(p)});
  if ((document.getElementById('editPath').value||'')===p){ document.getElementById('editor').value=''; }
  refresh();
}
async function mk(){
  let p=document.getElementById('newPath').value||'/new.txt';
  if (!p.startsWith('/')) p='/'+p;
  if (!p.startsWith('/integrated/')) p = CURR.replace(/\/+$/,'') + p;
  await fetch('/api/put', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'path='+encodeURIComponent(p)+'&content='});
  refresh();
}
async function mkdir(){
  let p=document.getElementById('newPath').value||'/folder';
  if (!p.startsWith('/')) p='/'+p;
  if (!p.startsWith('/integrated/')) p = CURR.replace(/\/+$/,'') + p;
  await fetch('/api/mkdir', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'path='+encodeURIComponent(p)});
  refresh();
}
async function loadEdit(){
  const p=document.getElementById('editPath').value;
  if (p.startsWith('/main')) { alert('Read-only: Infiltra Main'); return; }
  const r=await fetch('/api/get?path='+encodeURIComponent(p));
  if (r.status!==200){ alert('Open failed.'); return; }
  document.getElementById('editor').value = await r.text();
}
async function saveEdit(){
  const p=document.getElementById('editPath').value;
  if (!p.startsWith('/integrated/')) { alert('Save only in /integrated'); return; }
  const t=document.getElementById('editor').value;
  await fetch('/api/put', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'path='+encodeURIComponent(p)+'&content='+encodeURIComponent(t)});
  refresh();
}
async function pullStats(){
  try{
    const r=await fetch('/api/stats'); const s=await r.json();
    document.getElementById('ifcMode').textContent = s.mode;
    document.getElementById('ifcUrl').textContent  = s.url;
    document.getElementById('ifcIp').textContent   = s.ip;
    document.getElementById('dl').textContent      = s.dl_rate;
    document.getElementById('ul').textContent      = s.ul_rate;
    document.getElementById('dlT').textContent     = s.dl_total;
    document.getElementById('ulT').textContent     = s.ul_total;
  }catch(e){}
  setTimeout(pullStats, 1200); 
}
refresh(); pullStats();
</script>
</body></html>
)HTML";

static bool listDirJSON_fs(fs::FS &fs, const String& abs, String& out){
  fs::File root = fs.open(abs);
  if(!root || !root.isDirectory()) return false;
  out += "[";
  bool first=true;
  fs::File file = root.openNextFile();
  while(file){
    if(!first) out += ",";
    first=false;
    String full = String(file.name());
    String leaf = full;
    if (leaf.startsWith(abs)) leaf = leaf.substring(abs.length());
    if (leaf.startsWith("/")) leaf.remove(0,1);
    int slash = leaf.lastIndexOf('/');
    if (slash >= 0) leaf = leaf.substring(slash+1);
    out += "{\"name\":\"" + leaf + "\",";
    out += "\"path\":\"" + String(abs.endsWith("/")?abs:(abs+"/")) + leaf + (file.isDirectory()?"/":"") + "\",";
    out += "\"size\":" + String(file.isDirectory() ? 0 : file.size()) + ",";
    out += "\"type\":\"" + String(file.isDirectory() ? "dir" : "file") + "\"}";
    file = root.openNextFile();
  }
  out += "]";
  return true;
}

struct VNode { const char* name; bool dir; const int8_t* kids; uint8_t kidCount; };
enum : int8_t { V_ROOT=0,V_SRC,V_MAIN_CPP,V_MODULES,V_UI,V_COUNT };
static const int8_t VK_ROOT[]={ V_SRC,V_MODULES,V_UI };
static const int8_t VK_SRC[] ={ V_MAIN_CPP };
static const VNode VN[V_COUNT]={
  {"Infiltra Main",true, VK_ROOT,(uint8_t)sizeof(VK_ROOT)},
  {"src",true,            VK_SRC, (uint8_t)sizeof(VK_SRC)},
  {"main.cpp",false,      nullptr,0},
  {"Modules",true,        nullptr,0},
  {"UserInterface",true,  nullptr,0},
};
static bool vWalk(const String& vPath, int8_t& outNode){
  if (vPath == "/main" || vPath == "/main/"){ outNode=V_ROOT; return true; }
  String sub=vPath.substring(6);
  while (sub.length() && sub[0]=='/') sub.remove(0,1);
  int8_t cur=V_ROOT;
  while (sub.length()){
    int slash=sub.indexOf('/');
    String step=(slash<0)?sub:sub.substring(0,slash);
    const VNode& n=VN[cur]; bool found=false;
    for(uint8_t i=0;i<n.kidCount;i++){
      int8_t k=n.kids[i]; if(step==VN[k].name){ cur=k; found=true; break; }
    }
    if(!found) return false;
    if (slash<0) sub=""; else sub.remove(0,slash+1);
  }
  outNode=cur; return true;
}
static bool listDirJSON_main(const String& vPath, String& out){
  int8_t node; if(!vWalk(vPath,node)) return false;
  const VNode& n=VN[node]; if(!n.dir) return false;
  out += "[";
  for (uint8_t i=0;i<n.kidCount;i++){
    if (i) out += ",";
    const VNode& k = VN[n.kids[i]];
    out += "{\"name\":\""; out += k.name; out += (k.dir?"/":""); out += "\",";
    out += "\"path\":\""; out += vPath; if (!vPath.endsWith("/")) out+="/"; out += k.name; out += (k.dir?"/":""); out += "\",";
    out += "\"size\":0,\"type\":\""; out += (k.dir?"dir":"file"); out += "\"}";
  }
  out += "]";
  return true;
}

static String listRootJSON(){
  return "["
         "{\"name\":\"Infiltra Main/\",\"path\":\"/main/\",\"size\":0,\"type\":\"dir\"},"
         "{\"name\":\"integrated/\",\"path\":\"/integrated/\",\"size\":0,\"type\":\"dir\"}"
         "]";
}

static fs::File sUpFile;
static String contentTypeFor(const String& path) {
  String p = path; p.toLowerCase();
  if (p.endsWith(".html")||p.endsWith(".htm")) return "text/html";
  if (p.endsWith(".css"))  return "text/css";
  if (p.endsWith(".js"))   return "application/javascript";
  if (p.endsWith(".json")) return "application/json";
  if (p.endsWith(".txt"))  return "text/plain";
  if (p.endsWith(".png"))  return "image/png";
  if (p.endsWith(".jpg")||p.endsWith(".jpeg")) return "image/jpeg";
  if (p.endsWith(".gif"))  return "image/gif";
  return "application/octet-stream";
}
static void ensureDir(const String& path){ SPIFFS.mkdir(path); }
static String normDir(String p){
  if (!p.startsWith("/")) p = "/" + p;
  p.replace("\\", "/");
  return p;
}

static void handleIndex(){ markReq(); sSrv->send_P(200,"text/html",kIndexHTML); sDLTotal += strlen_P(kIndexHTML); }
static void handleList(){
  markReq();
  String p = sSrv->arg("path"); if (!p.length()) p="/";
  p.replace("\\", "/");
  if (p == "/" || p == ""){ String out=listRootJSON(); sSrv->send(200,"application/json",out); sDLTotal+=out.length(); return; }
  if (p.startsWith(kMainRoot)){ String out; if(!listDirJSON_main(p,out)) out="[]"; sSrv->send(200,"application/json",out); sDLTotal+=out.length(); return; }
  if (p.startsWith(kWriteRoot)){ String out; ensureDir(kWriteRoot); if(!listDirJSON_fs(SPIFFS,p,out)) out="[]"; sSrv->send(200,"application/json",out); sDLTotal+=out.length(); return; }
  String out=listRootJSON(); sSrv->send(200,"application/json",out); sDLTotal+=out.length();
}

static void handleGet(){
  markReq();
  String p=sSrv->arg("path"); if(!p.length()){ sSrv->send(400,"text/plain","missing path"); sDLTotal+=12; return; }
  if (p.startsWith(kMainRoot)){ sSrv->send(403,"text/plain","read-only: infiltra main"); sDLTotal+=24; return; }
  if (!p.startsWith(kWriteRoot)) p = String(kWriteRoot) + (p.startsWith("/")?p:("/"+p));
  if (!SPIFFS.exists(p)){ sSrv->send(404,"text/plain","not found"); sDLTotal+=9; return; }
  fs::File f=SPIFFS.open(p,"r"); if(!f){ sSrv->send(500,"text/plain","open fail"); sDLTotal+=9; return; }
  size_t sent = sSrv->streamFile(f, contentTypeFor(p)); sDLTotal += sent; f.close();
}

static void handlePut(){
  markReq();
  String p=sSrv->arg("path"); String c=sSrv->arg("content");
  if(!p.length()){ sSrv->send(400,"text/plain","missing path"); sDLTotal+=12; return; }
  if (p.startsWith(kMainRoot)){ sSrv->send(403,"text/plain","read-only: infiltra main"); sDLTotal+=24; return; }
  if (!p.startsWith(kWriteRoot)) p = String(kWriteRoot) + (p.startsWith("/")?p:("/"+p));
  int slash=p.lastIndexOf('/'); if(slash>0) ensureDir(p.substring(0,slash));
  sULTotal += p.length() + c.length();
  fs::File f=SPIFFS.open(p,"w");
  if(!f){ sSrv->send(500,"text/plain","write fail"); sDLTotal+=10; return; }
  if (c.length()) f.print(c);
  f.close();
  sSrv->send(200,"text/plain","ok"); sDLTotal+=2;
}

static void handleMkdir(){
  markReq();
  String p=sSrv->arg("path");
  if(!p.length()){ sSrv->send(400,"text/plain","missing path"); sDLTotal+=12; return; }
  if (p.startsWith(kMainRoot)){ sSrv->send(403,"text/plain","read-only: infiltra main"); sDLTotal+=24; return; }
  if (!p.startsWith(kWriteRoot)) p = String(kWriteRoot) + (p.startsWith("/")?p:("/"+p));
  ensureDir(p);
  sSrv->send(200,"text/plain","ok"); sDLTotal+=2;
}

static void handleDelete(){
  markReq();
  String p=sSrv->arg("path");
  if(!p.length()){ sSrv->send(400,"text/plain","missing path"); sDLTotal+=12; return; }
  if (p.startsWith(kMainRoot)){ sSrv->send(403,"text/plain","read-only: infiltra main"); sDLTotal+=24; return; }
  if (!p.startsWith(kWriteRoot)) p = String(kWriteRoot) + (p.startsWith("/")?p:("/"+p));
  sULTotal += p.length();
  if (!SPIFFS.exists(p)){ sSrv->send(404,"text/plain","not found"); sDLTotal+=9; return; }
  SPIFFS.remove(p); sSrv->send(200,"text/plain","ok"); sDLTotal+=2;
}

static void handleDownload(){
  markReq();
  String p=sSrv->arg("path");
  if (p.startsWith(kMainRoot)){ sSrv->send(403,"text/plain","read-only: infiltra main"); sDLTotal+=24; return; }
  if (!p.startsWith(kWriteRoot)) p = String(kWriteRoot) + (p.startsWith("/")?p:("/"+p));
  if (!SPIFFS.exists(p)){ sSrv->send(404,"text/plain","not found"); sDLTotal+=9; return; }
  fs::File f=SPIFFS.open(p,"r"); if(!f){ sSrv->send(500,"text/plain","open fail"); sDLTotal+=9; return; }
  size_t sent = sSrv->streamFile(f, contentTypeFor(p)); sDLTotal += sent; f.close();
}

static void handleUpload(){
  HTTPUpload& up = sSrv->upload();
  if (up.status == UPLOAD_FILE_START) {
    markReq();
    String dir = sSrv->arg("dir"); if(!dir.length()) dir = String(kWriteRoot) + "/";
    dir = normDir(dir);
    if (!dir.startsWith(kWriteRoot)) dir = String(kWriteRoot) + "/";
    String full = dir;
    String fn = up.filename; if (fn.startsWith("/")) fn.remove(0,1);
    full += fn;
    int slash = full.lastIndexOf('/'); if (slash>0) ensureDir(full.substring(0,slash));
    sUpFile = SPIFFS.open(full, "w");
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (sUpFile) sUpFile.write(up.buf, up.currentSize);
    sULTotal += up.currentSize;
  } else if (up.status == UPLOAD_FILE_END) {
    if (sUpFile) sUpFile.close();
    sSrv->send(200,"text/plain","ok"); sDLTotal+=2;
  }
}

static void handleStats(){
  String urlSta = sHasSta ? ("http://" + ipToStr(sStaIp)) : "";
  String urlAp  = "http://" + ipToStr(sApIp);
  String urlShown = (sShowSta && sHasSta) ? urlSta : urlAp;
  String ipShown  = (sShowSta && sHasSta) ? ipToStr(sStaIp) : ipToStr(sApIp);
  String mode     = (sShowSta && sHasSta) ? "STA" : "AP";
  String out = "{\"mode\":\""+mode+"\",\"url\":\""+urlShown+"\",\"ip\":\""+ipShown+"\","
               "\"dl_rate\":\""+humanRate(sDLRateBps)+"\",\"ul_rate\":\""+humanRate(sULRateBps)+"\","
               "\"dl_total\":\""+humanBytes(sDLTotal)+"\",\"ul_total\":\""+humanBytes(sULTotal)+"\"}";
  sSrv->send(200, "application/json", out); sDLTotal += out.length();
}

static void handleNotFound(){ markReq(); sSrv->send_P(200,"text/html",kIndexHTML); sDLTotal += strlen_P(kIndexHTML); }
static bool startAPWithCandidates(){
  IPAddress candidates[] = { IPAddress(192,168,4,1), IPAddress(10,1,1,1), IPAddress(172,16,4,1) };
  bool ok=false;
  uint64_t mac = ESP.getEfuseMac();
  char suf[7]; snprintf(suf,sizeof(suf),"%06X",(unsigned int)(mac & 0xFFFFFF));
  sApSsid = "Infiltra-" + String(suf);
  for (auto ip : candidates) {
    WiFi.softAPdisconnect(true); delay(50);
    WiFi.softAPConfig(ip, ip, IPAddress(255,255,255,0));
    bool ap = WiFi.softAP(sApSsid.c_str(), sApPass.length()?sApPass.c_str():nullptr, 6, 0);
    delay(80);
    IPAddress now = WiFi.softAPIP();
    if (ap && now[0]==ip[0] && now[1]==ip[1] && now[2]==ip[2]) { sApIp = now; ok = true; break; }
  }
  sApStarted = ok;
  return ok;
}
static void startDNS(){ if(!sDns) sDns = new DNSServer(); sDns->start(DNS_PORT, "*", sApIp); }
static void startServer(){
  if (!SPIFFS.begin(true)) { }
  if (!SPIFFS.exists(kWriteRoot)) SPIFFS.mkdir(kWriteRoot);
  if (!sSrv) sSrv = new WebServer(sPort);
  sSrv->on("/",            HTTP_GET,  handleIndex);
  sSrv->on("/api/list",    HTTP_GET,  handleList);
  sSrv->on("/api/get",     HTTP_GET,  handleGet);
  sSrv->on("/api/put",     HTTP_POST, handlePut);
  sSrv->on("/api/mkdir",   HTTP_POST, handleMkdir);
  sSrv->on("/api/delete",  HTTP_POST, handleDelete);
  sSrv->on("/download",    HTTP_GET,  handleDownload);
  sSrv->on("/upload",      HTTP_POST, [](){ sSrv->send(200,"text/plain","ok"); sDLTotal+=2; }, handleUpload);
  sSrv->on("/api/stats",   HTTP_GET,  handleStats);
  sSrv->onNotFound(handleNotFound);
  sSrv->begin();
  sRunning = true;
}

void webFilesStop(){
  if (sSrv) { sSrv->stop(); delete sSrv; sSrv = nullptr; }
  if (sDns) { sDns->stop(); delete sDns; sDns = nullptr; }
  if (sApStarted) { WiFi.softAPdisconnect(true); sApStarted=false; }
  sRunning = false;
}

static void drawFrame(TFT_eSPI& tft){
  tft.setRotation(ROT_TOP);
  drawOptionsLayerBackground(tft);
  tft.setRotation(ROT_ALT);
  applySmallUi(tft);
  W=tft.width(); H=tft.height();
  int marginX = 6, marginTop=8, marginBottom=4;
  bx = marginX; by = marginTop + 8; bw = W - marginX*2; bh = H - by - marginBottom;
  tft.fillRoundRect(bx, by, bw, bh, 6, TFT_BLACK);
  tft.drawRoundRect(bx, by, bw, bh, 6, TFT_DARKGREY);
  cx = bx+6; cy = by+6; cw = bw-12;
}

static void card(TFT_eSPI& tft, int x, int y, int w, int h, const char* title){
  tft.fillRoundRect(x, y, w, h, 4, TFT_BLACK);
  tft.drawRoundRect(x, y, w, h, 4, TFT_DARKGREY);
  tft.setCursor(x+6, y+2); tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.print(title);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawLine(x+4, y+14, x+w-4, y+14, TFT_DARKGREY);
}

static void kv(TFT_eSPI& tft, int x, int y, const char* k, const String& v, int w){
  tft.setCursor(x, y); tft.setTextColor(TFT_SILVER, TFT_BLACK); tft.print(k);
  int vx = x + 52; int vw = w - (vx - x) - 6;
  tft.setCursor(vx, y); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(fitText(tft, v, vw));
}

static void kvUpdate(TFT_eSPI& tft, int x, int y, const char* k, String& last, const String& now, int w){
  if (now == last) return;
  int vx = x + 52; int vw = w - (vx - x) - 6;
  tft.fillRect(vx, y-1, vw, 10, TFT_BLACK);
  tft.setCursor(vx, y);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print(fitText(tft, now, vw));
  last = now;
}

void webFilesReset(){
  sFrameDrawn=false; sStaticDrawn=false; sDirty=true; sLastReqMs=0;
  sShowSta=false;
  sULTotal = sDLTotal = sLastUL = sLastDL = 0;
  sULRateBps = sDLRateBps = 0.0f;
  sLastRateMs = millis();
  L_mode=L_url=L_ip=L_dlRate=L_ulRate=L_dlTot=L_ulTot=L_join="";
  WiFi.mode(WIFI_AP_STA);
  if (!startAPWithCandidates()){
    WiFi.softAPdisconnect(true); delay(50);
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    WiFi.softAP("Infiltra", nullptr);
    sApIp = WiFi.softAPIP();
    sApStarted = true;
    sApSsid = "Infiltra";
  }
  startDNS();
  sHasSta = (WiFi.status() == WL_CONNECTED);
  if (sHasSta) sStaIp = WiFi.localIP();
  startServer();
}

void webFilesDrawScreen(TFT_eSPI& tft){
  if (!sFrameDrawn){ drawFrame(tft); sFrameDrawn=true; sDirty=true; }
  if (!sRunning) { startServer(); sDirty=true; }
  if (sSrv) sSrv->handleClient();
  if (sDns) sDns->processNextRequest();
  updateRates();
  applySmallUi(tft);
  if (!sStaticDrawn || sDirty){
    tft.fillRect(bx+2, by+2, bw-4, 14, TFT_BLACK);
    tft.setCursor(cx, cy);
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.print("Web Files");
    tft.drawCircle(bx + bw - 12, by + 8, 2, TFT_DARKGREY);   
    int y = cy + 10;
    int footerH = 14;
    int contentBottom = by + bh - footerH - 4;
    int avail = contentBottom - y;
    int c1h = 30, c2h = 44, c3h = 28;
    int needed = c1h + 4 + c2h + 4 + c3h;
    if (needed > avail){
      int over = needed - avail;
      int d1=min(over, 6); c1h -= d1; over-=d1;
      int d2=min(over, 8); c2h -= d2; over-=d2;
      int d3=min(over, 6); c3h -= d3; over-=d3;
      if (c1h<24) c1h=24; if (c2h<34) c2h=34; if (c3h<22) c3h=22;
    }   
    rAccessX=cx-2; rAccessY=y; rAccessW=cw+4; rAccessH=c1h;
    card(tft, rAccessX, rAccessY, rAccessW, rAccessH, "Access");
    y += c1h + 4;
    rIfaceX=cx-2; rIfaceY=y; rIfaceW=cw+4; rIfaceH=c2h;
    card(tft, rIfaceX, rIfaceY, rIfaceW, rIfaceH, "Interfaces");
    y += c2h + 4;
    rTrafficX=cx-2; rTrafficY=y; rTrafficW=cw+4; rTrafficH=c3h;
    card(tft, rTrafficX, rTrafficY, rTrafficW, rTrafficH, "Traffic");
    tft.fillRect(cx, by + bh - footerH, cw, footerH, TFT_BLACK);
    tft.setCursor(cx, by + bh - footerH);
    tft.setTextColor(TFT_SILVER, TFT_BLACK);
    tft.print("A: toggle interface   C: back");
    sStaticDrawn = true;
    sDirty = false;
    L_mode=L_url=L_ip=L_dlRate=L_ulRate=L_dlTot=L_ulTot=L_join="";
  }

  bool flash = (millis() - sLastReqMs) < 550;
  uint16_t dotCol = flash ? TFT_GREEN : TFT_DARKGREY;
  tft.fillCircle(bx + bw - 12, by + 8, 2, dotCol);
  String urlSta = sHasSta ? ("http://" + ipToStr(sStaIp)) : "";
  String urlAp  = "http://" + ipToStr(sApIp);
  String urlShown = (sShowSta && sHasSta) ? urlSta : urlAp;
  String ipShown  = (sShowSta && sHasSta) ? ipToStr(sStaIp) : ipToStr(sApIp);
  String mode     = (sShowSta && sHasSta) ? "STA" : "AP";
  String join     = sApSsid + (sApPass.length()? " (pwd)" : " (open)");
  String dlR=humanRate(sDLRateBps), ulR=humanRate(sULRateBps);
  String dlT=humanBytes(sDLTotal) , ulT=humanBytes(sULTotal);

  {
    int x = rAccessX+6, y = rAccessY+18, w = rAccessW-12;
    if (join != L_join){
      tft.fillRect(x+52, y-1, w-52, 20, TFT_BLACK);
      kv(tft, x, y, "Join",  join, w); y+=10;
      kv(tft, x, y, "Open",  urlAp + "  (no port)", w);
      L_join = join;
    }
  }

  {
    int x = rIfaceX+6, y = rIfaceY+18, w = rIfaceW-12;
    kvUpdate(tft, x, y,   "URL",  L_url,  urlShown, w); y+=10;
    kvUpdate(tft, x, y,   "IP",   L_ip,   ipShown , w); y+=10;
    kvUpdate(tft, x, y,   "Mode", L_mode, (mode + "  (A toggles)"), w);
  }

  {
    int x = rTrafficX+6, y = rTrafficY+18, w = rTrafficW-12;
    kvUpdate(tft, x, y,   "DL",   L_dlRate, (dlR + "  (" + dlT + ")"), w); y+=10;
    kvUpdate(tft, x, y,   "UL",   L_ulRate, (ulR + "  (" + ulT + ")"), w);
    L_dlTot = dlT; L_ulTot = ulT; 
  }

  bool nowSta = (WiFi.status() == WL_CONNECTED);
  if (nowSta != sHasSta || (nowSta && WiFi.localIP() != sStaIp)) {
    sHasSta = nowSta; if (sHasSta) sStaIp = WiFi.localIP();
    int x=rIfaceX+6, y=rIfaceY+18, w=rIfaceW-12;
    tft.fillRect(x, y-1, w, 32, TFT_BLACK);
    L_mode=L_url=L_ip=""; 
  }
}

void webFilesHandleInput(bool a, bool b, bool c, bool& requestExit){
  requestExit = false;
  if (c) { requestExit = true; return; }
  if (a && sHasSta) { sShowSta = !sShowSta; L_mode=L_url=L_ip=""; } 
}
