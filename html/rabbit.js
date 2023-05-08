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

function rabbit_keyevent(event)
{
    websock.send(event.key);
}

function rabbit_setup()
{
    websock = new WebSocket("ws://"+ document.domain + ":18888");
    websock.onclose = function() {
	alert("Lost control to rabbit!");
    };
    window.addEventListener("keypress", rabbit_keyevent);
}
