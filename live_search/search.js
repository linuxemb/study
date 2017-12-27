// global variable to manager the itmeout 
var t;
// start a timeout wiht each key press
function startSearch() {
	if(t) window.clearTimeout(t);
	t = window.setTimeout("liveSearch()", 200);
}

// Perform the search
function liveSearch() {
	// assemble the php file name
	query = document.getElementById("searchlive").value;
	filename = "serch.php?query="+query;
	// displayresults will handle the Ajax response
	ajaxCallback = displayResults;
	//Send Ajax request
	ajaxRequest(filename);
	}

// Display search results

function displayResults() {
	// remove old list
	console.log("display REsult \n");
	u1 = document.getElementById("list");
	div = document.getElementById("results");
	div.removeChild(u1);

	//make a new list
	u1 = document.creatElement("u1");
	u1.id="list";
	names = ajaxreq.responseXML.getElementsByTagName('name');

	for (var i = 0; i< names.length; i++) {

		li=document.createElement("li");
		name = names[i].firstChild.nodeValue;
		text = document.createTextNode(name);
		li.appendChild(text);
		u1.appendChild(li);
	}

	if (name.length==0) {
	    li=document.createElement("li");
        text = document.createTextNode("no results");
        u1.appendChild(li);

	}
// display the new list
	div.appendChild(u1);

	
}

// set up event handler
obj = document.getElementById("searchlive");
obj.onkeydown = startSearch;
