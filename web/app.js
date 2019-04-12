var controls = [];

//3 element data structure type=key=value1=value2
//defined types are control to set a controls value, output to set a control to output mapping
function msg_rx(message){
  var parts = message.split("=");

  if(parts[0]=="control"){
    update_table(parts);
  }
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
