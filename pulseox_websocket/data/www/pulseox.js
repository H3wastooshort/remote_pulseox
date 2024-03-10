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

var minmax_smooth = 0.99;
var mid_smooth = 0.99;

const stretch = 8;

var red_mid = 40000;
var ir_mid = 40000;
function ws_msg(e) {
	msg_cnt++;
	//console.log(e.data);
	let msg = JSON.parse(e.data);
	if (msg.type != 'status') console.log(msg);

	if (typeof msg.ir == 'number' && typeof msg.red == 'number') {
		const center = cnv.height / 2;
		
		if (Math.abs(msg.ir-ir_mid) > 1000) ir_mid = msg.ir;
		if (Math.abs(msg.red-red_mid) > 1000) red_mid = msg.red;
		
		red_mid=red_mid*mid_smooth + msg.red*(1-mid_smooth);
		ir_mid=ir_mid*mid_smooth + msg.ir*(1-mid_smooth);
		
		let ir = msg.ir - ir_mid;
		let red = msg.red - red_mid;
		
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
		
		ir_bar.min = -100; //Math.min(msg.ir,ir_bar.min)*minmax_smooth + msg.ir*(1-minmax_smooth);
		red_bar.min = -100; // Math.min(msg.red,red_bar.min)*minmax_smooth + msg.red*(1-minmax_smooth);
		ir_bar.max = 100; // Math.max(msg.ir,ir_bar.max)*minmax_smooth + msg.ir*(1-minmax_smooth);
		red_bar.max = 100; // Math.max(msg.red,red_bar.max)*minmax_smooth + msg.red*(1-minmax_smooth);
	}
	
	if (typeof msg.bpm == 'number') {
		bpm_text.innerText = msg.bpm;
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