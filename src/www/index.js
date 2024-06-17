const webVersion = "v0.455b";

var DeviceType = {
	Serial: 0,
	Network: 1,
	Deo: 2,
	Whirligig: 3,
	Gamepad: 4,
	XTPWeb: 5,
	None: 6
};

var ConnectionStatus = {
	Connected: 0,
	Disconnected: 1,
	Connecting: 2,
	Error: 3
};

var ChannelType = {
	None: 0,
	Range: 1,
	Switch: 2,
	HalfRange: 3
};

var AxisDimension = {
	None: 0,
	Heave: 1,
	Surge: 2,
	Sway: 3,
	Pitch: 4,
	Roll: 5,
	Yaw: 6
};

var MediaType = {
    PlaylistInternal: 0,
    Video: 1,
    Audio: 2,
    FunscriptType: 3,
    VR: 4
};

var Roles = {
	DisplayRole: 0,
	DecorationRole: 1,
	EditRole: 2,
	ToolTipRole: 3,
	StatusTipRole: 4,
	WhatsThisRole: 5,
	// Metadata
	FontRole: 6,
	TextAlignmentRole: 7,
	BackgroundRole: 8,
	ForegroundRole: 9
};

var mediaLoading = false;
var debounceTracker = {};// Use to create timeout handles on the fly.
var wsUri;
var websocket = null;
var xtpConnected = false;
var xtpFormDirty = false;
var userAgent;
var remoteUserSettings;
var mediaListGlobal = [];
var mediaListDisplayed = [];
var selectedMediaItemMetaData = null;
var playingmediaItem;
var playingmediaItemNode;
var webSocket;
var deviceAddress;
var funscriptChannels = [];
//var loadedFunscripts;
var currentChannelIndex = 0;
var outputConnectionStatus = ConnectionStatus.Disconnected;
var selectedInputDevice;
var selectedOutputDevice;
var deviceConnectionStatusInterval;
var serverRetryTimeout;
var serverRetryTimeoutTries = 0;
var videoStallTimeout;
var userFilterCriteria;
var userTagFilterCriteria = [];
var filterDebounce;
var enableTextToSpeech = true;
var systemVoices = [];
var speechNotInbrowser = false;
var selectedVoiceIndex = 0;
var speechPitch = 5;
var speechRate = 5;
var speechVolume = 0.5;
var rangeChangeAmount = 100;

var shufflePlayMode = false;
var shufflePlayModePlayedIndexed = {};

const tagCheckboxesName = "TagCheckbox";
const metadataTagCheckboxesName = "MetadataTagCheckbox";
//var funscriptSyncWorker;
//var useDeoWeb;
//var deoVideoNode;
//var deoSourceNode;


const parseCookie = str =>
str
.split(';')
.map(v => v.split('='))
.reduce((acc, v) => {
	if(v && v[0].trim().length)
		acc[decodeURIComponent(v[0].trim())] = decodeURIComponent(v[1].trim());
return acc;
}, {});

function delete_cookie( name, path, domain ) {
	if( get_cookie( name ) ) {
		document.cookie = name + "=" +
		((path) ? ";path="+path:"")+
		((domain)?";domain="+domain:"") +
		";expires=Thu, 01 Jan 1970 00:00:01 GMT";
	}
}

function get_cookie(name) {
    return document.cookie.split(';').some(c => {
        return c.trim().startsWith(name + '=');
    });
}

document.addEventListener('keyup', keyboardShortcuts);
document.addEventListener('click', destroyMediaContextNode);

var cookies = parseCookie(document.cookie);
if(!cookies.sessionID) {
	document.getElementById("logoutSection").style.display = "none";
	document.getElementById("logoutSectionSpacing").style.display = "none";
	document.getElementById("sessionInfoTable").hidden = true;
}
getBrowserInformation();
var userAgentIsDeo = userAgent.indexOf("Deo VR") != -1;
var userAgentIsHereSphere = userAgent.indexOf("HereSphere") != -1;
var userAgentIsMobile = userAgent.indexOf("Mobile") != -1;
setDeoStyles(userAgentIsDeo);
var settingsNode = document.getElementById("settingsModal");
var mediaItemSettingsModalNode = document.getElementById("mediaItemSettingsModal");
var thumbsContainerNode = document.getElementById("thumbsContainer");
var videoMediaName = document.getElementById("videoMediaName");

const storedHash = window.localStorage.getItem("storedHash");
var sortByGlobal = JSON.parse(window.localStorage.getItem("sortBy"));
var showGlobal = JSON.parse(window.localStorage.getItem("show"));
if(showGlobal == null) {
	window.localStorage.setItem("show", JSON.stringify("All"));
	showGlobal = "All";
}
var thumbSizeGlobal = JSON.parse(window.localStorage.getItem("thumbSize"));
var selectedSyncConnectionGlobal = JSON.parse(window.localStorage.getItem("selectedSyncConnection"));
var selectedOutputConnectionGlobal = JSON.parse(window.localStorage.getItem("selectedOutputConnection"));
var disableLazyLoad = JSON.parse(window.localStorage.getItem("disableLazyLoad"));
toggleLazyLoad(disableLazyLoad, false);
/* 	if(!thumbSizeGlobal && window.devicePixelRatio == 2.75) {
		thumbSizeGlobal = 400;
	} */
//deviceAddress =  JSON.parse(window.localStorage.getItem("webSocketAddress"));
// if(!deviceAddress)
// 	deviceAddress = "tcode.local";

//document.getElementById("webSocketAddress").value = deviceAddress;

var externalStreaming = JSON.parse(window.localStorage.getItem("externalStreaming"));
toggleExternalStreaming(externalStreaming, false);

const skipToMoneyShotButton = document.getElementById('money-shot');
const skipToNextActionButton = document.getElementById('next-action');
const exitVideoButton = document.getElementById('exit-action');
skipToMoneyShotButton.addEventListener("click", sendSkipToMoneyShot);
skipToMoneyShotButton.addEventListener("click", onSkipToMoneyShotClick);
skipToNextActionButton.addEventListener("click", sendSkipToNextActionClick);
exitVideoButton.addEventListener("click", stopVideoClick);
videoNode.addEventListener("loadeddata", onVideoLoad);
videoNode.addEventListener("play", onVideoPlay);
videoNode.addEventListener("playing", onVideoPlaying);
videoNode.addEventListener("stalled", onVideoStall);
videoNode.addEventListener("waiting", onVideoStall);
videoNode.addEventListener("pause", onVideoPause);
videoNode.addEventListener("volumechange", onVolumeChange);
videoNode.addEventListener("ended", onVideoEnd);
videoNode.addEventListener("timeupdate", onVideoTimeUpdate);

const playNextButton = document.getElementById('play-next');
playNextButton.addEventListener('click', playNextVideoClick);
const playPreviousButton = document.getElementById('play-previous');
playPreviousButton.addEventListener('click', playPreviousVideoClick);

// Fires on load?
// videoSourceNode.addEventListener('error', function(event) { 
// 	userError("There was an issue loading media.");
// }, true);
// videoNode.addEventListener('error', function(event) { 
// 	userError("There was an issue loading media.");
// }, true);

const xtpWebVersionNode = document.getElementById("webVersion");
xtpWebVersionNode.innerText = webVersion;

var saveStateNode = document.getElementById("saveState");
var mediaReloadRequiredNode = document.getElementById("mediaReloadRequired");

var metaDataSaveStateNode = document.getElementById("metaDataSaveState");
var deviceConnectionStatusRetryButtonNodes = document.getElementsByName("deviceStatusRetryButton");
var deviceConnectionStatusRetryButtonImageNodes = document.getElementsByName("connectionStatusIconImage");

var progressNode = document.getElementById("statusOutput");
var progressLabelNode = document.getElementById("statusOutputLabel");

/* 	
	deoVideoNode = document.getElementById("deoVideoPlayer");
	deoSourceNode = document.getElementById("deoVideoSource");
	deoVideoNode.addEventListener("end", onVideoStop); 
	useDeoWeb = JSON.parse(window.localStorage.getItem("useDeoWeb"));

	if(useDeoWeb) {
		toggleUseDeo(useDeoWeb, false);
	} 
*/

setupTextToSpeech();
getServerSettings();

debugMode = true;
function debug(message) {
	if (debugMode)
		console.log(message);
}
function error(message) {
	console.error(message);
}

function sendWebsocketMessage(command, message) {
	if (websocket && xtpConnected) {
		var obj;
		if (!message) {
			obj = { "command": command }
		} else {
			obj = { "command": command, "message": message }
		}
		websocket.send(JSON.stringify(obj));
	}
}

function onInputDeviceConnectionChange(input, device) {
	selectedInputDevice = device;
	sendInputDeviceConnectionChange(device, input.checked);
	window.localStorage.setItem("selectedSyncConnection", parseInt(device, 10));
	sendMediaState();
}

function onOutputDeviceConnectionChange(input, device) {
	selectedOutputDevice = device;
	sendOutputDeviceConnectionChange(device, input.checked);
	window.localStorage.setItem("selectedOutputConnection", parseInt(device, 10));
	sendMediaState();
}

function startMetadataProcess() {
	showAlertWindow("Process metadata", "This will set metadata using the algrorith on first scan.<br>Adding smart tags and mfs tags based on the scan.<br>It will not change any user tags set by you.",sendMetadataProcess);
}
function sendMetadataProcess() {
	sendWebsocketMessage("startMetadataProcess");
	closeAlertWindow();
}

function tcodeDeviceConnectRetry() {
	sendWebsocketMessage("connectOutputDevice", { deviceName: remoteUserSettings.connection.output.selectedDevice });
}

function sendTCode(tcode) {
	sendWebsocketMessage("tcode", tcode);
}
function sendTCodeRange(channelName, min, max) {
	sendWebsocketMessage("setChannelRange", { channelName: channelName, min: min, max: max });
}

function sendInputDeviceConnectionChange(device, checked) {
	sendWebsocketMessage("connectInputDevice", { deviceName: device, enabled: checked });
}
function sendOutputDeviceConnectionChange(device, checked) {
	sendWebsocketMessage("connectOutputDevice", { deviceName: device, enabled: checked });
}
function sendSkipToNextActionClick() {
	if(!controlsVisible)
		return;
	sendSkipToNextAction();
}
function sendSkipToNextAction() {
	sendWebsocketMessage("skipToNextAction");
}
function sendSkipToMoneyShot() {
	sendWebsocketMessage("skipToMoneyShot");
}
function sendUpdateThumb(item) {
	sendWebsocketMessage("saveSingleThumb", { itemID: item.id, pos: 0 });
}
function sendUpdateThumbAtPos(item) {
	var pos = videoNode.currentTime;
	sendWebsocketMessage("saveSingleThumb", { itemID: item.id, pos: pos });
}
function sendUpdateMoneyShotAtPos(item) {
	var pos = videoNode.currentTime;
	sendWebsocketMessage("setMoneyShot", { itemID: item.id, pos: pos });
}

function setSaveState(node, saving, error) {
	if(!node)
		node = saveStateNode;
	node.innerText = saving ? "Saving settings..." : "Save success!";
	if(!error) {
		if(!saving) {
			node.style.color = "green";
			setTimeout(() => {
				node.style.visibility = "hidden";
				node.style.opacity = "0";
			}, 5000);
		} else {
			node.style.color = "white";
		}
		node.style.visibility = "visible";
		node.style.opacity = "1";
	} else {
		node.style.visibility = "visible";
		node.style.opacity = "1";
		node.style.color = "red";
		node.innerText = "Save fail";
		node.title = error;
	}
}


function restartXTP() {
	showAlertWindow("Confirm", `Are you sure you want restart XTP? 
		${remoteUserSettings.hasLaunchPass ? `
			<br><br>PLEASE NOTE: XTP is protected by a launch password.
			<br>You will need to go to the pc and enter the password manually for now.` : ''}`, 
	function () {
		sendWebsocketMessage("restartService");
		closeAlertWindow();
		closeSettings();
	});
}
function refreshXTPLibrary() {
	sendWebsocketMessage("reloadLibrary");
	mediaReloadRequiredNode.classList.add('hidden');
}

function initWebSocket() {
	try {
		wsUri = "ws://" + window.location.hostname + ":" + remoteUserSettings.webSocketServerPort;
		
		debug("Connecting to web socket uri: "+wsUri);
		if (typeof MozWebSocket == 'function')
			WebSocket = MozWebSocket;
		if (websocket && websocket.readyState == 1)
			websocket.close();
		websocket = new WebSocket(wsUri);
		websocket.onopen = function (evt) {
			xtpConnected = true;
			stopServerConnectionRetry();
			serverRetryTimeoutTries = 0;
			debug("CONNECTED");
			updateSettingsUI();
			sendMediaState();
		};
		websocket.onmessage = function (evt) {
			wsCallBackFunction(evt);
			debug("MESSAGE RECIEVED: " + evt.data);
		};
		websocket.onclose = function (evt) {
			debug("DISCONNECTED");
			xtpConnected = false;
			startServerConnectionRetry();
		};
		websocket.onerror = function (evt) {
			console.log('ERROR: ' + evt.data + ", Address: " + wsUri);
			xtpConnected = false;
			startServerConnectionRetry();
		};
	} catch (exception) {
		console.log('ERROR: ' + exception + ", Address: " + wsUri);
		xtpConnected = false;
		startServerConnectionRetry();
	}
}

function wsCallBackFunction(evt) {
	try {
		var data = JSON.parse(evt.data);
		switch (data["command"]) {
			case "outputDeviceStatus":
				var status = data["message"];
				var deviceName = status["deviceName"];
				setOutputConnectionStatus(deviceName, status["status"], status["message"]);
				break;
			case "inputDeviceStatus":
				var status = data["message"];
				var deviceName = status["deviceName"];
				setInputConnectionStatus(deviceName, status["status"], status["message"]);
				break;
			case "mediaLoaded":
				var mediaLoadingElement = document.getElementById("mediaLoading");
				mediaLoadingElement.style.display = "none"
				mediaLoading = false;
				getServerLibrary();
				break;
			case "mediaLoading":
				setMediaLoading();
				break;
			case "mediaLoadingStatus":
				setMediaLoadingStatus(data["message"]);
				break;
			case "updateThumb":
				var message = data["message"];
				var mediaElement = document.getElementById(message.id);
				if (mediaElement) {
					var imageElement = mediaElement.getElementsByClassName("media-thumb-nail")[0];
					var src = "/thumb/" + message.thumb + "?" + new Date().getTime();
					if(imageElement.src.length > 0)
						imageElement.src = src;
					else
						imageElement.dataset.src = src;
					if (message.errorMessage)
						imageElement.title = message.errorMessage;
					var index = mediaListGlobal.findIndex(x => x.id === message.id);
					mediaListGlobal[index].relativeThumb = "/thumb/" + message.thumb;
					mediaListGlobal[index].thumbFileExists = true;
				}
				break;
			case "updateItem":
				var message = data["message"];
				var libraryItem = message["item"];
				var roles = message["roles"];
				updateItem(libraryItem, roles);
				break;
			case "textToSpeech":
				var message = data["message"];
				onTextToSpeech(message);
				break;
			case "skipToNextAction":
				var message = data["message"];
				skipVideoTo(message);
				break;
			case "skipToMoneyShot":
				onSkipToMoneyShot();
				break;
			case "logout":
				logout();
				break;
			case "channelProfileChanged":
				getServerChannels();
				break;
			case "statusOutput":
				var messageObj = data["message"];
				var message = messageObj["message"]
				var percentage = messageObj["percentage"]
				setStatusOutput(message, percentage);
				break;
			case "tagsUpdate":
				var messageObj = data["message"];
				var smartTags = messageObj["smartTags"]
				var userTags = messageObj["userTags"]
				remoteUserSettings["allTags"] = [];
				remoteUserSettings["allTags"].push(...userTags, ...smartTags);
				remoteUserSettings["userTags"] = []
				remoteUserSettings["userTags"].push(...userTags);
				remoteUserSettings["smartTags"] = []
				remoteUserSettings["smartTags"].push(...smartTags);
				setupSystemTags();
				break;
		}
	}
	catch (e) {
		if(typeof e == "string")
			error(e);
		else
			error(e.toString());
	}
}

function setStatusOutput(message, percentage) {
	if(!message && !progressNode.classList.contains("hidden-visibility")) {
		progressNode.classList.add("hidden-visibility")
		progressLabelNode.classList.add("hidden-visibility");
	} else if(message && progressNode.classList.contains("hidden-visibility")) {
		progressNode.classList.remove("hidden-visibility");
		progressLabelNode.classList.remove("hidden-visibility");
	}
	// !message ? progressNode.classList.add("hidden-visibility") : progressNode.classList.remove("hidden-visibility");
	// !message ? progressLabelNode.classList.add("hidden-visibility") : progressLabelNode.classList.remove("hidden-visibility");
	progressLabelNode.innerText = (!message ? "" : message) + (percentage > -1 ? ": "+percentage+"%" : "");
	progressNode.value = (percentage > -1 ? percentage : 0);
}
function setMediaLoading() {
	mediaLoading = true;
	clearMediaList();
	var mediaLoadingElement = document.getElementById("mediaLoading");
	mediaLoadingElement.style.display = "flex"
	var noMediaElement = document.getElementById("noMedia");
	noMediaElement.hidden = true;
}

function setMediaLoadingStatus(status) {
	var mediaLoadingElement = document.getElementById("loadingStatus");
	mediaLoadingElement.innerText = status;
}

function onResizeVideo() {
	// if(!videoContainer.classList.contains('video-fixed-container'))
	// 	thumbsContainerNode.style.maxHeight = "calc(100vh - " + (+videoNode.offsetHeight + 100) + "px)";
	// else {
	// 	thumbsContainerNode.style.maxHeight = "calc(100vh - 100px)";
	// }
}


function getBrowserInformation() {
	var nAgt = navigator.userAgent;
	var browserName;
	var fullVersion;
	var majorVersion;
	var nameOffset, verOffset, ix;

	// In Opera, the true version is after "Opera" or after "Version"
	if ((verOffset = nAgt.indexOf("Opera")) != -1) {
		browserName = "Opera";
		fullVersion = nAgt.substring(verOffset + 6);
		if ((verOffset = nAgt.indexOf("Version")) != -1)
			fullVersion = nAgt.substring(verOffset + 8);
	}
	// In MSIE, the true version is after "MSIE" in userAgent
	else if ((verOffset = nAgt.indexOf("MSIE")) != -1) {
		browserName = "Microsoft Internet Explorer";
		fullVersion = nAgt.substring(verOffset + 5);
	}
	// In Chrome, the true version is after "Chrome" 
	else if ((verOffset = nAgt.indexOf("Chrome")) != -1) {
		browserName = "Chrome";
		fullVersion = nAgt.substring(verOffset + 7);
	}
	// In Safari, the true version is after "Safari" or after "Version" 
	else if ((verOffset = nAgt.indexOf("Safari")) != -1) {
		browserName = "Safari";
		fullVersion = nAgt.substring(verOffset + 7);
		if ((verOffset = nAgt.indexOf("Version")) != -1)
			fullVersion = nAgt.substring(verOffset + 8);
	}
	// In Firefox, the true version is after "Firefox" 
	else if ((verOffset = nAgt.indexOf("Firefox")) != -1) {
		browserName = "Firefox";
		fullVersion = nAgt.substring(verOffset + 8);
	}
	// In most other browsers, "name/version" is at the end of userAgent 
	else if ((nameOffset = nAgt.lastIndexOf(' ') + 1) <
		(verOffset = nAgt.lastIndexOf('/'))) {
		browserName = nAgt.substring(nameOffset, verOffset);
		fullVersion = nAgt.substring(verOffset + 1);
		if (browserName.toLowerCase() == browserName.toUpperCase()) {
			browserName = navigator.appName;
		}
	}
	// trim the fullVersion string at semicolon/space if present
	if ((ix = fullVersion.indexOf(";")) != -1)
		fullVersion = fullVersion.substring(0, ix);
	if ((ix = fullVersion.indexOf(" ")) != -1)
		fullVersion = fullVersion.substring(0, ix);

	majorVersion = parseInt('' + fullVersion, 10);
	if (isNaN(majorVersion)) {
		fullVersion = '' + parseFloat(navigator.appVersion);
		majorVersion = parseInt(navigator.appVersion, 10);
	}

	var divnode = document.createElement("div");
	divnode.innerHTML = ''
		+ 'Browser name  = ' + browserName + '<br>'
		+ 'Full version  = ' + fullVersion + '<br>'
		+ 'Major version = ' + majorVersion + '<br>'
		+ 'navigator.userAgent = ' + navigator.userAgent + '<br>';
	userAgent = navigator.userAgent;
	document.getElementById("browserInfo").appendChild(divnode)
}
function setDeoStyles(isDeo) {
	if (isDeo) {
		var checkboxes = document.querySelectorAll("input[type='checkbox']");
		for (var i = 0; i < checkboxes.length; i++) {
			checkboxes[i].classList.remove("styled-checkbox")
		}
	} else {
		var checkboxes = document.getElementsByClassName("input[type='checkbox']");
		for (var i = 0; i < checkboxes.length; i++) {
			checkboxes[i].classList.add("styled-checkbox");
		}
	}
}
/* 
function onResizeDeo() {
	if(useDeoWeb) {
		thumbsContainerNode.style.maxHeight = "calc(100vh - "+ (+deoVideoNode.offsetHeight + 120) + "px)";
	}
} 
*/
function logout() {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/logout", true);
	xhr.responseType = 'json';
	xhr.onload = function () {
		var status = xhr.status;
		if (status === 200) {
			window.localStorage.removeItem("storedHash");
			window.sessionStorage.clear();
			delete_cookie("sessionID");
			cookies.sessionID = undefined;
			window.location.replace(window.location.origin);
		} else {
			systemError('Error logging out: ' + err);
		}
	};
	xhr.onerror = function () {
		systemError("Error logging out");
	};
	xhr.send();
}

function checkPass() {
    var xhr = new XMLHttpRequest();
    xhr.open('POST', "/auth", true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.onreadystatechange = function () {
        if (xhr.readyState === 4) {
            var status = xhr.status;
            if (status == 200) {
				startServerConnectionRetry();
            }
        }
    }
    xhr.onerror = function () {
        onSaveFail(xhr.statusText);
    };
    if(storedHash) {
        xhr.send("{\"hashedPass\":\""+storedHash+"\", \"remember\":\""+!!storedHash+"\"}");
    } else
        alert("Invalid password");
}

function getServerSettings(retry) {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/settings", true);
	xhr.responseType = 'json';
	xhr.onload = function (evnt, retry) {
		var status = xhr.status;
		if (status === 200) {
			remoteUserSettings = xhr.response;

			document.getElementById("xteVersion").innerText = remoteUserSettings["xteVersion"];
			
			setupChannelData();

			setupSystemTags();
			
			initWebSocket();
		} else if(status == 401) {
			if(storedHash) {
				checkPass();
			} else {
				window.location.replace(window.location.origin);
			}
		} else {
	/* 		if(!retry)
				systemError("Error getting settings"); */
			startServerConnectionRetry();
		}
	}.bind(this);
	xhr.onerror = function(evnt, retry) {
	/* 	if(!retry)
			systemError("Error getting settings: "+ xhr.responseText);
		else */
		startServerConnectionRetry();
	};
	xhr.send();
}

function getServerChannels() {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/channels", true);
	xhr.responseType = 'json';
	xhr.onload = function (evnt) {
		var status = xhr.status;
		if (status === 200) {
			var response = xhr.response;
			remoteUserSettings.availableChannels = response.availableChannels;
			remoteUserSettings.selectedChannelProfile = response.selectedChannelProfile;
			remoteUserSettings.allChannelProfileNames = response.allChannelProfileNames;
			setupChannelData();
			updateChannelsUI();
		} else {
			systemError("Error getting channels: "+ xhr.responseText);
		}
	}.bind(this);
	xhr.onerror = function(evnt) {
		systemError("Error getting channels: "+ xhr.responseText);
	};
	xhr.send();
}

function setupChannelData() {
	// Used to get funscripts from server?
	funscriptChannels = Object.keys(remoteUserSettings["availableChannels"])
		.map(function (k) {
			return remoteUserSettings["availableChannels"][k]["channel"];
		});
	remoteUserSettings.availableChannelsArray = Object.keys(remoteUserSettings["availableChannels"])
		.map(function (k) {
			return remoteUserSettings["availableChannels"][k];
		});
	funscriptChannels.sort();
}

function setupSystemTags() {
	const tags = remoteUserSettings["allTags"];
	const tagsFIlterNode = document.getElementById("tagFilterOptions");
	const metaDataTagsNode = document.getElementById("metaDataTags");
	removeAllChildNodes(tagsFIlterNode);
	removeAllChildNodes(metaDataTagsNode);
	const containter = document.createElement("div");
	containter.classList.add("media-tag-filter-container")
	tagsFIlterNode.appendChild(containter);
	tags.forEach((x, i) => {
		containter.appendChild(createCheckBoxDiv(x+tagCheckboxesName+i, tagCheckboxesName, x, x, onFilterByTagClicked));

		metaDataTagsNode.appendChild(createCheckBoxDiv(x+metadataTagCheckboxesName+i, metadataTagCheckboxesName, x, x, onMetadataTagCheckboxClicked));
	});

	const userTagsSelect = document.getElementById("systemTagsSelect");
	while(userTagsSelect.options.length > 0) {
		userTagsSelect.options.remove(userTagsSelect.options.length - 1);
	}
	const userTags = remoteUserSettings["userTags"];
	userTags.forEach((x, i) => {
		const option = document.createElement("option");
		option.value = x;
		option.innerText = x;
		userTagsSelect.options.add(option);
	});
	
	const smartTagsSelect = document.getElementById("smartTagsSelect");
	while(smartTagsSelect.options.length > 0) {
		smartTagsSelect.options.remove(smartTagsSelect.options.length - 1);
	}
	const smartTags = remoteUserSettings["smartTags"];
	smartTags.forEach((x, i) => {
		const option = document.createElement("option");
		option.value = x;
		option.innerText = x;
		smartTagsSelect.options.add(option);
	});
}

function systemTagsSelectChange(selectElement) {
	document.getElementById("removeTagButton").disabled = selectElement.options.selectedIndex == -1;
}
function smartagsSelectChange(selectElement) {
	document.getElementById("removeSmartTagButton").disabled = selectElement.options.selectedIndex == -1;
}

function addSystemTag(smartMode) {
	let tags = remoteUserSettings[smartMode ? "smartTags" : "userTags"];
	const otherTagName = !smartMode ? "smartTags" : "userTags";
	let other = remoteUserSettings[otherTagName];
	var name = prompt(smartMode ? 'Smart tag name' : 'Tag name');
	if(!name || !name.trim().length) {
		return;
	}
	if(tags.findIndex(x => x == name) > -1) {
		showAlertWindow("Tag exists", `Tag '${name}' already exists!`);
		return;
	}
	if(other.findIndex(x => x == name) > -1) {
		showAlertWindow("Tag exists", `Tag '${name}' already exists in ${otherTagName}!`);
		return;
	}
	const settingMemberName = smartMode ? "smartTagsToAdd" : "tagsToAdd";
	if(!remoteUserSettings[settingMemberName])
		remoteUserSettings[settingMemberName] = [];
	remoteUserSettings[settingMemberName].push(name);
	const systemTagsSelect = document.getElementById(smartMode ? "smartTagsSelect" : "systemTagsSelect");
	const option = document.createElement("option");
	option.value = name;
	option.innerText = name;
	systemTagsSelect.options.add(option);
	if(smartMode)
		mediaReloadRequiredNode.classList.remove('hidden');

	//tags.push(name);

	markXTPFormDirty();
}

function removeSystemTags(smartMode) {
	let tags = remoteUserSettings[smartMode ? "smartTags" : "userTags"];
	const systemTagsSelect = document.getElementById(smartMode ? "smartTagsSelect" : "systemTagsSelect");
	const settingMemberName = smartMode ? "smartTagsToRemove" : "tagsToRemove";
	if(!remoteUserSettings[settingMemberName])
		remoteUserSettings[settingMemberName] = [];
	while(systemTagsSelect.options.selectedIndex > -1) {
		const opt = systemTagsSelect.options[systemTagsSelect.options.selectedIndex];
		remoteUserSettings[settingMemberName].push(opt.value);
		//tags.splice(tags.findIndex(x => x == opt.value), 1);
		systemTagsSelect.options.remove(systemTagsSelect.options.selectedIndex);
	}
	markXTPFormDirty();
}

function setSelectedProfile(profileName) {
	sendWebsocketMessage("changeChannelProfile", profileName);
}

function getServerSessions() {
	if(cookies.sessionID) {
		var xhr = new XMLHttpRequest();
		xhr.open('GET', "/activeSessions", true);
		xhr.responseType = 'json';
		xhr.onload = function (evnt) {
			var status = xhr.status;
			if (status === 200) {
				var activeSessions = xhr.response;
				var tBody = document.getElementById("sessionInfoBody");
				removeAllChildNodes(tBody);
				activeSessions.forEach(element => {
					var row = document.createElement("tr");
					row.id = element["id"];
					var cellSessionID = document.createElement("td");
					cellSessionID.innerText = element["id"];
					var cellCreate = document.createElement("td");
					cellCreate.innerText = element["lastAccessed"];
					var cellExpire = document.createElement("td");
					cellExpire.innerText = element["expire"];
					var cellCurrent = document.createElement("td");
					cellCurrent.innerText = element["current"] ? "*" : "";
					var cellDelete = document.createElement("td");
					var btnDelete = document.createElement("button");
					btnDelete.onclick = checkSession(element["id"]);
					btnDelete.innerText = "X"
					cellDelete.appendChild(btnDelete);
					row.appendChild(cellSessionID);
					row.appendChild(cellCreate);
					row.appendChild(cellExpire);
					row.appendChild(cellCurrent);
					row.appendChild(cellDelete);
					tBody.appendChild(row);
				});
			} else {
				showAlertWindow(status.statusText);
			}
		}.bind(this);
		xhr.onerror = function(evnt, retry) {
			onSaveFail(xhr.status.statusText);
		};
		xhr.send();
	}
}

var checkSession = function (sessionID) {
	return function () {
		if(sessionID == cookies.sessionID)
			showAlertWindow("Logout?", "Are you sure you want to delete the current session? This will log you out.", function() {deleteSession(sessionID)});
		else
			deleteSession(sessionID);
	}
}

function deleteSession (sessionID) {
	var xhr = new XMLHttpRequest();
	xhr.open('POST', "/expireSession?sessionID="+sessionID, true);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			var status = xhr.status;
			if (status == 200) {
				var sessionNode = document.getElementById(sessionID);
				document.getElementById("sessionInfoBody").removeChild(sessionNode);
				if(sessionID == cookies.sessionID) {
					delete_cookie("sessionID");
					logout();
				}
			}
		}
	}
	xhr.onerror = function () {
		onSaveFail(xhr.statusText);
	};
	xhr.send();
}

function startServerConnectionRetry() {
	stopServerConnectionRetry();
	setMediaLoading();
	setMediaLoadingStatus("Waiting for reconnect...");
	serverRetryTimeoutTries++;
	if (serverRetryTimeoutTries < 100) {
		serverRetryTimeout = setTimeout(() => {
			getServerSettings();
		}, 5000);
	} else {
		setMediaLoadingStatus("Timed out while looking for server. Please refresh the page when the server has started again.")
	}
}
function stopServerConnectionRetry() {
	if(serverRetryTimeout != undefined) {
		clearTimeout(serverRetryTimeout);
		serverRetryTimeout = undefined;
	}
}

function updateSettingsUI() {
	updateChannelsUI();
	setupConnectionsTab();

	document.getElementById("tabLocalTab").onclick();
}

function updateChannelsUI() {
	// //Dropdown
	// var profileNamesSelect = document.getElementById("profileNamesSelect");
	// removeAllChildNodes(profileNamesSelect);
	// remoteUserSettings["allChannelProfileNames"].forEach(x => {
	// 	var option = document.createElement("option");
	// 	option.value = x;
	// 	option.innerText = x;
	// 	profileNamesSelect.appendChild(option);
	// });
	// profileNamesSelect.value = remoteUserSettings.selectedChannelProfile;
	// Radios
	var profileNamesDiv = document.getElementById("profileNamesDiv");
	removeAllChildNodes(profileNamesDiv);
	remoteUserSettings["allChannelProfileNames"].forEach(x => {
		var radio = document.createElement("input");
		var label = document.createElement("label");
		radio.type = "radio";
		radio.id = x;
		radio.name = "motionProfileGroup";
		radio.value = x;
		radio.onclick = function() {
			setSelectedProfile(this.value);
		}
		if(x == remoteUserSettings.selectedChannelProfile) {
			radio.checked = true;
		}
		label.for = x;
		label.innerText = x;
		profileNamesDiv.appendChild(radio);
		profileNamesDiv.appendChild(label);
	});

	setupSliders();
	setupMotionModifiers();
}

function getServerLibrary() {
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/media", true);
	xhr.responseType = 'json';
	xhr.onload = function () {
		var status = xhr.status;
		if (status === 200) {
			mediaListGlobal = xhr.response;
			setThumbSize(thumbSizeGlobal, false);
			sort(sortByGlobal, false);
			mediaListDisplayed = getDisplayedMediaList(showGlobal, false);
			loadMedia(mediaListDisplayed);
			filter();
		} else {
			systemError('Error getting media list: ' + err);
		}
	};
	xhr.onerror = function () {
		systemError("Error getting media");
	};
	xhr.send();
}

function setInputConnectionStatus(deviceName, status, message) {
	// check &#x2714;
	// x &#x2718;
	// triangle ! &#x26A0;
	var statusImage = ""
	var statusColor = "";
	switch (status) {
		case ConnectionStatus.Connected:
			statusImage = "://images/icons/check-mark-black.png";
			statusColor = "chartreuse";
			break;
		case ConnectionStatus.Connecting:
			statusImage = "://images/icons/reload.svg";
			statusColor = "yellow";
			break;
		case ConnectionStatus.Disconnected:
			statusImage = "://images/icons/x.svg";
			statusColor = "crimson";
			break;
		case ConnectionStatus.Error:
			statusImage = "://images/icons/error-black.png";
			statusColor = "red";
			break;
	}
	var deoVRStatusImage = document.getElementById("deoVRStatus");
	var whirligigStatusImage = document.getElementById("whirligigStatus");
	var xtpWebStatusImage = document.getElementById("xtpWebStatus");
	var gamepadStatusImage = document.getElementById("gamepadStatus");
	if (deviceName != DeviceType.Gamepad) {
		deoVRStatusImage.src = "://images/icons/x.svg";
		deoVRStatusImage.style.backgroundColor = "crimson";
		deoVRStatusImage.title = "Disconnected";
		//deoVRStatusImage.alt = "Disconnected"
		whirligigStatusImage.src = "://images/icons/x.svg";
		whirligigStatusImage.style.backgroundColor = "crimson";
		whirligigStatusImage.title = "Disconnected";
		//whirligigStatusImage.alt = "Disconnected"
		xtpWebStatusImage.src = "://images/icons/x.svg";
		xtpWebStatusImage.style.backgroundColor = "crimson";
		xtpWebStatusImage.title = "Disconnected";
		//xtpWebStatusImage.alt = "Disconnected"
	}
	switch (deviceName) {
		case DeviceType.Deo:
			deoVRStatusImage.src = statusImage;
			deoVRStatusImage.style.backgroundColor = statusColor;
			deoVRStatusImage.title = message;
			//deoVRStatusImage.alt = message 
			break;
		case DeviceType.Whirligig:
			whirligigStatusImage.src = statusImage;
			whirligigStatusImage.style.backgroundColor = statusColor;
			whirligigStatusImage.title = message;
			//deoVRStatusImage.alt = message 
			break;
		case DeviceType.XTPWeb:
			xtpWebStatusImage.src = statusImage;
			xtpWebStatusImage.style.backgroundColor = statusColor;
			xtpWebStatusImage.title = message;
			//deoVRStatusImage.alt = message 
			break;
		case DeviceType.Gamepad:
			gamepadStatusImage.src = statusImage;
			gamepadStatusImage.style.backgroundColor = statusColor;
			gamepadStatusImage.title = message;
			//deoVRStatusImage.alt = message 
			break;

	}
}
function setOutputConnectionStatus(deviceName, status, message) {
	for (var i = 0; i < deviceConnectionStatusRetryButtonNodes.length; i++) {
		var deviceConnectionStatusRetryButtonNode = deviceConnectionStatusRetryButtonNodes[i];
		var deviceConnectionStatusRetryButtonImageNode = deviceConnectionStatusRetryButtonImageNodes[i];
		deviceConnectionStatusRetryButtonNode.title = "TCode status: " + message;
		deviceConnectionStatusRetryButtonNode.style.cursor = "";
		deviceConnectionStatusRetryButtonNode.disabled = true;
		var tcodeDeviceSettingsLink = document.getElementById("tcodeDeviceSettingsLink");
		tcodeDeviceSettingsLink.hidden = true;
		outputConnectionStatus = status;
		switch (status) {
			case ConnectionStatus.Disconnected:
				deviceConnectionStatusRetryButtonNode.style.backgroundColor = "crimson";
				deviceConnectionStatusRetryButtonNode.style.cursor = "pointer";
				deviceConnectionStatusRetryButtonNode.disabled = false;
				deviceConnectionStatusRetryButtonImageNode.src = "://images/icons/x.svg";
				deviceConnectionStatusRetryButtonNode.title += ": Check your devices connection and click this to retry"
				break;
			case ConnectionStatus.Connecting:
				deviceConnectionStatusRetryButtonNode.style.backgroundColor = "yellow";
				deviceConnectionStatusRetryButtonImageNode.src = "://images/icons/reload.svg";
				break;
			case ConnectionStatus.Connected:
				outputDeviceConnected = true;
				deviceConnectionStatusRetryButtonNode.style.backgroundColor = "chartreuse";
				deviceConnectionStatusRetryButtonImageNode.src = "://images/icons/check-mark-black.png";
				if (remoteUserSettings.connection.output.selectedDevice == DeviceType.Network) {
					tcodeDeviceSettingsLink.href = "http://" + remoteUserSettings.connection.networkAddress;
					tcodeDeviceSettingsLink.hidden = false;
				}
				break;
			case ConnectionStatus.Error:
				deviceConnectionStatusRetryButtonNode.style.backgroundColor = "red";
				deviceConnectionStatusRetryButtonNode.style.cursor = "pointer";
				deviceConnectionStatusRetryButtonNode.disabled = false;
				deviceConnectionStatusRetryButtonImageNode.src = "://images/icons/error-black.png";
				deviceConnectionStatusRetryButtonNode.title += ": Check your devices connection and click this to retry"
				break;

		}
	}
	var statusImage = ""
	var statusColor = "";
	switch (status) {
		case ConnectionStatus.Connected:
			statusImage = "://images/icons/check-mark-black.png";
			statusColor = "chartreuse";
			break;
		case ConnectionStatus.Connecting:
			statusImage = "://images/icons/reload.svg";
			statusColor = "yellow";
			break;
		case ConnectionStatus.Disconnected:
			statusImage = "://images/icons/x.svg";
			statusColor = "crimson";
			break;
		case ConnectionStatus.Error:
			statusImage = "://images/icons/error-black.png";
			statusColor = "red";
			break;
	}
	var networkStatusImage = document.getElementById("networkStatus");
	var serialStatusImage = document.getElementById("serialStatus");
	networkStatusImage.src = "://images/icons/x.svg";
	networkStatusImage.style.backgroundColor = "crimson";
	networkStatusImage.title = "Disconnected";
	//networkStatusImage.alt = "Disconnected"
	serialStatusImage.src = "://images/icons/x.svg";
	serialStatusImage.style.backgroundColor = "crimson";
	serialStatusImage.title = "Disconnected";
	switch (deviceName) {
		case DeviceType.Network:
			networkStatusImage.src = statusImage;
			networkStatusImage.style.backgroundColor = statusColor;
			networkStatusImage.title = message;
			//networkStatusImage.alt = message 
			break;
		case DeviceType.Serial:
			serialStatusImage.src = statusImage;
			serialStatusImage.style.backgroundColor = statusColor;
			serialStatusImage.title = message;
			//serialStatusImage.alt = message 
			break;

	}
	//whirligigStatusImage.alt = "Disconnected"
}
/* function getDeviceConnectionStatus() {
	if(deviceConnectionStatusInterval)
		clearTimeout(deviceConnectionStatusInterval);
	var xhr = new XMLHttpRequest();
	xhr.open('GET', "/settings/deviceConnectionStatus", true);
	xhr.responseType = 'json';
	xhr.onload = function() {
	  var status = xhr.status;
	  if (status === 200) {
		var connectionStatusEvent = xhr.response;
		connectionStatus = connectionStatusEvent.status;
		setConnectionStatus(connectionStatus, connectionStatusEvent.message);
		pollDeviceConnectionStatus(30000);
		if(connectionStatus == ConnectionStatus.Connecting) {
			pollDeviceConnectionStatus(1000);
		}
	  } else {
		systemError("Http Error getting device connection status: "+ status);
		if(deviceConnectionStatusInterval)
			clearTimeout(deviceConnectionStatusInterval);
			setConnectionStatus(ConnectionStatus.Error, "Http Error getting device connection status: "+ status);
	  }
	};
	xhr.onerror = function() {
		var status = xhr.status;
		systemError("Error getting device connection status: "+ status);
		if(deviceConnectionStatusInterval)
			clearTimeout(deviceConnectionStatusInterval);
			setConnectionStatus(ConnectionStatus.Error, "Error getting device connection status: "+ status);
	};
	xhr.send();
} */

/* function pollDeviceConnectionStatus(pollInterval) {
	if(deviceConnectionStatusInterval) {
		clearTimeout(deviceConnectionStatusInterval);
		deviceConnectionStatusInterval = null;
	}
	deviceConnectionStatusInterval = setTimeout(function () {
		getDeviceConnectionStatus();
	}, pollInterval);
} */


function getMediaFunscripts(path, isMFS) {
	var channel = funscriptChannels[currentChannelIndex];
	var trackName = remoteUserSettings["availableChannels"][channel]["trackName"];
	var xhr = new XMLHttpRequest();
	var channelPath = trackName === "" ? trackName : "." + trackName;
	xhr.open('GET', path + channelPath + ".funscript", true);
	xhr.responseType = 'json';
	xhr.onload = function () {
		var status = xhr.status;
		if (status === 200) {
			loadedFunscripts.push({ "channel": channel, "atList": [] });
			var index = loadedFunscripts.length - 1;
			var items = xhr.response;
			for (var i = 0; i < items.actions.length; i++) {
				var item = items.actions[i];
				var at = item["at"];
				loadedFunscripts[index][at] = item["pos"];
				loadedFunscripts[index].atList.push(at);
			}
		}
		currentChannelIndex++;
		if (currentChannelIndex < funscriptChannels.length) {
			getMediaFunscripts(path, isMFS);
		}
		else {
			currentChannelIndex = 0;
			videoNode.play();
			console.log("Funscripts load finish");
		}
	};
	xhr.send();
}

function onSaveToXTPClick() {
	postServerSettings();
}
function postServerSettings() {
	setSaveState(null, true);
	var xhr = new XMLHttpRequest();
	xhr.open('POST', "/settings", true);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			var status = xhr.status;
			if (status !== 200)
				onSaveFail(xhr.statusText);
			else {
				onSaveSuccess();
				markXTPFormClean();
			}
		}
	}
	xhr.onerror = function () {
		onSaveFail(xhr.statusText);
	};
	xhr.send(JSON.stringify(remoteUserSettings));
}

function postMediaItemMetaData(metaData) {
	var xhr = new XMLHttpRequest();
	xhr.open('POST', "/mediaItemMetadata", true);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.onreadystatechange = function () {
		if (xhr.readyState === 4) {
			var status = xhr.status;
			if (status !== 200)
				onSaveFail(xhr.statusText, metaDataSaveStateNode);
			else {
				onSaveSuccess(metaDataSaveStateNode);
				document.getElementById("saveMediaItemMetaDataButton").disabled = true;
			}
		}
	}
	xhr.onerror = function () {
		onSaveFail(xhr.statusText, metaDataSaveStateNode);
	};
	xhr.send(JSON.stringify(metaData));
}

function postMediaState(mediaState) {
	var xhr = new XMLHttpRequest();
	xhr.open("POST", "/xtpweb", true);
	xhr.setRequestHeader('Content-Type', 'application/json');
	xhr.send(JSON.stringify(mediaState));
	xhr.oneload = function () {
		var status = xhr.status;
		if (status !== 200)
			console.log('Error sending mediastate: ' + xhr.statusText)
	};
	xhr.onerror = function () {
		console.log('Error sending mediastate: ' + xhr.statusText)
	};
}

function onSaveSuccess(node) {
	setSaveState(node, false);
}
function onSaveFail(error, node) {
	setSaveState(node, false, error);
}

function clearMediaList() {
	var medialistNode = document.getElementById("mediaList");
	removeAllChildNodes(medialistNode);
	return medialistNode;
}
function loadMedia(mediaList) {
	var medialistNode = clearMediaList();

	var noMediaElement = document.getElementById("noMedia");
	if (!mediaList || mediaList.length == 0) {
		if(!mediaLoading) {
			noMediaElement.innerHTML = "No media found<br>Current filter: " + showGlobal;
			noMediaElement.hidden = false;
		}
		return;
	}
	noMediaElement.hidden = true;

	var createClickHandler = function (obj) {
		return function () {
			//loadVideo(obj); 
			playVideo(obj);
		}
	};

	var toggleContext = function (contextMenu, mediaItem) {
		return function () {
			contextMenu.classList.toggle("hidden");
			var thumbAtPosMenuItem = contextMenu.getElementsByClassName("setThumbAtCurrent")[0];
			var setMoneyShotAtCurrentMenuItem = contextMenu.getElementsByClassName("setMoneyShotAtCurrent")[0];
			if(isVideoShown() && playingmediaItem && playingmediaItem.id === mediaItem.id) {
				thumbAtPosMenuItem.classList.remove("disabled");
				setMoneyShotAtCurrentMenuItem.classList.remove("disabled");
				setMoneyShotAtCurrentMenuItem.title = "Set moneyshot at current"
				thumbAtPosMenuItem.title = "Set thumb at current"
			} else {
				thumbAtPosMenuItem.classList.add("disabled");
				thumbAtPosMenuItem.title = "This is not the current playing video so it cannot be set at the current position."
				setMoneyShotAtCurrentMenuItem.classList.add("disabled");
				setMoneyShotAtCurrentMenuItem.title = "This is not the current playing video so it cannot be set at the current position."
			}
			var regenerateThumbMenuItem = contextMenu.getElementsByClassName("regenerateThumb")[0];
			if(mediaItem.managedThumb && !mediaItem.thumb.includes(".lock."))
				regenerateThumbMenuItem.classList.remove("disabled");
			else {
				regenerateThumbMenuItem.classList.add("disabled");
				regenerateThumbMenuItem.title = "This thumb file is not modifiable because it is not managed by XTP."
			}
			
		}
	};

	var closeContext = function (contextMenu) {
		return function () {
			contextMenu.classList.add("hidden");
		}
	};

	var mediaSettingsClick = function (mediaItem, contextMenu) {
		return function () {
			openMetaDataModal(mediaItem);
			contextMenu.classList.add("hidden");
		}
	};
	
	var regenThumbClick = function (mediaItem, contextMenu) {
		return function () {
			sendUpdateThumb(mediaItem);
			contextMenu.classList.add("hidden");
		}
	};
	var regenThumbCurrentPosClick = function (mediaItem, contextMenu) {
		return function () {
			sendUpdateThumbAtPos(mediaItem);
			contextMenu.classList.add("hidden");
		}
	};
	var setMoneyShotCurrentPosClick = function (mediaItem, contextMenu) {
		return function () {
			mediaItem["metaData"]["moneyShotMillis"] = Math.round(videoNode.currentTime * 1000).toString();
			postMediaItemMetaData(mediaItem["metaData"]);
			contextMenu.classList.add("hidden");
		}
	};

	var textHeight = 0
	var width = 0;
	var widthInt = 0;
	var height = 0;
	var fontSize = 0;
	for (var i = 0; i < mediaList.length; i++) {
		var obj = mediaList[i];
		if (!thumbSizeGlobal) {
			setThumbSize(obj.thumbSize, true);
			setThumbSize(obj.thumbSize, false);
		}
		if (!textHeight) {
			textHeight = (thumbSizeGlobal * 0.25);
			widthInt = thumbSizeGlobal + (thumbSizeGlobal * 0.15);
			width = widthInt+ "px";
			height = thumbSizeGlobal + textHeight + "px";
			fontSize = (textHeight * 0.35) + "px";
		}
		var divnode = document.createElement("div");
		divnode.id = obj.id
		divnode.className += "media-item"
		divnode.style.width = width;
		divnode.style.height = height;
		divnode.title = obj.name;

		var info = document.createElement("button");
		info.id = obj.id + "InfoButton";
		info.classList.add("media-context");
		// if(userAgentIsHereSphere)
		// 	info.classList.add("media-context-heresphere");
		info.style.width = widthInt * 0.20 + "px";
		info.style.height = widthInt * 0.20 + "px";
		var contextMenu = document.createElement("div"); 
		contextMenu.id = obj.id + "ContextMenu";
		info.onclick = toggleContext(contextMenu, obj);
		info.dataset.title = "Media actions";
		// var icon = document.createElementNS("http://www.w3.org/2000/svg", "svg");
		// icon.style.width = widthInt * 0.08 + "px";
		// icon.style.height = widthInt * 0.08 + "px";
		// //icon.classList.add("player-button-png");
		// var iconUse = document.createElementNS("http://www.w3.org/2000/svg", "use");
		// iconUse.setAttribute("href", "://images/icons/context-menu.svg");
		var icon = document.createElement("img");
		icon.src = "://images/icons/context-menu.svg";
		icon.style.width = widthInt * 0.10 + "px";;
		icon.style.height = widthInt * 0.10 + "px";
		//icon.appendChild(iconUse);
		info.appendChild(icon);
		contextMenu.classList.add("media-context-menu", "hidden");
		// if(userAgentIsHereSphere)
		// 	contextMenu.classList.add("media-context-menu-heresphere");
		
		contextMenu.style.fontSize = fontSize;
		//contextMenu.style.width = widthInt * 0.85 + "px";

		var contextMenuItem = createContextMenuItem("Regenerate thumb", regenThumbClick(obj, contextMenu));
		contextMenuItem.classList.add("regenerateThumb", "disabled");
		contextMenu.appendChild(contextMenuItem);
		var contextMenuItem = createContextMenuItem("Set thumb at current", regenThumbCurrentPosClick(obj, contextMenu));
		contextMenuItem.classList.add("setThumbAtCurrent", "disabled");
		contextMenu.appendChild(contextMenuItem);
		var contextMenuItem = createContextMenuItem("Set moneyshot at current", setMoneyShotCurrentPosClick(obj, contextMenu));
		contextMenuItem.classList.add("setMoneyShotAtCurrent", "disabled");
		contextMenu.appendChild(contextMenuItem);
		var contextMenuItem = createContextMenuItem("Settings", mediaSettingsClick(obj, contextMenu));
		contextMenu.appendChild(contextMenuItem);
		var contextCancelMenuItem = createContextMenuItem("Cancel", closeContext(contextMenu));
		contextMenu.appendChild(contextCancelMenuItem);

		divnode.appendChild(contextMenu);
		divnode.appendChild(info);

		var anode = document.createElement("a");
		anode.className += "media-link"
		
		//anode.style.width = width;
		//anode.style.height = height;
		anode.onclick = createClickHandler(obj);
		var image = document.createElement("img");
		
		if(!disableLazyLoad) {
			if (obj.thumbFileExists)
				image.dataset.src = "/thumb/" + obj.relativeThumb;
			else
				image.dataset.src = "/thumb/" + obj.thumbFileLoading;
			image.classList.add("lazy");
		} else {
			if (obj.thumbFileExists)
				image.src = "/thumb/" + obj.relativeThumb;
			else
				image.src = "/thumb/" + obj.thumbFileLoading;
			image.loading = "lazy"
		}
		image.classList.add("media-thumb-nail")
		image.style.width = thumbSizeGlobal + "px";
		image.style.height = thumbSizeGlobal - textHeight + "px";
		//image.onerror=onThumbLoadError(image, 1)
		var namenode = document.createElement("div");
		namenode.id = obj.id + "DisplayName";
		namenode.innerText = obj.displayName;
		namenode.className += "name"
		namenode.style.width = width;
		namenode.style.height = textHeight + "px";
		namenode.style.fontSize = fontSize;

		updateScriptStatus(obj, divnode, namenode);

		divnode.appendChild(anode);
		anode.appendChild(image);
		anode.appendChild(namenode);

		divnode.hidden = isFiltered(userFilterCriteria, divnode.innerText);
		medialistNode.appendChild(divnode);
		if (playingmediaItem && playingmediaItem.id === obj.id)
			setPlayingMediaItem(obj);

		if(obj.subtitle) {// Must be after divnode has been appended
			updateSubTitle(obj, divnode, contextMenu);
		}

	}
	if(!disableLazyLoad)
		setupLazyLoad();
}

function createMediaContextNode(parent, mediaItem) {
	// var regenThumbClick = function (mediaItem) {
	// 	return function () {
	// 		sendUpdateThumb(mediaItem)
	// 	}
	// };
	// var infoContainer = document.createElement("div");
	// infoContainer.id = "mediaContextContainer";
	// infoContainer.style.left = parent.style.right;
	// infoContainer.style.top = parent.style.bottom;
	// var contextMenu = document.createElement("div"); 
	// contextMenu.classList.add("media-context-menu");
	// var regenerateThumbContextItem = document.createElement("a");
	// regenerateThumbContextItem.classList.add("media-context-menu-item");
	// regenerateThumbContextItem.innerText = "Regenerate thumb";
	// regenerateThumbContextItem.onclick = regenThumbClick(mediaItem);
	// contextMenu.appendChild(regenerateThumbContextItem);
	// infoContainer.appendChild(contextMenu);
	// document.documentElement.appendChild(infoContainer);
}

function createContextMenuItem(innerText, onclick) {
	var contextMenuItem = document.createElement("div");
	contextMenuItem.classList.add("media-context-menu-item");
	contextMenuItem.innerText = innerText;
	contextMenuItem.onclick = onclick;
	return contextMenuItem;
}

function destroyMediaContextNode(event) {
	// if(event.target.classList.contains("media-context"))
	// 	return;
	// var node = document.getElementById("mediaContextContainer");
	// if(node)
	// 	document.documentElement.removeChild(node);
}

function setupLazyLoad() {
	var lazyImages = [].slice.call(document.querySelectorAll("img.lazy"));;
	if ("IntersectionObserver" in window && "IntersectionObserverEntry" in window && "intersectionRatio" in window.IntersectionObserverEntry.prototype) {
	  let lazyImageObserver = new IntersectionObserver(function(entries, observer) {
		entries.forEach(function(entry) {
		  if (entry.isIntersecting) {
			let lazyImage = entry.target;
			lazyImage.src = lazyImage.dataset.src;
			//lazyImage.srcset = lazyImage.dataset.srcset;
			lazyImage.classList.remove("lazy");
			lazyImageObserver.unobserve(lazyImage);
		  }
		});
	  });
  
	  lazyImages.forEach(function(lazyImage) {
		lazyImageObserver.observe(lazyImage);
	  });
	}
}

function updateItem(libraryItem, roles)
{
	var mediaNode;
	if(roles.findIndex(x => x == Roles.DisplayRole) > -1) {
		mediaNode = updateScriptStatus(libraryItem);
	}
	if(roles.findIndex(x => x == Roles.DecorationRole) > -1) {
		mediaNode = updateSubTitle(libraryItem, mediaNode);
	}
}

function updateSubTitle(libraryItem, mediaNode, contextMenu) {
	if(!mediaNode)
		mediaNode = document.getElementById(libraryItem.id);
	if (!mediaNode)
		return undefined;
	if(!contextMenu)
		contextMenu = document.getElementById(libraryItem.id+"ContextMenu");
	if (!contextMenu)
		return mediaNode;

	var index = mediaListGlobal.findIndex(x => x.id === libraryItem.id);

	var contextButton = document.getElementById(libraryItem.id + "InfoButton");
	var ccIcon = document.getElementById(libraryItem.id+ "ccIconImg")
	if(ccIcon)
		contextButton.removeChild(ccIcon);

	var contextMenuItems = contextMenu.getElementsByClassName("downloadSubtitle");
	if(contextMenuItems.length)
		contextMenu.removeChild(contextMenuItems[0]);
	
	if(libraryItem.subtitle) {
		var icon = document.createElement("img");
		icon.id = libraryItem.id+ "ccIconImg";
		icon.src = "://images/icons/cc.svg";
		widthInt = thumbSizeGlobal + (thumbSizeGlobal * 0.15);
		icon.style.width = widthInt * 0.10 + "px";;
		icon.style.height = widthInt * 0.10 + "px";
		icon.classList.add("media-cc");
		//icon.appendChild(iconUse);
		contextButton.appendChild(icon);
		
		var subtitle_path =  "/media" + libraryItem.subtitleRelative;
		const ext = libraryItem.subtitleRelative.substring(libraryItem.subtitleRelative.lastIndexOf("."), libraryItem.subtitleRelative.length);
		const link = document.createElement("a");
		link.href = subtitle_path;
		link.setAttribute('download', libraryItem.displayName + ext);
		link.innerText = "Download subtitle";
		//link.onclick = contextMenu.classList.add("hidden");
		var contextSubtitleMenuItem = createContextMenuItem("", undefined);
		contextSubtitleMenuItem.classList.add("downloadSubtitle");
		contextSubtitleMenuItem.appendChild(link);
		contextMenu.insertBefore(contextSubtitleMenuItem, contextMenu.firstChild);
	}

	mediaListGlobal[index].subtitle = libraryItem.subtitle;
	mediaListGlobal[index].subtitleRelative = libraryItem.subtitleRelative;
	return mediaNode;
}

function updateScriptStatus(libraryItem, mediaNode, displayNameNode) {
	if(!mediaNode)
		mediaNode = document.getElementById(libraryItem.id);
	if (!mediaNode)
		return undefined;
	if(!displayNameNode)
		displayNameNode = document.getElementById(libraryItem.id + "DisplayName");
	if(!displayNameNode)
		return undefined;
	displayNameNode.innerText = libraryItem.displayName;
	if (libraryItem.isMFS) {
		mediaNode.classList.add("media-item-mfs");
	} else {
		mediaNode.classList.remove("media-item-mfs");
	}
	if (!libraryItem.hasScript) {
		mediaNode.classList.add("media-item-noscript");
	} else {
		mediaNode.classList.remove("media-item-noscript");
	}
	var index = mediaListGlobal.findIndex(x => x.id === libraryItem.id);
	mediaListGlobal[index].displayName = libraryItem.displayName;
	mediaListGlobal[index].isMFS = libraryItem.isMFS;
	mediaListGlobal[index].hasScript = libraryItem.hasScript;
	return mediaNode;
}

function onThumbLoadError(imageElement, tries) {
	imageElement.onerror = null;
	if (tries < 3) {
		imageElement.src = '://images/icons/loading.png';
		imageElement.onerror = onThumbLoadError(imageElement, 2);
	} else {
		imageElement.src = '://images/icons/error.png';
	}
}

function removeAllChildNodes(parent) {
	while (parent.firstChild) {
		parent.removeChild(parent.firstChild);
	}
}

function sort(value, userClick) {
	if (!value)
		value = "nameAsc";
	var randomButton = document.getElementById("randomizeButton");
	if(random !== "random" && !randomButton.classList.contains("hidden"))
		randomButton.classList.add("hidden")
	switch (value) {
		case "dateDesc":
			mediaListGlobal.sort(function (a, b) {
				return new Date(b.modifiedDate) - new Date(a.modifiedDate);
			});
			break;
		case "dateAsc":
			mediaListGlobal.sort(function (a, b) {
				return new Date(a.modifiedDate) - new Date(b.modifiedDate);
			});
			break;
		case "nameAsc":
			mediaListGlobal.sort(function (a, b) {
				var nameA = a.displayName.toUpperCase(); // ignore upper and lowercase
				var nameB = b.displayName.toUpperCase(); // ignore upper and lowercase
				if (nameA < nameB) {
					return -1;
				}
				if (nameA > nameB) {
					return 1;
				}
			});
			break;
		case "nameDesc":
			mediaListGlobal.sort(function (a, b) {
				var nameA = a.displayName.toUpperCase(); // ignore upper and lowercase
				var nameB = b.displayName.toUpperCase(); // ignore upper and lowercase
				if (nameB < nameA) {
					return -1;
				}
				if (nameB > nameA) {
					return 1;
				}
			});
			break;
		case "random":
			fy(mediaListGlobal)
			randomButton.classList.remove("hidden")
		break;
	}
	if (!userClick) {
		//document.getElementById("sortBy").value = value;
		if (value) {
			document.getElementById(value.toString()).checked = true;
		}
	}
	else {
		window.localStorage.setItem("sortBy", JSON.stringify(value));
		sortByGlobal = value;
	}
	return mediaListGlobal;
}

//https://stackoverflow.com/questions/2450954/how-to-randomize-shuffle-a-javascript-array
function fy(a,b,c,d){//array,placeholder,placeholder,placeholder
	c=a.length;while(c)b=Math.random()*c--|0,d=a[c],a[c]=a[b],a[b]=d
}

function getDisplayedMediaList(showValue, userClick) {
	let filteredMediaScoped = [];
	//let mediaListGlobalCopy = JSON.parse(JSON.stringify(mediaListGlobal));
	switch (showValue) {
		case "All":
			filteredMediaScoped = JSON.parse(JSON.stringify(mediaListGlobal));
			break;
		case "3DOnly":
			filteredMediaScoped = mediaListGlobal.filter(x => x.type === MediaType.VR);
			break;
		case "2DAndAudioOnly":
			filteredMediaScoped = mediaListGlobal.filter(x => x.type === MediaType.Audio || x.type === MediaType.Video);
			break;
	}
	
	if(userClick != undefined) {
		if (!userClick) {
			document.getElementById(showValue.toString()).checked = true;
		}
	}
	return filteredMediaScoped
}

function filter(criteria) {
	userFilterCriteria = criteria;
	var filterInput = document.getElementById("filterInput");
	filterInput.enabled = false;
	var mediaItems = document.getElementsByClassName("media-item");
	for (var item of mediaItems) {
		const libraryItem = mediaListDisplayed.find(x => x.id === item.id);

		item.hidden = isFiltered(userFilterCriteria, item.textContent) || isTagFiltered(userTagFilterCriteria, libraryItem.metaData.tags);
	};
	filterInput.enabled = true;
}

function debounceFilter(criteria) {
	if (filterDebounce) 
		clearTimeout(filterDebounce);
	filterDebounce = setTimeout(function () {
		filter(criteria);
		filterDebounce = undefined;
	}, 1000);
}

function isFiltered(criteria, textToSearch) {
	if (!criteria || criteria.trim().length == 0)
		return false;
	else
		return !textToSearch.trim().toUpperCase().includes(criteria.trim().toUpperCase());
}

var onFilterByTagClicked = function (tagCheckbox) {
	return function () {
		if(tagCheckbox) {
			if(tagCheckbox.checked)
				userTagFilterCriteria.push(tagCheckbox.value);
			else {
				var index = userTagFilterCriteria.findIndex(x => x == tagCheckbox.value);
				if(index > -1)
					userTagFilterCriteria.splice(index, 1);
			}
			filterByTag(userTagFilterCriteria);
		}
	}
};

function filterByTag(filterCriteria) {
	userTagFilterCriteria = filterCriteria;
	var tagsFilterOptions = document.getElementsByName(tagCheckboxesName);
	tagsFilterOptions.forEach(x => x.enabled = false);
	var mediaItems = document.getElementsByClassName("media-item");
	for (var item of mediaItems) {
		const libraryItem = mediaListDisplayed.find(x => x.id === item.id);

		item.hidden = isTagFiltered(userTagFilterCriteria, libraryItem.metaData.tags) || isFiltered(userFilterCriteria, item.textContent);
	};
	tagsFilterOptions.forEach(x => x.enabled = true);
}

function isTagFiltered(selectedTags, mediaTags) {
	if (!selectedTags || !selectedTags.length || !mediaTags || !mediaTags.length)
		return false;
	else {
		  for(let i=0; i<selectedTags.length; i++) {
			if(!mediaTags.includes(selectedTags[i]))
				return true;
		  }
		return false;
	}
}
/* 
function onClickUseDeoWebCheckbox(checkbox)
{
	toggleUseDeo(checkbox.checked, true);
}

function toggleUseDeo(value, userClicked)
{
	if(userClicked) {
		useDeoWeb = value;
		window.localStorage.setItem("useDeoWeb", JSON.stringify(useDeoWeb));
	}
	else
		document.getElementById("useDeoWebCheckbox").checked = value;
	if(!useDeoWeb)
	{
		deoVideoNode.style.display = "none";
		thumbsContainerNode.style.maxHeight = "";
	} else {
		deoVideoNode.style.display = "block";
		onResizeDeo();
	}
}

function loadVideo(obj) {
	if(useDeoWeb) {
		deoVideoNode.setAttribute("format", x.type === MediaType.VR ? "LR" : "mono");
		deoSourceNode.setAttribute("src", "/video/" + obj.relativePath);
		deoVideoNode.setAttribute("title", obj.name);
		deoVideoNode.setAttribute("cover-image", "/thumb/" + obj.relativeThumb);
		
		if(!DEO.isStarted(deoVideoNode))
			DEO.setVolume(deoVideoNode, 0.3);
		DEO.play(deoVideoNode);
	}
} 
*/
function onClickLazyLoadCheckbox(checkbox) {
	toggleLazyLoad(checkbox.checked, true);
}
function toggleLazyLoad(value, userClicked) {
	if (userClicked) {
		externalStreaming = value;
		window.localStorage.setItem("disableLazyLoad", JSON.stringify(value));
	}
	else
		document.getElementById("disableLazyLoadCheckbox").checked = value;
	if (value) {
		stopVideo();
	}
}

function onClickExternalStreamingCheckbox(checkbox) {
	toggleExternalStreaming(checkbox.checked, true);
}
function toggleExternalStreaming(value, userClicked) {
	if (userClicked) {
		externalStreaming = value;
		window.localStorage.setItem("externalStreaming", JSON.stringify(value));
	}
	else
		document.getElementById("externalStreamingCheckbox").checked = value;
	if (value) {
		stopVideo();
	}
}
function setupTextToSpeech() {
	if (typeof speechSynthesis === 'undefined') {
		speechNotInbrowser = true;
		disableTextToSpeech();
		return;
	}

	if (speechSynthesis.onvoiceschanged !== undefined) {
		speechSynthesis.onvoiceschanged = setupVoices;
	} else {
		setTimeout(function () {
			setupVoices();
		}, 1000);
	}

	selectedVoiceIndex = JSON.parse(window.localStorage.getItem("selectedVoiceIndex"));
	if (selectedVoiceIndex === null) {
		selectedVoiceIndex = 0;
		window.localStorage.setItem("selectedVoiceIndex", JSON.stringify(selectedVoiceIndex));
	}
	enableTextToSpeech = JSON.parse(window.localStorage.getItem("enableTextToSpeech"));
	if (enableTextToSpeech === null) {
		enableTextToSpeech = true;
		window.localStorage.setItem("enableTextToSpeech", JSON.stringify(enableTextToSpeech));
	}
	toggleEnableTextToSpeech(enableTextToSpeech, false);
	speechPitch = JSON.parse(window.localStorage.getItem("speechPitch"));
	if (speechPitch === null) {
		speechPitch = 1;
		window.localStorage.setItem("speechPitch", JSON.stringify(speechPitch));
	}
	toggleSpeechPitchChange(speechPitch, false);
	speechRate = JSON.parse(window.localStorage.getItem("speechRate"));
	if (speechRate === null) {
		speechRate = 1;
		window.localStorage.setItem("speechRate", JSON.stringify(speechRate));
	}
	toggleSpeechRateChange / (speechRate, false);
	speechVolume = JSON.parse(window.localStorage.getItem("speechVolume"));
	if (speechVolume === null) {
		speechVolume = 0.5;
		window.localStorage.setItem("speechVolume", JSON.stringify(speechVolume));
	}
	toggleSpeechVolumeChange(speechVolume, false);
}

function setupVoices() {
	systemVoices = speechSynthesis.getVoices();
	var voiceSelect = document.getElementById("voiceSelect");
	for (var i = 0; i < systemVoices.length; i++) {
		var option = document.createElement('option');
		option.textContent = systemVoices[i].name + ' (' + systemVoices[i].lang + ')';
		option.value = i;

		if (systemVoices[i].default) {
			option.textContent += ' — DEFAULT';
		}

		option.setAttribute('data-lang', systemVoices[i].lang);
		option.setAttribute('data-name', systemVoices[i].name);
		voiceSelect.appendChild(option);
	}
	voiceSelect.value = selectedVoiceIndex;
}

function onVoiceSelect(index) {
	selectedVoiceIndex = index;
	window.localStorage.setItem("selectedVoiceIndex", JSON.stringify(selectedVoiceIndex));
	onTextToSpeech("Hello I am " + systemVoices[selectedVoiceIndex].name);
}
function onTextToSpeech(message) {
	if (!message || !enableTextToSpeech || typeof speechSynthesis === 'undefined') {
		return;
	}
	if (systemVoices && systemVoices.length > 0) {
		var msg = new SpeechSynthesisUtterance();
		msg.voice = systemVoices[selectedVoiceIndex];
		msg.volume = speechVolume; // From 0 to 1
		msg.rate = speechRate; // From 0.1 to 10
		msg.pitch = speechPitch; // From 0 to 2
		//msg.lang = 'en';
		msg.text = message;
		window.speechSynthesis.speak(msg);
	}
}
function onEnableTextToSpeechClick(checkbox) {
	toggleEnableTextToSpeech(checkbox.checked, true);
}
function toggleEnableTextToSpeech(value, userClicked) {
	if (userClicked) {
		enableTextToSpeech = value;
		window.localStorage.setItem("enableTextToSpeech", JSON.stringify(value));
		if (value)
			onTextToSpeech("Voice enabled");
	}
	else
		document.getElementById("enableTextToSpeech").checked = value;

	document.getElementsByName("voiceFormElement").forEach(x => {
		x.style.display = value ? "grid" : "none"
	});
}
function onSpeechPitchChange(value) {
	toggleSpeechPitchChange(value, true);
}
function toggleSpeechPitchChange(value, userClicked) {
	if (userClicked) {
		speechPitch = value;
		window.localStorage.setItem("speechPitch", JSON.stringify(value));
		onTextToSpeech("Pitch changed to " + value);
	}
	else
		document.getElementById("speechPitch").value = value;
}
function onSpeechRateChange(value) {
	toggleSpeechRateChange(value, true);
}
function toggleSpeechRateChange(value, userClicked) {
	if (userClicked) {
		speechRate = value;
		window.localStorage.setItem("speechRate", JSON.stringify(value));
		onTextToSpeech("Rate changed to " + value);
	}
	else
		document.getElementById("speechRate").value = value;
}
function onSpeechVolumeChange(value) {
	toggleSpeechVolumeChange(value, true);
}
function toggleSpeechVolumeChange(value, userClicked) {
	if (userClicked) {
		speechVolume = value;
		window.localStorage.setItem("speechVolume", JSON.stringify(value));
		onTextToSpeech("Volume changed to " + value);
	}
	else
		document.getElementById("speechVolume").value = value;
}
function disableTextToSpeech(message) {
	var textToSpeechUnavailable = document.getElementById("textToSpeechUnavailable");
	textToSpeechUnavailable.hidden = false;
	if (message) {
		textToSpeechUnavailable.innerText = message;
	}
	var enableTTSCheckbox = document.getElementById("enableTextToSpeech");
	enableTTSCheckbox.title = "Text to speech is not available in your browser."
	enableTTSCheckbox.enabled = false;
	enableTTSCheckbox.checked = false;
	toggleEnableTextToSpeech(false, true);
}

function toggleShufflePlay() {
	if(!mediaListDisplayed.length)
		return;
	shufflePlayMode = !shufflePlayMode;
	if(shufflePlayMode) {
		document.getElementById("shuffleButton").classList.add("icon-button-down");
		showVideo();
		playVideo(getNextShuffleMediaItem());
	} else {
		shufflePlayModePlayedIndexed = {};
		document.getElementById("shuffleButton").classList.remove("icon-button-down");
	}
}

function getNextShuffleMediaItem() {
	var currentDisplayedMedia = getCurrentDisplayedMediaNodes();
	if(!currentDisplayedMedia || !currentDisplayedMedia.length) {
		return;
	}
	if(Object.keys(shufflePlayModePlayedIndexed).length === currentDisplayedMedia.length) {
		shufflePlayModePlayedIndexed = {};
	}
	// Find a media item that hasnt been played.
	var randomIndex;
	var id;
	do {
		randomIndex = Math.floor(Math.random() * (currentDisplayedMedia.length - 1));
		id = currentDisplayedMedia[randomIndex].id;
	} while(shufflePlayModePlayedIndexed[id]);

	document.getElementById(id).scrollIntoView({ behavior: "smooth", block: "end", inline: "nearest" });
	shufflePlayModePlayedIndexed[id] = true;
	return currentDisplayedMedia[randomIndex];
}
function playVideo(obj) {
	if (playingmediaItem) {
		if (playingmediaItem.id === obj.id)
			return;
		clearPlayingMediaItem();
	}
	setPlayingMediaItem(mediaListGlobal[mediaListGlobal.findIndex(x => x.id==obj.id)]);
	sendMediaState();
	if(obj["subtitle"]) {
		if(!externalStreaming) {
			if(obj["subtitle"].endsWith("vtt")) { // Only vtt is supported by html video element
				const tracks = videoNode.querySelectorAll("track");
				let track;
				if(!tracks.length) {
					track = document.createElement("track");
					track.default = true;
					videoNode.appendChild(track);
				} else {
					track = tracks[0];
				}
				track.src = "/media" + obj.subtitleRelative;
			} else {
				var tracks = videoNode.querySelectorAll('track')
				for (var i = 0; i < tracks.length; i++) {
					tracks[i].src = undefined;
				}
			}
		}
	} else {
		var tracks = videoNode.querySelectorAll('track')
		for (var i = 0; i < tracks.length; i++) {
			tracks[i].src = undefined;
		}
	}
	if (externalStreaming) {
		var file_path =  "/media" + obj.relativePath;// + "?sessionID="+cookies.sessionID;
		window.open(file_path);
		return;
	}
	showVideo();
	dataLoading();
	videoSourceNode.setAttribute("src", "/media" + obj.relativePath);
	videoNode.setAttribute("title", obj.name);
	videoMediaName.innerText = obj.name;
	videoNode.setAttribute("poster", "/thumb/" + obj.relativeThumb);
	videoNode.load();
	// loadedFunscripts = [];
	// if(playingmediaItem.hasScript)
	// 	loadMediaFunscript(playingmediaItem.scriptNoExtensionRelativePath, playingmediaItem.isMFS);
	// else
	//videoNode.play();
}

function stopVideoClick() {
	if(!controlsVisible)
		return;
	stopVideo();
}
function stopVideo() {
	hideVideo();
	videoNode.pause();
	videoSourceNode.setAttribute("src", "");
	videoNode.removeAttribute("title");
	videoNode.removeAttribute("poster");
	videoMediaName.innerText = "";
	videoNode.load();
	if (playingmediaItem) {
		playingmediaItem.playing = false;
		playingmediaItem.stopped = true;
		sendMediaState();
		clearPlayingMediaItem();// Clear after sending so the correct state is applied.
	}
}

function skipVideoTo(timeInMSecs) {
	if(videoNode && playingmediaItem) {
		videoNode.currentTime = timeInMSecs;
	}
}

function onSkipToMoneyShotClick() {
	if(!controlsVisible)
		return;
	onSkipToMoneyShot();
}

function onSkipToMoneyShot() {
	if(playingmediaItem) {
		var moneyShotSecs = playingmediaItem.metaData.moneyShotSecs;
		if(moneyShotSecs < 1) {
			moneyShotSecs = videoNode.duration - (videoNode.duration * 0.1);
		}
		skipVideoTo(moneyShotSecs);
	}
}

function onToggleFilterInput(searchButton) {
	var filterInput = document.getElementById('filterInput');
	filterInput.classList.toggle('hidden');
	searchButton.classList.toggle('icon-button-down');
}

function onToggleTagInput(tagButton) {
	var filterInput = document.getElementById('tagFilterOptions');
	filterInput.classList.toggle('hidden');
	tagButton.classList.toggle('icon-button-down');
}

function setPlayingMediaItem(obj) {
	playingmediaItem = obj;
	playingmediaItemNode = document.getElementById(obj.id);
	playingmediaItemNode.classList.add("media-item-playing");
}

function clearPlayingMediaItem() {
	playingmediaItemNode.classList.remove("media-item-playing");
	playingmediaItem = null;
	playingmediaItemNode = null;
}

var timer1 = 0;
var timer2 = Date.now();
function onVideoTimeUpdate(event) {
	if (xtpConnected && selectedInputDevice == DeviceType.XTPWeb) {
		if (timer2 - timer1 >= 1000) {
			timer1 = timer2;
			sendMediaState();
		}
		timer2 = Date.now();
	}
}
function onVideoLoading(event) {
	debug("Data loading");
	if(videoStallTimeout)
		clearTimeout(videoStallTimeout);
	dataLoading();
	if(playingmediaItem)
		playingmediaItem.loaded = false;
	// SOmetimes the stall signal is sent but the loading modal is never removed.
	videoStallTimeout = setTimeout(() => {
		//playNextVideo();
		dataLoaded();
		videoStallTimeout = undefined;
	}, 20000);
}
function onVideoLoad(event) {
	debug("Data loaded");
	debug("Duration: " + videoNode.duration)
	if(playingmediaItem)
		playingmediaItem.loaded = true;
}
function onVideoPlay(event) {
	debug("Video play");
	if(playingmediaItem)
		playingmediaItem.playing = true;
	// if(!funscriptSyncWorker && loadedFunscripts && loadedFunscripts.length > 0)
	// 	startFunscriptSync(loadedFunscripts);
	sendMediaState();
}
function onVideoPause(event) { 
	debug("Video pause");
	if(playingmediaItem)
		playingmediaItem.playing = false;
	//setTimeout(function() {
	sendMediaState();// Sometimes a timeupdate is sent after this event fires?
	//}, 500);
}
function onVideoStall(event) {
	debug("Video stall");
	dataLoading();
	if(playingmediaItem)
		playingmediaItem.playing = false;
	sendMediaState();
	if(videoStallTimeout)
		clearTimeout(videoStallTimeout);
	// SOmetimes the stall signal is sent but the loading modal is never removed.
	videoStallTimeout = setTimeout(() => {
		//playNextVideo();
		dataLoaded();
		videoStallTimeout = undefined;
	}, 20000);
}
function onVideoPlaying(event) {
	debug("Video playing");
	if(videoStallTimeout)
		clearTimeout(videoStallTimeout);
	if(playingmediaItem)
		playingmediaItem.playing = true;
	sendMediaState();
}
function onVolumeChange() {
	window.localStorage.setItem("volume", videoNode.volume);
}
function onVideoEnd(event) {
	// if(funscriptSyncWorker) {
	// 	funscriptSyncWorker.postMessage(JSON.stringify({"command": "terminate"}));
	// }
	playNextVideo();
}

function playNextVideoClick() {
	if(!controlsVisible)
		return;
	playNextVideo();
}

function playNextVideo() {
	if(mediaListDisplayed.length == 0)
		return;
	if(shufflePlayMode) {
		playVideo(getNextShuffleMediaItem())
	} else {
		var currentDisplayedMedia = getCurrentDisplayedMediaNodes();
		if(!currentDisplayedMedia || !currentDisplayedMedia.length) {
			return;
		}
		if(playingmediaItem) {
			var playingIndex = currentDisplayedMedia.findIndex(x => x.id === playingmediaItem.id);
			playingIndex++;
		} else {
			playingIndex = 0;
		}
		if (playingIndex < currentDisplayedMedia.length)
			playVideo(currentDisplayedMedia[playingIndex]);
		else
			playVideo(currentDisplayedMedia[0]);
	}
}
function playPreviousVideoClick() {
	if(!controlsVisible)
		return;
	playPreviousVideo();
}
function playPreviousVideo() {
	if(mediaListDisplayed.length == 0)
		return;
	var currentDisplayedMedia = getCurrentDisplayedMediaNodes();
	if(!currentDisplayedMedia || !currentDisplayedMedia.length) {
		return;
	}
	if(playingmediaItem) {
		var playingIndex = currentDisplayedMedia.findIndex(x => x.id === playingmediaItem.id);
		playingIndex--;
	} else {
		playingIndex = -1;
	}
	if (playingIndex < 0)
		playVideo(currentDisplayedMedia[currentDisplayedMedia.length - 1]);
	else
		playVideo(currentDisplayedMedia[playingIndex]);
}
function setThumbSize(value, userClick) {
	if (!userClick) {
		if (value) {
			document.getElementById(value.toString()).checked = true;
			//document.getElementById("thumbSize").value = value.toString();
		}
	} else {
		window.localStorage.setItem("thumbSize", parseInt(value, 10));
		thumbSizeGlobal = parseInt(value, 10);
	}
}

// function startFunscriptSync() {
// 	if (window.Worker) {
// 		if(funscriptSyncWorker)
// 			funscriptSyncWorker.terminate();
// 		funscriptSyncWorker = new Worker('syncFunscript.js');
// 		funscriptSyncWorker.postMessage(JSON.stringify({
// 			"command": "startThread", 
// 			"funscripts": loadedFunscripts, 
// 			"remoteUserSettings": remoteUserSettings
// 		}));
// 		funscriptSyncWorker.onmessage = onFunscriptWorkerThreadRecieveMessage;
// 	}
// }

function sendMediaState() {
	//console.log("sendMediaState")
	if (selectedInputDevice == DeviceType.XTPWeb) {
		if (playingmediaItem) {
			postMediaState({
				"path": playingmediaItem.path,
				"playing": playingmediaItem.playing,
				"currentTime": videoNode.currentTime,
				"duration": videoNode.duration,
				"playbackSpeed": videoNode.speed
			});
		} else {
			postMediaState({
				"path": undefined,
				"playing": false,
				"currentTime": 0,
				"duration": 0,
				"playbackSpeed": 1
			});
		}
	}
}

function onFunscriptWorkerThreadRecieveMessage(e) {
	isMediaFunscriptPlaying = true;
	var data;
	if (typeof e.data === "string")
		data = JSON.parse(e.data);
	else
		data = e.data;
	switch (data["command"]) {
		case "sendTcode":
			sendTcode(data["tcode"])
			break;
		case "getMediaState":
			sendMediaState();
			break;
		case "end":
			funscriptSyncWorker.terminate();
			funscriptSyncWorker = null;
			break;
	}
}
//Settings
function openSettings() {
	getServerSessions();
	settingsNode.style.visibility = "visible";
	settingsNode.style.opacity = 1;
	document.getElementById("settingsTabs").style.display = "block";
	if (!speechNotInbrowser && (!systemVoices || systemVoices.length === 0))
		disableTextToSpeech("No voices found");
}

function closeSettings() {
	settingsNode.style.visibility = "hidden";
	settingsNode.style.opacity = 0;
	document.getElementById("settingsTabs").style.display = "none";
}

function openMetaDataModal(mediaItem) {
	selectedMediaItemMetaData = mediaItem["metaData"];
	document.getElementById("mediaOffset").value = selectedMediaItemMetaData["offset"];
	document.getElementById("moneyShotMillis").value = selectedMediaItemMetaData["moneyShotMillis"];
	const tagCheckboxes = document.getElementsByName(metadataTagCheckboxesName);
	tagCheckboxes.forEach(x => {
		x.checked = selectedMediaItemMetaData.tags.findIndex(y => y == x.value) > -1;
	})
	mediaItemSettingsModalNode.style.visibility = "visible";
	mediaItemSettingsModalNode.style.opacity = 1;
}
function closeMetaDataModal() {
	mediaItemSettingsModalNode.style.visibility = "hidden";
	mediaItemSettingsModalNode.style.opacity = 0;
	document.getElementById("saveMediaItemMetaDataButton").disabled = true;
}
function saveMetaData() {
	selectedMediaItemMetaData["offset"] = parseInt(document.getElementById("mediaOffset").value);
	selectedMediaItemMetaData["moneyShotMillis"] = parseInt(document.getElementById("moneyShotMillis").value);
	postMediaItemMetaData(selectedMediaItemMetaData);
}

function metaDataChange() {
	document.getElementById("saveMediaItemMetaDataButton").disabled = false;
}

function addToSelectedMetaData(amount) {
	var mediaOffsetNode = document.getElementById("mediaOffset");
	var value = parseInt(mediaOffsetNode.value);
	value += amount;
	mediaOffsetNode.value = value;
	metaDataChange();
}

function resetMoneyShot() {
	document.getElementById("moneyShotMillis").value = -1;
	metaDataChange();
}

var onMetadataTagCheckboxClicked = function (tagCheckbox) {
	return function () {
		let tagsChanged = false;
		if(tagCheckbox && selectedMediaItemMetaData) {
			const index = selectedMediaItemMetaData.tags.findIndex(x => x == tagCheckbox.value);
			if(tagCheckbox.checked && index == -1) {
				selectedMediaItemMetaData.tags.push(tagCheckbox.value);
				tagsChanged = true;
			} else if(!tagCheckbox.checked && index > -1) {
				selectedMediaItemMetaData.tags.splice(index, 1);
				tagsChanged = true;
			}
		}
		if(tagsChanged)
			metaDataChange();
	}
};

function tabClick(tab, tabNumber) {
	var allTabs = document.getElementsByClassName("tab-section-tab")
	for (var i = 0; i < allTabs.length; i++) {
		if (i == tabNumber)
			continue;
		var otherTab = allTabs[i];
		otherTab.style.backgroundColor = "#5E6B7F";
	}
	var allContent = document.getElementsByClassName("tab-content")
	for (var i = 0; i < allContent.length; i++) {
		if (i == tabNumber)
			continue;
		var content = allContent[i];
		content.style.display = 'none';
		content.style.opacity = "0";
	}
	allContent[tabNumber].style.display = 'block';
	allContent[tabNumber].style.opacity = "1";
	tab.style.backgroundColor = '#8DA1BF';
}

function getCurrentDisplayedMediaNodes() {
	var displayedElementIDs = Array.from(document.querySelectorAll(".media-item:not([hidden])")).map(x => x.id);
	return mediaListDisplayed.filter(x => {
		return displayedElementIDs.includes(x.id);
	});
}

function showChange(value) {
	if (!value)
		showGlobal = "All";
	else
		showGlobal = value;
	window.localStorage.setItem("show", JSON.stringify(value));
	mediaListDisplayed = getDisplayedMediaList(value, true);
	loadMedia(mediaListDisplayed);
	filter();
}

function sortChange(value) {
	sort(value, true);
	mediaListDisplayed = getDisplayedMediaList(value, false);
	loadMedia(mediaListDisplayed);
	filter();
}

function thumbSizeChange(value) {
	setThumbSize(value, true);
	loadMedia(mediaListDisplayed);
	filter();
}

async function setupSliders() {
	// Initialize Sliders
	var availableChannels = remoteUserSettings.availableChannelsArray;
	var tcodeRanges = document.getElementById("tcodeRanges");
	if(!availableChannels || availableChannels.length == 0) {
		console.error("availableChannels was null or empty when setupSliders was called");
		return;
	}
	removeAllChildNodes(tcodeRanges);
	for (var i = 0; i < availableChannels.length; i++) {
		var channel = availableChannels[i];

		var formElementNode = document.createElement("div");
		formElementNode.classList.add("formElement");

		var labelNode = document.createElement("label");
		labelNode.classList.add("range-label")
		labelNode.innerText = channel.friendlyName;
		labelNode.for = channel.channel;
		formElementNode.appendChild(labelNode);

		var sectionNode = document.createElement("section");
		sectionNode.classList.add("range-slider");
		sectionNode.id = channel.channel;
		formElementNode.appendChild(sectionNode);

		var rangeValuesNode = document.createElement("span");
		rangeValuesNode.classList.add("range-values")

		var input1Node = document.createElement("input");
		input1Node.type = "range";
		input1Node.min = channel.min;
		input1Node.max = channel.max - 1;
		input1Node.value = channel.userMin;

		var input2Node = document.createElement("input");
		input2Node.type = "range";
		input2Node.min = channel.min + 1;
		input2Node.max = channel.max;
		input2Node.value = channel.userMax;

		if (userAgentIsDeo) {
			formElementNode.style.height = "50px"
			labelNode.style.height = "50px"
		} else {
			input1Node.classList.add("range-input");
			input2Node.classList.add("range-input");
		}
		input1Node.oninput = function (input1Node, input2Node, rangeValuesNode, channel) {
			var slide1 = parseInt(input1Node.value);
			var slide2 = parseInt(input2Node.value);
			var slideMid = Math.round((slide2 + slide1) / 2);
			rangeValuesNode.innerText = slide1 + " - " + slideMid + " - " + slide2;
			if (slide2 < slide1) {
				input2Node.value = slide1 + 1;
				remoteUserSettings.availableChannels[channel.channel].userMax = input2Node.value;
			}
			remoteUserSettings.availableChannels[channel.channel].userMin = slide1;
			remoteUserSettings.availableChannels[channel.channel].userMid = slideMid;
			sendTCode(channel.channel + input1Node.value.toString().padStart(4, '0') + "S2000");
			//markXTPFormDirty();
			var debouncerOBJ = debounceTracker[channel.channel + "min"];
			if(debouncerOBJ)
				clearTimeout(debouncerOBJ);
			debounceTracker[channel.channel + "min"] = setTimeout(function () {
				sendTCodeRange(channel.channel, parseInt(input1Node.value), parseInt(input2Node.value));
				debounceTracker[channel.channel + "min"] = undefined;
			}, 1000);
		}.bind(input1Node, input1Node, input2Node, rangeValuesNode, channel);

		input2Node.oninput = function (input1Node, input2Node, rangeValuesNode, channel) {
			var slide1 = parseInt(input1Node.value);
			var slide2 = parseInt(input2Node.value);
			var slideMid = Math.round((slide2 + slide1) / 2);
			rangeValuesNode.innerText = slide1 + " - " + slideMid + " - " + slide2;
			if (slide1 > slide2) {
				input1Node.value = slide2 - 1;
				remoteUserSettings.availableChannels[channel.channel].userMin = input1Node.value;
			}
			remoteUserSettings.availableChannels[channel.channel].userMax = slide2;
			remoteUserSettings.availableChannels[channel.channel].userMid = slideMid;
			sendTCode(channel.channel + input2Node.value.toString().padStart(4, '0') + "S2000");
			//markXTPFormDirty();
			var debouncerOBJ = debounceTracker[channel.channel + "max"];
			if(debouncerOBJ)
				clearTimeout(debouncerOBJ);
			debounceTracker[channel.channel + "max"] = setTimeout(function () {
				sendTCodeRange(channel.channel, parseInt(input1Node.value), parseInt(input2Node.value));
				debounceTracker[channel.channel + "max"] = undefined;
			}, 1000);
		}.bind(input2Node, input1Node, input2Node, rangeValuesNode, channel);

		var minIncrementButton = document.createElement("button");
		minIncrementButton.classList.add("min-max-button");
		minIncrementButton.innerText = "+";
		minIncrementButton.onclick = function (input1Node, input2Node) {
			const minValue = parseInt(input1Node.value);
			const maxValue = parseInt(input2Node.value);
			const newValue = minValue + rangeChangeAmount;
			input1Node.value = newValue <= maxValue ? newValue : maxValue - rangeChangeAmount;
			input1Node.dispatchEvent(new Event('input', { bubbles: true }));
		}.bind(input1Node, input1Node, input2Node, channel);

		var minDecrementButton = document.createElement("button");
		minDecrementButton.classList.add("min-max-button");
		minDecrementButton.innerText = "-";
		minDecrementButton.onclick = function (input1Node, channel) {
			const minValue = parseInt(input1Node.value);
			const newValue = minValue - rangeChangeAmount;
			input1Node.value = newValue >= remoteUserSettings.availableChannels[channel.channel].min ? newValue : remoteUserSettings.availableChannels[channel.channel].min;
			input1Node.dispatchEvent(new Event('input', { bubbles: true }));
		}.bind(input1Node, input1Node, channel);

		var maxIncrementButton = document.createElement("button");
		maxIncrementButton.classList.add("min-max-button");
		maxIncrementButton.innerText = "+";
		maxIncrementButton.onclick = function (input2Node, channel) {
			const maxValue = parseInt(input2Node.value);
			const newValue = maxValue + rangeChangeAmount;
			input2Node.value = newValue <= remoteUserSettings.availableChannels[channel.channel].max ? newValue : remoteUserSettings.availableChannels[channel.channel].max;
			input2Node.dispatchEvent(new Event('input', { bubbles: true }));
		}.bind(input2Node, input2Node, channel);

		var maxDecrementButton = document.createElement("button");
		maxDecrementButton.classList.add("min-max-button");
		maxDecrementButton.innerText = "-";
		maxDecrementButton.onclick = function (input1Node, input2Node) {
			const minValue = parseInt(input1Node.value);
			const maxValue = parseInt(input2Node.value);
			const newValue = maxValue - rangeChangeAmount;
			input2Node.value = newValue >= minValue ? newValue : minValue + rangeChangeAmount;
			input2Node.dispatchEvent(new Event('input', { bubbles: true }));
		}.bind(input1Node, input1Node, input2Node, channel);

		var minmaxButtonSpanRow = document.createElement("span");
		minmaxButtonSpanRow.classList.add("min-max-buttons");
		var minButtonSpan = document.createElement("span");
		minButtonSpan.appendChild(minDecrementButton);
		minButtonSpan.appendChild(minIncrementButton);
		var maxButtonSpan = document.createElement("span");
		maxButtonSpan.appendChild(maxDecrementButton);
		maxButtonSpan.appendChild(maxIncrementButton);

		minmaxButtonSpanRow.appendChild(minButtonSpan);
		minmaxButtonSpanRow.appendChild(rangeValuesNode);
		minmaxButtonSpanRow.appendChild(maxButtonSpan);

		sectionNode.appendChild(minmaxButtonSpanRow);
		sectionNode.appendChild(input1Node);
		sectionNode.appendChild(input2Node);

		var slide1 = parseInt(input1Node.value);
		var slide2 = parseInt(input2Node.value);
		var slideMid = Math.round((slide2 + slide1) / 2);
		rangeValuesNode.innerText = slide1 + " - " + slideMid + " - " + slide2;

		tcodeRanges.appendChild(formElementNode);
	}
}

async function setupMotionModifiers() {
	var tab = document.getElementById("tabFunscript");
	removeAllChildNodes(tab);

	var formElementNode = document.createElement("div");
	formElementNode.classList.add("formElement");

	var headerDivNode = document.createElement("div");
	headerDivNode.classList.add("tab-content-header");
	var headerNode = document.createElement("div");
	headerNode.innerText = "Motion modifier"
	headerNode.classList.add("tab-content-header-main");
	var subtextNode = document.createElement("div");
	subtextNode.classList.add("tab-content-header-eyebrow");
	subtextNode.innerText = "Add random motion to other channels"
	headerDivNode.appendChild(headerNode);
	headerDivNode.appendChild(subtextNode);

	var labelNode = document.createElement("label");
	labelNode.innerText = "Enabled";
	labelNode.for = "multiplierEnabled"

	var sectionNode = document.createElement("section");
	sectionNode.classList.add("form-group-section");
	sectionNode.id = "multiplierEnabled";

	var multiplierEnabledNode = document.createElement("input");
	multiplierEnabledNode.id = "multiplierEnabled";
	multiplierEnabledNode.type = "checkbox";
	multiplierEnabledNode.checked = remoteUserSettings.multiplierEnabled;

	multiplierEnabledNode.oninput = function (event) {
		remoteUserSettings.multiplierEnabled = event.target.checked;
		toggleMotionModifierState(event.target.checked);
		markXTPFormDirty();
	}.bind(multiplierEnabledNode);

	sectionNode.appendChild(multiplierEnabledNode);

	var headers = [
		//"Modifier", 
		"Link to MFS", 
		"Speed"]
	headers.forEach(element => {
		var gridHeaderNode = document.createElement("div");
		gridHeaderNode.classList.add("form-group-control");
		gridHeaderNode.classList.add("form-group-control-header");
		var gridHeaderContentNode = document.createElement("span");
		gridHeaderContentNode.innerText = element;
		gridHeaderNode.appendChild(gridHeaderContentNode);
		sectionNode.appendChild(gridHeaderNode);
	});


	formElementNode.appendChild(labelNode);
	formElementNode.appendChild(sectionNode);

	tab.appendChild(headerDivNode);
	tab.appendChild(formElementNode);

	var availableChannels = remoteUserSettings.availableChannelsArray;
	for (var i = 0; i < availableChannels.length; i++) {

		var channel = availableChannels[i];
		var channelName = channel.channel;

		if (channel.dimension === AxisDimension.Heave)
			continue;

		var formElementNode = document.createElement("div");
		formElementNode.classList.add("formElement");

		var labelNode = document.createElement("label");
		labelNode.innerText = channel.friendlyName;
		labelNode.for = channelName;
		formElementNode.appendChild(labelNode);


		/* 	value["damperEnabled"] = availableChannels->value(channel).DamperEnabled;
			value["damperValue"] = availableChannels->value(channel).DamperValue;
			value["dimension"] = (int)availableChannels->value(channel).Dimension;
			value["friendlyName"] = availableChannels->value(channel).FriendlyName;
			value["linkToRelatedMFS"] = availableChannels->value(channel).LinkToRelatedMFS;
			value["max"] = availableChannels->value(channel).Max;
			value["mid"] = availableChannels->value(channel).Mid;
			value["min"] = availableChannels->value(channel).Min;
			value["multiplierEnabled"] = availableChannels->value(channel).MultiplierEnabled;
			value["multiplierValue"] = availableChannels->value(channel).MultiplierValue;
			value["relatedChannel"] = availableChannels->value(channel).RelatedChannel; */

		var sectionNode = document.createElement("section");
		sectionNode.setAttribute("name", "motionModifierSection");
		sectionNode.classList.add("form-group-section");
		sectionNode.id = channelName;

		var enabledValueNode = document.createElement("div");
		enabledValueNode.classList.add("form-group-control");

		var multiplierEnabledNode = document.createElement("input");
		multiplierEnabledNode.setAttribute("name", "motionModifierInput");
		multiplierEnabledNode.type = "checkbox";
		multiplierEnabledNode.checked = channel.multiplierEnabled;

		multiplierEnabledNode.oninput = function (channelName, event) {
			remoteUserSettings.availableChannels[channelName].multiplierEnabled = event.target.checked;
			markXTPFormDirty();
		}.bind(multiplierEnabledNode, channelName);

		// var multiplierValueNode = document.createElement("input");
		// multiplierValueNode.setAttribute("name", "motionModifierInput");
		// multiplierValueNode.value = channel.multiplierValue;

		// multiplierValueNode.oninput = function (channelName, event) {
		// 	var value = parseFloat(event.target.value);
		// 	if (value) {
		// 		remoteUserSettings.availableChannels[channelName].multiplierValue = value;
		// 		markXTPFormDirty();
		// 	}
		// }.bind(multiplierValueNode, channelName);

		enabledValueNode.appendChild(multiplierEnabledNode);
		//enabledValueNode.appendChild(multiplierValueNode);

		var linkedEnabledValueNode = document.createElement("div");
		linkedEnabledValueNode.classList.add("form-group-control");

		var linkToRelatedMFSNode = document.createElement("input");
		linkToRelatedMFSNode.setAttribute("name", "motionModifierInput");
		linkToRelatedMFSNode.type = "checkbox";
		linkToRelatedMFSNode.checked = channel.linkToRelatedMFS;

		linkToRelatedMFSNode.oninput = function (channelName, event) {
			remoteUserSettings.availableChannels[channelName].linkToRelatedMFS = event.target.checked;
			markXTPFormDirty();
		}.bind(linkToRelatedMFSNode, channelName);


		var relatedChannelNode = document.createElement("select");
		relatedChannelNode.setAttribute("name", "motionModifierInput");

		availableChannels.forEach(element => {
			if (element.channel !== channel.channel) {
				var relatedChannelOptionNode = document.createElement("option");
				relatedChannelOptionNode.innerHTML = element.friendlyName
				relatedChannelOptionNode.value = element.channel
				relatedChannelNode.appendChild(relatedChannelOptionNode)
			}
		});
		relatedChannelNode.value = channel.relatedChannel;

		relatedChannelNode.oninput = function (channelName, event) {
			remoteUserSettings.availableChannels[channelName].relatedChannel = event.target.value;
			markXTPFormDirty();
		}.bind(relatedChannelNode, channelName);

		linkedEnabledValueNode.appendChild(linkToRelatedMFSNode);
		linkedEnabledValueNode.appendChild(relatedChannelNode);

		var damperEnabledValueNode = document.createElement("div");
		damperEnabledValueNode.classList.add("form-group-control");

		var damperEnabledNode = document.createElement("input");
		damperEnabledNode.setAttribute("name", "motionModifierInput");
		damperEnabledNode.type = "checkbox";
		damperEnabledNode.checked = channel.damperEnabled;

		damperEnabledNode.oninput = function (channelName, event) {
			remoteUserSettings.availableChannels[channelName].damperEnabled = event.target.checked;
			markXTPFormDirty();
		}.bind(damperEnabledNode, channelName);

		var damperValueNode = document.createElement("input");
		damperValueNode.setAttribute("name", "motionModifierInput");
		damperValueNode.value = channel.damperValue;

		damperValueNode.oninput = function (channelName, event) {
			var value = parseFloat(event.target.value);
			if (value) {
				remoteUserSettings.availableChannels[channelName].damperValue = value;
				markXTPFormDirty();
			}
		}.bind(damperValueNode, channelName);

		damperEnabledValueNode.appendChild(damperEnabledNode);
		damperEnabledValueNode.appendChild(damperValueNode);

		sectionNode.appendChild(enabledValueNode);
		sectionNode.appendChild(linkedEnabledValueNode);
		sectionNode.appendChild(damperEnabledValueNode);

		formElementNode.appendChild(sectionNode);

		tab.appendChild(formElementNode);
	}

	toggleMotionModifierState(remoteUserSettings.multiplierEnabled);
	setUpInversionMotionModifier();
}
function toggleMotionModifierState(enabled) {
	var motionModifierElements = document.getElementsByName("motionModifierInput");
	motionModifierElements.forEach(element => {
		element.disabled = !enabled;
	})
}
async function setUpInversionMotionModifier() {

	var tab = document.getElementById("tabFunscript");

	var formElementNode = document.createElement("div");
	formElementNode.classList.add("formElement");

	var headerDivNode = document.createElement("div");
	headerDivNode.classList.add("tab-content-header");
	var subtextNode = document.createElement("div");
	subtextNode.classList.add("tab-content-header-eyebrow");
	subtextNode.innerText = "Invert motion of channels"
	headerDivNode.appendChild(subtextNode);

	tab.appendChild(headerDivNode);


	var availableChannels = remoteUserSettings.availableChannelsArray;
	for (var i = 0; i < availableChannels.length; i++) {

		var channel = availableChannels[i];
		var channelName = channel.channel;

		var formElementNode = document.createElement("div");
		formElementNode.classList.add("formElement");

		var labelNode = document.createElement("label");
		labelNode.innerText = channel.friendlyName;
		labelNode.for = channel.channel + "Inverted";
		formElementNode.appendChild(labelNode);


		/* value["inverted"] = availableChannels->value(channel).Inverted; */

		var sectionNode = document.createElement("section");
		sectionNode.setAttribute("name", "motionModifierInvertedSection");
		sectionNode.classList.add("form-group-section");
		sectionNode.id = channel.channel + "Inverted";

		var enabledValueNode = document.createElement("div");
		enabledValueNode.classList.add("form-group-control");

		var invertedEnabledNode = document.createElement("input");
		invertedEnabledNode.setAttribute("name", "motionModifierInputInverted");
		invertedEnabledNode.type = "checkbox";
		invertedEnabledNode.checked = channel.inverted;

		invertedEnabledNode.oninput = function (channelName, event) {
			remoteUserSettings.availableChannels[channelName].inverted = event.target.checked;
			markXTPFormDirty();
		}.bind(invertedEnabledNode, channelName);

		enabledValueNode.appendChild(invertedEnabledNode);
		sectionNode.appendChild(enabledValueNode);
		formElementNode.appendChild(sectionNode);

		tab.appendChild(formElementNode);
	}
}

function markXTPFormDirty() {
	xtpFormDirty = true;
	var saveToXTPButton = document.getElementById("saveToXTPButton");
	saveToXTPButton.disabled = false;
}

function markXTPFormClean() {
	xtpFormDirty = false;
	var saveToXTPButton = document.getElementById("saveToXTPButton");
	saveToXTPButton.disabled = true;
}

function setupConnectionsTab() {
	/*   
		connectionSettingsJson["networkAddress"] = SettingsHandler::getServerAddress();
		connectionSettingsJson["networkPort"] = SettingsHandler::getServerPort();
		connectionSettingsJson["serialPort"] = SettingsHandler::getSerialPort(); */
	selectedInputDevice = remoteUserSettings.connection.input.selectedDevice;
	if (selectedSyncConnectionGlobal && selectedSyncConnectionGlobal != selectedInputDevice) {
		sendInputDeviceConnectionChange(selectedSyncConnectionGlobal, true);
		selectedInputDevice = selectedSyncConnectionGlobal;
	}
	switch (selectedInputDevice) {
		case DeviceType.Deo:
			document.getElementById("connectionDeoVR").checked = true;
			break;
		case DeviceType.Whirligig:
			document.getElementById("connectionWhirligig").checked = true;
			break;
		case DeviceType.XTPWeb:
			document.getElementById("connectionXTPWeb").checked = true;
			break;
		case DeviceType.None:
			document.getElementById("connectionInputNone").checked = true;
			break;
	}
	document.getElementById("deoVRAddress").value = remoteUserSettings.connection.input.deoAddress
	document.getElementById("deoVRPort").value = remoteUserSettings.connection.input.deoPort
	document.getElementById("connectionGamepad").checked = remoteUserSettings.connection.input.gamePadEnabled;

	selectedOutputDevice = remoteUserSettings.connection.output.selectedDevice;
	if (selectedOutputConnectionGlobal && selectedOutputConnectionGlobal != selectedOutputDevice) {
		sendOutputDeviceConnectionChange(selectedOutputConnectionGlobal, true);
		selectedOutputDevice = selectedOutputConnectionGlobal;
	}
	switch (selectedOutputDevice) {
		case DeviceType.None:
			document.getElementById("connectionOutputNone").checked = true;
			break;
		case DeviceType.Network:
			document.getElementById("connectionNetwork").checked = true;
			break;
		case DeviceType.Serial:
			document.getElementById("connectionSerial").checked = true;
			break;
	}
	
	document.getElementById("networkAddress").value = remoteUserSettings.connection.output.networkAddress
	document.getElementById("networkPort").value = remoteUserSettings.connection.output.networkPort
	document.getElementById("serialPort").value = remoteUserSettings.connection.output.serialPort
}

var debouncer;
function webSocketAddressChange(e) {
	if (debouncer) {
		clearTimeout(debouncer);
	}
	debouncer = setTimeout(function () {
		debouncer = null;
		deviceAddress = document.getElementById("webSocketAddress").value;
		window.localStorage.setItem("webSocketAddress", value);
	}, 500);
}

function defaultLocalSettings() {
	showAlertWindow("Confirm", "Are you sure you want to reset ALL local settings to default? Note: This only resets the settings in this browser. Your settings stored in XTP will remain.", function () {
		window.localStorage.clear();
		window.close();//Does not work
		closeAlertWindow();
	});
}

function deleteLocalSettings() {
	showAlertWindow("Confirm", "Are you sure you want to delete ALL settings from localStorage and close the window?", function () {
		window.localStorage.clear();
		window.location.reload();
		closeAlertWindow();
	});
}
function onDeoVRAddressChange(input) {
	if(!input.value || input.value.trim().length === 0) {
		userError("Invalid Deo/Heresphere network address");
		return;
	}
	remoteUserSettings.connection.input.deoAddress = input.value.trim();
	markXTPFormDirty();
}
function onDeoVRPortChange(input) {
	if(!input.value || input.value.trim().length === 0) {
		userError("Invalid Deo/Heresphere network port");
		return;
	}
	remoteUserSettings.connection.input.deoPort = input.value.trim();
	markXTPFormDirty();
}
function onNetworkAddressChange(input) {
	if(!input.value || input.value.trim().length === 0) {
		userError("Invalid network address");
		return;
	}
	remoteUserSettings.connection.output.networkAddress = input.value.trim();
	markXTPFormDirty();
}
function onNetworkPortChange(input) {
	if(!input.value || input.value.trim().length === 0) {
		userError("Invalid network port");
		return;
	}
	remoteUserSettings.connection.output.networkPort = input.value.trim();
	markXTPFormDirty();
}
function onSerialPortChange(input) {
	remoteUserSettings.connection.output.serialPort = input.value;
	markXTPFormDirty();
}
// function connectToTcodeDevice() {
// 	webSocket = new WebSocket("ws://"+deviceAddress+"/ws");
// 	webSocket.onopen = function (event) {
// 		webSocket.send("D1");
// 	};
// 	webSocket.onmessage = function (event) {
// 		console.log(event.data);
// 		document.getElementById("webSocketStatus").innerHTML = "Connected";
// 	}
// 	webSocket.onerror = function (event) {
// 		console.log(event.data);
// 		webSocket = null;
// 		document.getElementById("webSocketStatus").innerHTML = "Error";
// 	}
// }

// function sendTcode(tcode) {
// 	console.log("Send tcode: "+tcode);
// 	if(webSocket)
// 	{
// 		webSocket.send(tcode);
// 	}
// }


// keyboardShortcuts executes the relevant functions for
// each supported shortcut key
function keyboardShortcuts(event) {
	if(!filterDebounce) {
		const { key } = event;
		switch (key) {
			case 'k':
				togglePlay();
				animatePlayback();
				if (videoNode.paused) {
					showControls();
				} else {
					hideControls();
				}
				break;
			case 'm':
				toggleMute();
				break;
			case 'f':
				toggleFullScreen();
				break;
			case 'p':
				togglePip();
				break;
			case 'ArrowRight':
				playNextVideo();
				break;
			case 'ArrowLeft':
				playPreviousVideo();
				break;
			case '+':
				volumeUp();
				break;
			case '-':
				volumeDown();
				break;
			case 'a':
				sendSkipToNextAction();
				break;
			case 'c':
				onSkipToMoneyShot();
				break;
			case 'x':
				stopVideoClick();
				break;
			
		}
	}
}