
var alertModelNode = document.getElementById("alertModal");

function userError(message) {
	//alert(message);
	showAlertWindow("Error", message)
}
function systemError(message) {
	//alert(message);
	showAlertWindow("System error", message)
}

function showAlertWindow(header, message, yesCallback) {
	if(alertModelNode.style.visibility != "visible") {
		var confirmButton = document.getElementById("alertConfirmButton");
		var closebutton = document.getElementById("alertCancelButton");
		var headerNode = document.getElementById("alert-modal-title");
		headerNode.innerText = header;
		var alertModalBody = document.getElementById("alertModalBody");
		if(yesCallback) {
			confirmButton.hidden = false;
			confirmButton.onclick = yesCallback;
			alertModalBody.innerHTML = message;
			closebutton.innerText = "No";
		} else {
			confirmButton.hidden = true;
			if(message)
				alertModalBody.innerHTML = message;
			closebutton.innerText = "Ok";
		}
		alertModelNode.style.visibility = "visible";
		alertModelNode.style.opacity = 1;
	} else {
		systemError("Two alert windows opened");
	}
}
	
function closeAlertWindow() {
	alertModelNode.style.visibility = "hidden";
	alertModelNode.style.opacity = 0;
	setTimeout(function() {
		var confirmButton = document.getElementById("alertConfirmButton");
		confirmButton.onclick = undefined;
		var alertModalBody = document.getElementById("alertModalBody");
		alertModalBody.innerText = undefined;
	}, 275)
}