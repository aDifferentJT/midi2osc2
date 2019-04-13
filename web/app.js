var controls = [];
const socket = new WebSocket('ws://192.168.1.122:8080');

socket.addEventListener('message', function (event) {
  msg_rx(event.data);
});

socket.onclose = function(event) {
  console.log("WebSocket is closed now.");
};

//3 element data structure type=key=value1=value2
//defined types are control to set a controls value, output to set a control to output mapping
function msg_rx(message){
  var parts = message.split(":");

  if(parts[0]=="moved"){
    update_control(parts);
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



//processes messages which create controls which have 4 parts control=ctlname=value=definedoutput
//for updates the definedoutput can be omitted
function update_table(parts){
  var tbl = document.getElementById("controls");
  var ctlname = parts[1];
  var value = parts[2];
  var output = "";
  if(parts.length>=4){
    output = parts[3];
  }

  if(controls.includes(ctlname)){
    //update existing control
    //get the row id
    var rowid = document.getElementById("ctl-"+ctlname).rowIndex;
    var valueCell = tbl.rows[rowid].cells[1];
    valueCell.innerHTML = value;
    if(parts.length>=4){
      var outputCell = tbl.rows[rowid].cells[2];
      var input_out = outputCell.getElementsByTagName("input")[0];
      input_out.value=output;
      outputCell.setAttribute("data-cur", output);
    }
  }else{
    //control is not currently defined, define new control
    controls.push(ctlname);

    var row = tbl.insertRow(-1);

    row.id="ctl-"+ctlname;
    row.setAttribute("data-ctlname", ctlname);

    var nameCell = row.insertCell(0);
    nameCell.innerHTML=ctlname;
    var valueCell = row.insertCell(1);
    valueCell.innerHTML=value;
    var outputCell = row.insertCell(2);
    var input_out = document.createElement("input");
    input_out.value=output;
    outputCell.appendChild(input_out);

    outputCell.setAttribute("data-cur", output);
  }
}

function apply_outputs(){
  var tbl = document.getElementById("controls");

  for(var i=1; i<tbl.rows.length; i++){
    var row = tbl.rows[i];

    var ctlname = row.getAttribute("data-ctlname");

    var outputCell = row.cells[2];
    var input_out = outputCell.getElementsByTagName("input")[0];

    var current = outputCell.getAttribute("data-cur");

    if(current != input_out.value){
      //value changed
      tx_output_update(ctlname, input_out.value);
      outputCell.setAttribute("data-cur", input_out.value);
    }
  }
}

function tx_output_update(ctlname, output){
  console.log("output="+ctlname+"="+output);
}
