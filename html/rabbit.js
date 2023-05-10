/*
 * rabbit.js
 *
 * Copyright (C) 2023, Charles Chiou
 */

var websock;

function set_rgb_stream()
{
    document.getElementById("rgb-stream").src =
	window.location.protocol + "//" + document.domain + ":8000/rgb";
}

function rabbit_key_pressed(event)
{
    websock.send(event.key);
}

function rabbit_key_downed(event)
{
    if (event.keyCode == '38') {
	websock.send('8');
    } else if (event.keyCode == '40') {
	websock.send('2');
    } else if (event.keyCode == '39') {
	websock.send('6');
    } else if (event.keyCode == '37') {
	websock.send('4');
    }
}

function rabbit_message(message)
{
    var rabbit_log = document.getElementById("rabbit-log");

    rabbit_log.value += (message.data);
    rabbit_log.scrollTop = rabbit_log.scrollHeight;
}

function rabbit_connected()
{
    var rabbit_log = document.getElementById("rabbit-log");

    rabbit_log.value += ("Established connection to rabbit'bot!\n");
    rabbit_log.scrollTop = rabbit_log.scrollHeight;
    document.getElementById("rabbit-connect").value = "Disconnect";

    set_rgb_stream();
}

function rabbit_disconnected()
{
    var rabbit_log = document.getElementById("rabbit-log");

    rabbit_log.value += ("Lost connection to rabbit'bot!\n");
    rabbit_log.scrollTop = rabbit_log.scrollHeight;
    document.getElementById("rabbit-connect").value = "Connect";
    document.getElementById("rgb-stream").src = "";
}

function rabbit_connect_button_event()
{
    var rabbit_connect = document.getElementById("rabbit-connect");
    var text = rabbit_connect.value;

    if (text == "Connect") {
	websock = new WebSocket("ws://"+ document.domain + ":18888");
	websock.onclose = rabbit_disconnected;
	websock.onopen = rabbit_connected;
	websock.onmessage = rabbit_message;
    } else {
	websock.close();
    }
}

function rabbit_refresh_button_event()
{
    if (websock.readyState !== WebSocket.CLOSED) {
	set_rgb_stream();
    }
}

function rabbit_logclear_button_event()
{
    var rabbit_log = document.getElementById("rabbit-log");

    rabbit_log.value = "";
}

function rabbit_wheel_fwl()
{
    websock.send('w7');
}

function rabbit_wheel_fwd()
{
    websock.send('w8');
}

function rabbit_wheel_fwr()
{
    websock.send('w9');
}

function rabbit_wheel_rol()
{
    websock.send('w4');
}

function rabbit_wheel_halt()
{
    websock.send('h');
}

function rabbit_wheel_ror()
{
    websock.send('w6');
}

function rabbit_wheel_bwl()
{
    websock.send('w1');
}

function rabbit_wheel_bwd()
{
    websock.send('w2');
}

function rabbit_wheel_bwr()
{
    websock.send('w3');
}

function rabbit_keys_setup()
{
    document.getElementById("rabbit-connect").onclick =
	rabbit_connect_button_event;
    document.getElementById("rabbit-refresh").onclick =
	rabbit_refresh_button_event;
    document.getElementById("rabbit-logclear").onclick =
	rabbit_logclear_button_event;

    document.getElementById("wheel-fwl").onclick =
	rabbit_wheel_fwl;
    document.getElementById("wheel-fwd").onclick =
	rabbit_wheel_fwd;
    document.getElementById("wheel-fwr").onclick =
	rabbit_wheel_fwr;
    document.getElementById("wheel-rol").onclick =
	rabbit_wheel_rol;
    document.getElementById("wheel-halt").onclick =
	rabbit_wheel_halt;
    document.getElementById("wheel-ror").onclick =
	rabbit_wheel_ror;
    document.getElementById("wheel-bwl").onclick =
	rabbit_wheel_bwl;
    document.getElementById("wheel-bwd").onclick =
	rabbit_wheel_bwd;
    document.getElementById("wheel-bwr").onclick =
	rabbit_wheel_bwr;
}

function rabbit_setup()
{
    if (window.WebSocket) {
	websock = new WebSocket("ws://"+ document.domain + ":18888");
	websock.onopen = rabbit_connected;
	websock.onclose = rabbit_disconnected;
	websock.onmessage = rabbit_message;
	document.addEventListener("keypress", rabbit_key_pressed);
	document.addEventListener("keydown", rabbit_key_downed);
	rabbit_keys_setup();
    } else {
	alert("Browser does not support WebSocket!");
    }

    set_rgb_stream();
}
