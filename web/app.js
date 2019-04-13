const socket = new WebSocket('ws://169.254.7.139:8080');

socket.addEventListener('message', function (event) {
  msg_rx(event.data);
});

socket.onclose = function(event) {
  update_status("Disconnected");
};

socket.onopen = function(event) {
  update_status("Connected");
};

function msg_rx(message){
  var parts = message.split(":");

  if(parts[0]=="moved"){
    update_control(parts);
  }else if(parts[0] == "bank"){
    update_status("Bank "+parts[1]);
  }else if(parts[0]=="echo"){
    chat_rx(message);
  }
}

var lastmoved ="";

function update_control(parts){
  var label = document.getElementById("movedcontrol");
  var value = document.getElementById("movedcontrolvalue");
  var cgd =  document.getElementById("movedcontrolchdevice");
  var cg =  document.getElementById("movedcontrolchannel");
  var ag =  document.getElementById("movedcontrolaction");
  var device = document.getElementById("movedcontroldevice");
  var output = document.getElementById("movedcontroloutput");
  label.innerHTML = parts[1];
  value.innerHTML = parts[2];
  device.innerHTML = parts[3];
  output.innerHTML = parts[4];
  //cg.innerHTML = parts[3];
  //ag.innerHTML = parts[4];

  lastmoved = parts[1];
}

function update_status(status){
  var statusLabel = document.getElementById("status");

  statusLabel.innerHTML = status;
}

var edit_control_name = "";

function edit_mode(){
  edit_control_name = lastmoved;
  var output = document.getElementById("movedcontroloutput");
  var edit_output = document.getElementById("edit-output");

  document.getElementById("edit-output").value = document.getElementById("movedcontroloutput").innerHTML;
  document.getElementById("edit-device").value = document.getElementById("movedcontroldevice").innerHTML;
  document.getElementById("edit-ch-device").value = document.getElementById("movedcontrolchdevice").innerHTML;
  document.getElementById("editcontrol").innerHTML = edit_control_name;
  document.getElementById("edit-action").value = document.getElementById("movedcontrolaction").innerHTML;

  var chparams = document.getElementById("movedcontrolchannel").innerHTML.split("$");
  document.getElementById("edit-ch-sel").value = chparams[0];
  if(chparams.length>1){
    document.getElementById("edit-ch-opt").value = chparams[1];
  }



  document.getElementById("editbox").className = "show";
}

function cancel_edit_mode(){
  document.getElementById("editbox").className = "hidden";
}

function set_control_output(){
  var device = document.getElementById("edit-device");
  var output = document.getElementById("edit-output");
  socket.send("setControl:"+edit_control_name+":"+device.value+":"+output.value);

  if(edit_control_name == lastmoved){
    document.getElementById("movedcontroldevice").innerHTML = device.value;
    document.getElementById("movedcontroloutput").innerHTML = output.value;
  }

  cancel_edit_mode();

  return 0;
}

function clear_output(){
  socket.send("setControl:"+edit_control_name+"::");
  if(edit_control_name == lastmoved){
    document.getElementById("movedcontroldevice").innerHTML = "";
    document.getElementById("movedcontroloutput").innerHTML = "";
  }

  cancel_edit_mode();
  return 0;
}

function channel_selection_changed(){
  var sel = document.getElementById("edit-ch-sel").value;

  if(sel=="input"){
    document.getElementById("edit-ch-opt").className = "show";
  }else{
    document.getElementById("edit-ch-opt").value = "";
    document.getElementById("edit-ch-opt").className = "hidden";
  }
}

function set_channel(){
  var device = document.getElementById("edit-ch-device");
  var sel = document.getElementById("edit-ch-sel");
  var opt = document.getElementById("edit-ch-opt");

  socket.send("setChannel:"+edit_control_name+":"+device.value+":"+sel.value+":"+opt.value);

  if(edit_control_name == lastmoved){
    document.getElementById("movedcontrolchdevice").innerHTML = device.value;
    document.getElementById("movedcontrolchannel").innerHTML = sel.value+"$"+opt.value;
  }

  cancel_edit_mode();

  return 0;
}

function clear_channel(){
  if(edit_control_name == lastmoved){
    document.getElementById("movedcontrolchdevice").innerHTML = "";
    document.getElementById("movedcontrolchannel").innerHTML = "";
  }
  socket.send("setChannel:"+edit_control_name+":::");

  cancel_edit_mode();

  return 0;
}

function set_action(){
  var act = document.getElementById("edit-action");

  socket.send("setAction:"+edit_control_name+":"+act.value);
  if(edit_control_name == lastmoved){
    document.getElementById("movedcontrolaction").innerHTML = act.value;
  }

  cancel_edit_mode();
  return 0;
}

function clear_action(){
  socket.send("setAction:"+edit_control_name+":");
  if(edit_control_name == lastmoved){
    document.getElementById("movedcontrolaction").innerHTML = "";
  }

  cancel_edit_mode();
  return 0;
}

function chat_rx(full_msg){
  document.getElementById("chat").innerHTML = full_msg.substring(full_msg.indexOf("echo:")+5);
}

function chat_tx(){
  socket.send("echo:"+document.getElementById("chat-send").value);
}
