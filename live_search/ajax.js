// global varibalbe to keep track the request
// and funciton clal back when done
var ajaxreq = false, ajaxCallback;

// ajaxRequest :setup q request
function ajaxRequest(filename) {
	try {
		// make a new request obj
		ajaxreq = new XMLHttpRequest();

	} catch (error) {
		return false;
	}

	ajaxreq.open("GET",filename);
	console.log("ajaxRequest called\n");
	ajaxreq.onreadystatechange = ajaxResponse;
	ajaxreq.send(null);
}

// ajaxResponse: Wait for response and calls a function
function ajaxResponse() {
	if(ajaxreq.readyState !=4) return;
	if(ajaxreq.status ==200) {
		// if the request sucessed...
	if( ajaxCallback) ajaxCallback();
	console.log("status =200 =\n");

	} else alert ("request failed" + ajaxreq.statusText);
	console.log("statusRequest ="+ajaxreq.statusText);
	return true;
}
