var socket;
var hostname = "";
var connectId = -1;
var connected = false;

function initial_load() {
  if (window.location.protocol == "file:") {
    hostname = "localhost";
  } else {
    hostname = window.location.hostname;
  }

  document.getElementById("cnn-hostname").value = hostname;

  connectId = setInterval(connect, 1000);
}

function connect_form() {
  if(!connected){
    //terminate any existing connection attempts
    socket.close();
    hostname = document.getElementById('cnn-hostname').value;
    show_spinners();

    clearInterval(connectId);
    connectId = setInterval(connect, 1000);
  }
  return false;
}

function connect() {
  if (connected) {
    return;
  }
  socket = new WebSocket(`ws://${hostname}:8080`);

  socket.addEventListener('message', function (event) {
    msg_rx(event.data);
  });

  socket.onopen = function(event) {
    connected = true;
    clearInterval(connectId);
    update_status("Connected");
    document.getElementById("cnn-form").className = "hidden";
    document.getElementById("main").className="";
    hide_spinners();

    //register on close after on openned
    socket.onclose = function(event) {
      update_status("Disconnected");
      console.log("disconnect shown");
      //clear any active edits to ensure that if a reconnect occurs, sync issues do not occur.
      cancel_edit_mode();
      disable_controls();
      document.getElementById("cnn-form").className = "";
      if (connected) {
        connected = false;
        clearInterval(connectId);
        connectId = setInterval(connect, 1000);
        show_spinners();
        document.getElementById("main").className="hidden";
      }
    };
  };
}

function msg_rx(message) {
  var parts = message.split(":");
  if (parts[0] == "moved") {
    update_control(parts);
  } else if (parts[0] == "bank") {
    update_bank(parts);
  } else if (parts[0] == "devices") {
    update_devices(parts.slice(1));
  } else if (parts[0] == "echo") {
    chat_rx(message);
  } else if (parts[0] == "enableBank") {
    enable_bank(parts[1], true);
  } else if (parts[0] == "disableBank") {
    enable_bank(parts[1], false);
  } else if (parts[0] == "mock") {
    enable_mock();
  } else if (parts[0] == "mockSetLed") {
    mock_set_led(parts[1], parts[2]);
  } else {
    console.log(`Unrecognised command: ${message}`);
  }
}

var lastmovedcontrol = "";
var lastmovedchannel = "";
var lastmovedaction = "";

function update_control(parts) {
  var label = document.getElementById("movedcontrol");
  var value = document.getElementById("movedcontrolvalue");
  var device = document.getElementById("movedcontroldevice");
  var output = document.getElementById("movedcontroloutput");
  var inverted = document.getElementById("movedcontrolinverted");
  var cg =  document.getElementById("movedcontrolchannel");
  var cgd =  document.getElementById("movedcontrolcgdevice");
  var cgo =  document.getElementById("movedcontrolcgoutput");
  var ag =  document.getElementById("movedcontrolaction");
  var ago =  document.getElementById("movedcontrolagoutput");
  label.innerHTML = parts[1];
  if(parts[2] != "unchanged"){
    value.innerHTML = parts[2];
  }
  device.innerHTML = parts[3];
  output.innerHTML = parts[4];
  inverted.innerHTML = parts[5];
  cg.innerHTML = parts[6];
  cgd.innerHTML = parts[7];
  cgo.innerHTML = parts[8];
  ag.innerHTML = parts[9];
  ago.innerHTML = parts[10];

  lastmovedcontrol = parts[1];
  lastmovedchannel = parts[6];
  lastmovedaction = parts[9];

  document.getElementById("enteredit").disabled = false;
}

function update_status(status) {
  document.getElementById("status").innerHTML = status;
}

function update_bank(parts){
  update_status(`Bank ${parts[1]}`);
  document.getElementById("enteredit").disabled = true;
  cancel_edit_mode();
}

function disable_controls(){
  ["bank-left", "bank-right", "enteredit"].forEach(control => {
    document.getElementById(control).disabled = true;
  });
}

function enable_bankswitch(){
  ["bank-left", "bank-right"].forEach(control => {
    document.getElementById(control).disabled = false;
  });
}

function show_spinners(){
  ["spinner1"].forEach(control => {
    document.getElementById(control).className = "";
  });
}

function hide_spinners(){
  ["spinner1"].forEach(control => {
    document.getElementById(control).className = "hidden";
  });
}

function update_devices(devices) {
  ["edit-device", "edit-cg-device"].forEach( fieldId => {
    var field = document.getElementById(fieldId);
    while (field.options.length > 0) {
      field.options.remove(0);
    }
    devices.forEach( device => {
      var element = document.createElement("option")
      element.value = device;
      element.innerHTML = device;
      field.options.add(element);
    });
  });
}

var edit_control_name = "";
var edit_channel_name = "";
var edit_action_name = "";

var in_edit_mode=0;

function edit_mode() {
  edit_control_name = lastmovedcontrol;
  edit_channel_name = lastmovedchannel;
  edit_action_name = lastmovedaction;

  in_edit_mode = 1;

  var output = document.getElementById("movedcontroloutput");
  var edit_output = document.getElementById("edit-output");

  document.getElementById("editcontrol").innerHTML = edit_control_name;
  document.getElementById("edit-device").value = document.getElementById("movedcontroldevice").innerHTML;
  document.getElementById("edit-output").value = document.getElementById("movedcontroloutput").innerHTML;
  document.getElementById("edit-inverted").checked = document.getElementById("movedcontrolinverted").innerHTML == "true";
  document.getElementById("editchannel").innerHTML = edit_channel_name;
  document.getElementById("edit-cg-device").value = document.getElementById("movedcontrolcgdevice").innerHTML;
  var chparams = document.getElementById("movedcontrolcgoutput").innerHTML.split("$");
  document.getElementById("edit-cg-sel").value = chparams[0];
  channel_selection_changed();
  if (chparams.length>1) {
    document.getElementById("edit-cg-opt").value = chparams[1];
  } else {
    document.getElementById("edit-cg-opt").value = "";
  }
  document.getElementById("editaction").innerHTML = edit_action_name;
  document.getElementById("edit-action").value = document.getElementById("movedcontrolagoutput").innerHTML;

  document.getElementById("editbox").className = "show";
  document.getElementById("editmode-button").className = "hidden";
}

function cancel_edit_mode() {
  document.getElementById("editbox").className = "hidden";
  document.getElementById("edit-cg-opt").className = "hidden";
  document.getElementById("editmode-button").className = "show";
  in_edit_mode = 0;
}

function set_control_output() {
  var device = document.getElementById("edit-device").value;
  var output = document.getElementById("edit-output").value;
  var inverted = document.getElementById("edit-inverted").checked;
  socket.send(`setControl:${edit_control_name}:${device}:${output}:${inverted}`);
  if (edit_control_name == lastmovedcontrol) {
    document.getElementById("movedcontroldevice").innerHTML = device;
    document.getElementById("movedcontroloutput").innerHTML = output;
    document.getElementById("movedcontrolinverted").innerHTML = inverted;
  }
  cancel_edit_mode();
  return 0;
}

function clear_output() {
  socket.send(`clearControl:${edit_control_name}`);
  if (edit_control_name == lastmovedcontrol) {
    document.getElementById("movedcontroldevice").innerHTML = "";
    document.getElementById("movedcontroloutput").innerHTML = "";
    document.getElementById("movedcontrolinverted").innerHTML = "";
  }
  cancel_edit_mode();
  return 0;
}

function channel_selection_changed() {
  var sel = document.getElementById("edit-cg-sel").value;
  if (sel=="input") {
    document.getElementById("edit-cg-opt").className = "show";
  } else {
    document.getElementById("edit-cg-opt").value = "";
    document.getElementById("edit-cg-opt").className = "hidden";
  }
}

function set_channel() {
  var device = document.getElementById("edit-cg-device");
  var sel = document.getElementById("edit-cg-sel");
  var opt = document.getElementById("edit-cg-opt");
  socket.send(`setChannel:${edit_channel_name}:${device.value}:${sel.value}$${opt.value}`);
  if (edit_channel_name == lastmovedchannel) {
    document.getElementById("movedcontrolcgdevice").innerHTML = device.value;
    document.getElementById("movedcontrolcgoutput").innerHTML = `${sel.value}$${opt.value}`;
  }
  cancel_edit_mode();
  return 0;
}

function clear_channel() {
  if (edit_channel_name == lastmovedchannel) {
    document.getElementById("movedcontrolcgdevice").innerHTML = "";
    document.getElementById("movedcontrolcgoutput").innerHTML = "";
  }
  socket.send(`clearChannel:${edit_channel_name}`);
  cancel_edit_mode();
  return 0;
}

function set_action() {
  var act = document.getElementById("edit-action");
  socket.send(`setAction:${edit_action_name}:${act.value}`);
  if (edit_action_name == lastmovedaction) {
    document.getElementById("movedcontrolagoutput").innerHTML = act.value;
  }
  cancel_edit_mode();
  return 0;
}

function clear_action() {
  socket.send(`clearAction:${edit_action_name}`);
  if (edit_action_name == lastmovedaction) {
    document.getElementById("movedcontrolagoutput").innerHTML = "";
  }
  cancel_edit_mode();
  return 0;
}

function chat_rx(full_msg) {
  document.getElementById("chatdiv").className="pullbottom";
  document.getElementById("chat").innerHTML = full_msg.substring(full_msg.indexOf("echo:")+5);
}

function chat_tx() {
  var txt = document.getElementById("chat-send");
  socket.send(`echo:${txt.value}`);
  txt.value="";
}

function bank_change(direction) {
  socket.send(`bankChange:${direction}:${lastmovedcontrol}`)
}

function enable_bank(bank, value) {
  if (bank == "left") {
    document.getElementById("bank-left").disabled = !value;
  } else if (bank == "right") {
    document.getElementById("bank-right").disabled = !value;
  }
}

function enable_mock() {
  document.getElementById("mock-div").className = "show";
}

function mock_moved() {
  control = document.getElementById("mock-moved-control").value;
  value = document.getElementById("mock-moved-value").value;
  socket.send(`mockMoved:${control}:${value}`);
}

function mock_set_led(control, value) {
  document.getElementById("mock-led").innerHTML = `LED ${control}: ${value}`;
}
