const scriptStatusElement = document.getElementById("scriptStatusModal");
const scriptStatusContainerElements = document.getElementsByName("scriptStatusContainer");
const scriptStatusModalButton = document.getElementById("scriptStatusModalButton");
const noScriptInputEmlements = document.getElementsByName("noScriptInput");

const scriptStatusPlayerContainer = document.getElementById("scriptStatusPlayerContainer");
const scriptStatusPlayerButton = document.getElementById("scriptStatusPlayerButton");

let currentChannels = {};
let isShown = false;
let isModalShown = false;
let isPlayerShown = false;
let lerpInterval = undefined;
let isScriptStatusPaused = false;


function toggleScriptStatusPlayer() {
    scriptStatusPlayerContainer.classList.toggle("hidden");
	scriptStatusPlayerButton.classList.toggle('icon-button-down');
    updateShown();
    isPlayerShown = !scriptStatusPlayerContainer.classList.contains("hidden");
    if(isPlayerShown) {
        showControls(true);
    } else {
        disableForceControlsVisible();
        hideControlsAfterTimeout();
    }
}
function toggleScriptStatusModal() {
    scriptStatusElement.classList.toggle("hidden");
	scriptStatusModalButton.classList.toggle('icon-button-down');
    updateShown();
    isModalShown = !scriptStatusElement.classList.contains("hidden");
}

function updateShown() {
    isShown = !scriptStatusElement.classList.contains("hidden") || !scriptStatusPlayerContainer.classList.contains("hidden");
}

function setPauseScriptStatus(isPaused) {
    isScriptStatusPaused = isPaused;
}

function setupchannel(channel) {
    if(currentChannels[channel]) {
        currentChannels[channel].animations.forEach(element => {
            element.show();
        });;
        return;
    }
// progress::-moz-progress-bar { background: blue; }
// progress::-webkit-progress-value { background: blue; }
// progress { color: blue; }
    let userChannel = remoteUserSettings.availableChannelsArray.find(x => x.channel == channel);
    currentChannels[channel] = {};
    currentChannels[channel].animations = [];
    for (let i = 0; i < scriptStatusContainerElements.length; i++) {
        const element = scriptStatusContainerElements[i];
        let channelAnimation = new ChannelAnimation()
        element.appendChild(channelAnimation.create(userChannel));
        currentChannels[channel].animations.push(channelAnimation);
        
    }
    // let channelAnimation = new ChannelAnimation()
    // scriptStatusContainerElement.appendChild(channelAnimation.create(userChannel));
    // currentChannels[channel] = channelAnimation;
}

function setChannelStatus(channel, value, time, timeType) {
    //ChannelTimeType.Interval;
    //ChannelTimeType.Speed;
    if(!isShown)
        return;
    for (let index = 0; index < noScriptInputEmlements.length; index++) {
        const noScriptInputEmlement = noScriptInputEmlements[index];
        if(!noScriptInputEmlement.classList.contains("hidden"))
            noScriptInputEmlement.classList.add("hidden");
    }
    setupchannel(channel);
    debug(`${channel} enter setChannelStatus value: ${value} time: ${time} timeType: ${timeType}`);

    for (let i = 0; i < currentChannels[channel].animations.length; i++) {
        let channelsAnimation = currentChannels[channel].animations[i];
        if(channelsAnimation.timeout)
            clearTimeout(channelsAnimation.timeout);

        channelsAnimation.setChannelStatus(value, time, timeType);

        channelsAnimation.timeout = setTimeout(function() {
            for (let i = 0; i < currentChannels[channel].animations.length; i++) {
                channelsAnimation.hide();
                if(Object.keys(currentChannels).every(x => currentChannels[x].animations.every(y => y.hidden)))
                for (let index = 0; index < noScriptInputEmlements.length; index++) {
                    const noScriptInputEmlement = noScriptInputEmlements[index];
                    noScriptInputEmlement.classList.remove("hidden");
                }
            }
        }, 10000);
    }
}


class ChannelAnimation { 
    hidden = false;
    timeout = undefined;

    // Private
    parent;
    label;
    progress
    channel;
    targetValue = 0;
    targetValue = 0;
    targetTime = 0;
    startValue = 0;
    startTime = 0;
    elapsedMillis = 0;

    lastAnimation;

    constructor() { }

    create(userChannel) {
        this.channel = userChannel;
        this.parent = document.createElement("div");
        this.parent.id = userChannel.channel + "ParentElement";
        this.parent.classList.add("script-status-div");
        this.progress = document.createElement("progress");
        const channelColor = getChannelColor(userChannel.channel);
        this.progress.style.setProperty("color" , channelColor);
        this.progress.style.setProperty("--scrollbar-background", channelColor);
        this.progress.classList.add("script-status-progress");
        this.label = document.createElement("label");
        this.label.classList.add("script-status-label");
        this.label.innerText = userChannel.friendlyName;
        let valueId = userChannel.channel + "ValueElement";
        this.label.setAttribute("for", valueId);
        this.progress.id = valueId;
        this.progress.min = 0;
        this.progress.max = 100;
        this.progress.classList.add("pregress-status");
        this.parent.appendChild(this.label);
        this.parent.appendChild(this.progress);
        return this.parent;
    }
    setChannelStatus(value, time, timeType) {
        if(this.lastAnimation) {
            cancelAnimationFrame(this.lastAnimation);
            debug(`${this.channel.channel} cancelAnimationFrame`);
            this.lastAnimation = undefined;
        }
        this.startValue = this.progress.value;
        this.startTime = 0;
        this.targetValue = value;
        this.targetTime = time;
        if(this.progress.value == value)
            return;
        debug(`${this.channel.channel} setChannelStatus startValue: ${this.startValue} targetValue: ${this.targetValue} targetTime: ${this.targetTime}`);
        //debug(`${this.channel.channel} startTime: ${this.startTime}`);
        this.label.innerText = `${this.channel.friendlyName}: ${this.targetValue} ${isScriptStatusPaused? ": paused" : ""}`
        this.lastAnimation = requestAnimationFrame(this.animate.bind(this));
    }
    hide() {
        this.parent.classList.add("hidden");
        this.hidden = true;
    }
    show() {
        if(!this.hidden)
        this.parent.classList.remove("hidden");
        this.hidden = false;
    }
    animate(timeStamp) {
        if (!this.startTime || isScriptStatusPaused) {
            this.startTime = timeStamp;
        } 
        if(!isScriptStatusPaused) {
            this.elapsedMillis = timeStamp - this.startTime;
            let downStroke = this.startValue > this.targetValue;
            let timeScale = downStroke ? scale(this.elapsedMillis, this.targetTime, 0, 0.0, 1.0) : scale(this.elapsedMillis, 0, this.targetTime, 0.0, 1.0);
            if(isNaN(timeScale))
                return;
            this.progress.value = downStroke ? lerp(this.targetValue, this.startValue, timeScale) : lerp(this.startValue, this.targetValue, timeScale);
            debug(`${this.channel.channel} ${downStroke ? "DownStroke" : "UpStroke"} targetValue: ${round(this.targetValue, 2)} targetTime: ${round(this.targetTime, 2)} elapsedMillis: ${round(this.elapsedMillis, 2)} timescale: ${round(timeScale,2 )} progressValue: ${round(this.progress.value, 2)}`);
            if(this.progress.value == this.targetValue) {
                debug(`${this.channel.channel} Target value reached: ${this.targetValue}`);
                debug(`${this.channel.channel} progressValue: ${this.progress.value}`);
                return;
            }
            if(this.elapsedMillis >= this.targetTime) {
                debug(`${this.channel.channel} Target time reached: ${this.targetTime}`);
                debug(`${this.channel.channel} elapsedMillis: ${this.elapsedMillis}`);
                return;
            }
        } else {
            this.label.innerText = `${this.channel.friendlyName}: ${this.targetValue} ${isScriptStatusPaused? ": paused" : ""}`
        }
        requestAnimationFrame(this.animate.bind(this));
    }
    // https://stackoverflow.com/questions/46722471/how-to-interpolate-between-2-points-in-a-requestanimationframe-loop
    // https://github.com/chenglou/tween-functions/blob/master/index.js
    linear(time, start, end, duration) {
        var c = end - start;
        return c * time / duration + start;
    }
}