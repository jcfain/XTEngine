
var alertModelNode = document.getElementById("alertModal");
var textModelNode = document.getElementById("textModal");

function userError(message) {
	showAlertWindow("Error", message);
}
function systemError(message) {
	showAlertWindow("System error", message);
}
function userWarning(message) {
	showAlertWindow("Warning", message);
}
function systemWarning(message) {
	showAlertWindow("System warning", message);
}
function systemSuccess(message) {
	showAlertWindow("Success!", message);
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
		if(alertModelNode.style.visibility != "visible") {// Already been replaced. Avoids race condition when closing one alert to open another after.
			var confirmButton = document.getElementById("alertConfirmButton");
			confirmButton.onclick = undefined;
			var alertModalBody = document.getElementById("alertModalBody");
			alertModalBody.innerText = undefined;
		}
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

function scale(number, inMin, inMax, outMin, outMax) {
    return (((number - inMin) * (outMax - outMin)) / (inMax - inMin)) + outMin;
}

function getChannelColor(channel) {
	switch (channel)
	{
		case "L0":
			return "red";
		case "L1":
			return "yellow";
		case "L2":
			return "orange";
		case "R0":
			return "blue";
		case "R1":
			return "green";
		case "R2":
			return "purple";
		case "V0":
			return "pink";
		case "V1":
			return "aliceblue";
		case "A0":
			return "cyan";
		case "A1":
			return "magenta";
		default:
			return "white"
	}
}

function lerp(startValue, targetValue, timeScale) {
	//return startValue*timeScale + targetValue*(1-timeScale);
	return startValue + (targetValue - startValue) * timeScale;
}

function millis(lastTime = 0) {
	return Date.now() - lastTime;
}

function round(value, precision) {
    var multiplier = Math.pow(10, precision || 0);
    return Math.round(value * multiplier) / multiplier;
}

function capitalizeFirstLetter(val) {
    return String(val).charAt(0).toUpperCase() + String(val).slice(1);
}