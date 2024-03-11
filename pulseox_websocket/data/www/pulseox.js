function ws_open(e) {
	ws_text.innerText="Connected";
	ws_text.style.color='#0F0';
}
function ws_err(e) {
	ws_text.innerText="ERROR";
	ws_text.style.color='#F00';
}
function ws_closed(e) {
	if (ws_text.innerText != "ERROR") ws_text.innerText="Disconnected";
	ws_text.style.color='#F00';
	setTimeout(ws_connect,10000);
}

var msg_cnt=0;
setInterval (()=>{
	pps_text.innerText=msg_cnt;
	msg_cnt=0;
},1000);

const cnv = document.getElementById('data_plot');
const ctx = cnv.getContext('2d');
ctx.strokeStyle='white';
ctx.fillStyle='black';

var last_ir=0;
var last_red=0;
var cnv_step = 0;

var stretch = 8;

var detune_scale = 5;

function ws_msg(e) {
	msg_cnt++;
	//console.log(e.data);
	let msg = JSON.parse(e.data);
	if (msg.type != 'status') console.log(msg);

	if (typeof msg.ir == 'number' && typeof msg.red == 'number') {
		const center = cnv.height / 2;
		let ir = msg.ir;
		let red = msg.red;
		
		osc.detune.value=(ir-last_ir)*detune_scale;
		
		ctx.fillRect(cnv_step,0,stretch,cnv.height);
		
		ctx.strokeStyle='#f00';
		ctx.beginPath();
		ctx.moveTo(cnv_step-stretch,last_ir+center);
		ctx.lineTo(cnv_step,ir+center);
		ctx.stroke();
		ctx.strokeStyle='#f0f';
		ctx.beginPath();
		ctx.moveTo(cnv_step-stretch,last_red+center);
		ctx.lineTo(cnv_step,red+center);
		ctx.stroke();
		
		cnv_step+=stretch;
		cnv_step = cnv_step % cnv.width;
		
		last_ir = ir_bar.value = ir;
		//ir_txt.innerText = msg.ir;
		last_red = red_bar.value = red;
		//red_txt.innerText = msg.red;
		
		//hist[hist_idx] = [msg.ir,msg.red];
		//hist_idx = ++hist_idx % max_hist;
		
		
		if (enable_log.checked) data_log.value+=ir+","+red+"\n";
	}
	
	if (typeof msg.bpm == 'number') {
		bpm_text.innerText = msg.bpm;
	}
	if (typeof msg.rssi == 'number') {
		rssi_text.innerText = msg.rssi;
	}
	if (typeof msg.temp == 'number') {
		temp_text.innerText = msg.temp;
	}
}

var ws;
function ws_connect() {
	ws = new WebSocket("ws://"+location.host+"/ws");
	ws.addEventListener("message", ws_msg);
	ws.addEventListener("open", ws_open);
	ws.addEventListener("close", ws_closed);
	ws.addEventListener("error", ws_err);
	
	ws_text.innerText="Connecting...";
	ws_text.style.color='#0FF';
}
ws_connect();



const ac = new (window.AudioContext || window.webkitAudioContext)();
const osc = ac.createOscillator();
osc.frequency.value=100;
osc.detune.maxValue=2000;
osc.connect(ac.destination);
osc.start();
