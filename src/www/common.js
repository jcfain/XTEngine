
var alertModelNode = document.getElementById("alertModal");
var textModelNode = document.getElementById("textModal");

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

function showGetTextWindow(header, message, yesCallback) {
	if(textModelNode.style.visibility != "visible") {
		var textModalInput = document.getElementById("textModalInput");
		var headerNode = document.getElementById("text-modal-title");
		headerNode.innerText = header;
		var textModalLabel = document.getElementById("textModalLabel");
		textModalLabel.innerHTML = message;
		var confirmButton = document.getElementById("textConfirmButton");
		confirmButton.onclick = function(textModalInput) { 
			yesCallback(textModalInput.value);
		}.bind(textModalInput, textModalInput);
		textModelNode.style.visibility = "visible";
		textModelNode.style.opacity = 1;
	} else {
		systemError("Two text windows opened");
	}
}
	
function closeTextWindow() {
	textModelNode.style.visibility = "hidden";
	textModelNode.style.opacity = 0;
	setTimeout(function() {
		var confirmButton = document.getElementById("textConfirmButton");
		confirmButton.onclick = undefined;
		var textModalInput = document.getElementById("textModalInput");
		textModalInput.value = "";
	}, 275)
}

function createCheckBoxDiv(id, name, value, labelText, onCheckedChange) {
	const divNode = document.createElement("div");
	const label = document.createElement("label");
	label.innerText = labelText;
	label.setAttribute("for", id);
	const checkbox = document.createElement("input");
	checkbox.type = "checkbox";
	checkbox.value = value;
	//checkbox.classList.add("styled-checkbox");
	checkbox.id = id;
	checkbox.name = name;
	checkbox.onclick = onCheckedChange(checkbox);
	divNode.appendChild(checkbox);
	divNode.appendChild(label);
	return divNode;
}