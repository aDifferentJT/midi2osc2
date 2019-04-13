const socket = new WebSocket('ws://192.168.1.122:8080');

socket.addEventListener('message', function (event) {
  msg_rx(event.data);
});

socket.onclose = function(event) {
  console.log("WebSocket is closed now.");
};

function msg_rx(message){
  var parts = message.split(":");

  if(parts[0]=="moved"){
    update_control(parts);
  }else if(parts[0] == "bank"){
    bank_update(parts);
  }
}

var lastmoved =""; 

function update_control(parts){
  var label = document.getElementById("movedcontrol");
  var value = document.getElementById("movedcontrolvalue");
  var cg =  document.getElementById("movedcontrolchannel");
  var ag =  document.getElementById("movedcontrolaction");
  var device = document.getElementById("movedcontroldevice");
  var output = document.getElementById("movedcontroloutput");
  label.innerHTML = parts[1];
  value.innerHTML = parts[2];
  output.innerHTML = parts[4];
  device.innerHTML = parts[3];
  //cg.innerHTML = parts[3];
  //ag.innerHTML = parts[4];

  lastmoved = parts[1];
}

function bank_update(parts){
  var bankLabel =  document.getElementById("bank");

  bankLabel.innerHTML = parts[1];
}

var edit_control_name = "";

function edit_mode(){
  edit_control_name = lastmoved;
  var output = document.getElementById("movedcontroloutput");
  var edit_output = document.getElementById("edit-output");

  document.getElementById("edit-output").value = document.getElementById("movedcontroloutput").innerHTML;
   document.getElementById("edit-device").value = document.getElementById("movedcontroldevice").innerHTML;
  document.getElementById("editcontrol").innerHTML = edit_control_name;
}

function set_control_output(){
  var device = document.getElementById("edit-device");
  var output = document.getElementById("edit-output");
  socket.send("setControl:"+edit_control_name+":"+device.value+":"+output.value);
}
