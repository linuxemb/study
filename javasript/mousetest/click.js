//Get the current date

var now = new Date();

//Delineate hours, minutes, seconds

hour_of_day = now.getHours();
minute_of_hour = now.getMinutes();
seconds_of_minute = now.getSeconds();

// Display the time
document.write("<h2>");
document.write(hour_of_day + ":" + minute_of_hour + ":"+ seconds_of_minute);
document.write("</h2");


function mouseStatus(e)

{
	if(!e) e=window.event;
	btn = e.button;
	whichone = (btn< 2) ? "Left": "Right";
	message=e.type + ":" + whichone + "<br>";
	document.getElementById('testarea').innerHTML += message;
}

obj= document.getElementById('testlink');
obj.onmousedown = mouseStatus;
obj.onmouseup = mouseStatus;
obj.onclick = mouseStatus;
obj.ondblclick =mouseStatus;