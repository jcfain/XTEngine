const scriptStatusElement = document.getElementById("scriptStatus");
const scriptStatusContainerElement = document.getElementById("scriptStatusContainer");
const scriptStatusButton = document.getElementById("scriptStatusButton");
const noScriptInputEmlement = document.getElementById("noScriptInput");

let currentChannels = {};
let isShown = false;
let lerpInterval = undefined;

function toggleScriptStatusModal() {
    scriptStatusElement.classList.toggle("hidden");
	scriptStatusButton.classList.toggle('icon-button-down');
    isShown = !scriptStatusElement.classList.contains("hidden");
}

function setupchannel(channel) {
    if(currentChannels[channel]) {
        currentChannels[channel].element.classList.remove("hidden");
        return;
    }
//     progress::-moz-progress-bar { background: blue; }
// progress::-webkit-progress-value { background: blue; }
// progress { color: blue; }
    let userChannel = remoteUserSettings.availableChannelsArray.find(x => x.channel == channel);
    let div = document.createElement("div");
    div.id = channel;
    div.classList.add("script-status-div");
    let element = document.createElement("progress");
    const channelColor = getChannelColor(channel);
    element.style.setProperty("color" , channelColor);
    element.style.setProperty("--scrollbar-background", channelColor);
    element.classList.add("script-status-progress");
    let elementLabel = document.createElement("label");
    elementLabel.classList.add("script-status-label");
    elementLabel.innerText = userChannel.friendlyName;
    let valueId = channel + "valueElement";
    elementLabel.setAttribute("for", valueId);
    element.id = valueId;
    element.min = 0;
    element.max = 100;
    element.classList.add("pregress-status");
    channelsObj = {};
    channelsObj["timeout"] = undefined;
    channelsObj["element"] = div;
    channelsObj["progress"] = element
    channelsObj["channel"] = userChannel
    channelsObj["hidden"] = false;
    currentChannels[channel] = channelsObj;
    div.appendChild(elementLabel);
    div.appendChild(element);
    scriptStatusContainerElement.appendChild(div);
}

function setChannelStatus(channel, value, time, timeType) {
    //ChannelTimeType.Interval;
    //ChannelTimeType.Speed;
    if(!isShown)
        return;
    if(!noScriptInputEmlement.classList.contains("hidden"))
        noScriptInputEmlement.classList.add("hidden");
    setupchannel(channel);
    let channelsObj = currentChannels[channel];
    if(channelsObj.timeout)
        clearTimeout(channelsObj.timeout);
    //channelsObj.progress.value = value;
    startLerp(channelsObj, value, time);
    channelsObj.timeout = setTimeout(function() {
        channelsObj.element.classList.add("hidden");
        channelsObj.hidden = true;
        if(channelsObj.lerpInterval)
            clearInterval(channelsObj.lerpInterval);
        if(Object.keys(currentChannels).every(x => currentChannels[x].hidden))
            noScriptInputEmlement.classList.remove("hidden");
    }, 10000);

    //let mapped = scale(value, 0, 100, 0, canvas.width)
    // context = element.getContext("2d");
    // let mapped = scale(value, 0, 100, 0, canvas.width)
    // lerp(context, )
}

function startLerp(channelsObj, targetValue, targetTime) {
    if(channelsObj.lerpInterval)
        clearInterval(channelsObj.lerpInterval);
    const startValue = channelsObj.progress.value;
    if(startValue == targetValue)
        return;
    channelsObj["currentTime"] = startValue > targetValue ? targetTime : 0;
    channelsObj["currentValue"] = startValue;
    // debug("startValue: "+startValue);
    // debug("targetValue: "+ targetValue);
    // debug("targetTime: "+ targetTime);
    // debug("currentTime: "+ channelsObj.currentTime);
    const interval = 10;
    const posScale = Math.abs(startValue - targetValue) * (interval / targetTime);
    channelsObj["lerpInterval"] = setInterval(function(channelsObj, targetValue, targetTime, startValue, interval, posScale) {
        // //let timeScale = scale(channelsObj.currentTime, 0, targetTime, 0.0, 1.0);
        // let timeScale = channelsObj.currentTime / targetTime;
        // debug("timescale: "+ timeScale);
        // debug("currentTime: "+ channelsObj.currentTime);
        // debug("targetTime: "+ targetTime);
        // channelsObj.progress.value = startValue == targetValue ? startValue : 
        //     startValue > targetValue ? lerp(targetValue, startValue, timeScale) : lerp(startValue, targetValue, timeScale);
        // debug("value: "+ channelsObj.progress.value);
        // channelsObj.currentTime = startValue > targetValue ? channelsObj.currentTime - interval : channelsObj.currentTime + interval;
        // debug("new currentTime: "+ channelsObj.currentTime);
        // if(startValue > targetValue ? channelsObj.currentTime <= 0 : channelsObj.currentTime >= targetTime) {
        //     debug("Clear interval target: "+targetValue);
        //     clearInterval(channelsObj.lerpInterval);
        // }
        
        channelsObj.currentTime = startValue > targetValue ? channelsObj.currentTime -= interval : channelsObj.currentTime += interval;
        startValue > targetValue ? channelsObj.currentValue -= posScale : channelsObj.currentValue += posScale;
        if(startValue > targetValue ? channelsObj.currentTime <= 0 : channelsObj.currentTime >= targetTime) {
            debug("Clear interval target: "+targetValue);
            clearInterval(channelsObj.lerpInterval);
            return;
        }
        channelsObj.progress.value = channelsObj.currentValue;
    }, interval, channelsObj, targetValue, targetTime, startValue, interval, posScale);
}
/*
 * pA : the first point
 * pB : the second point
 * level : just for coloring each state 
 * of the interpolation
 * https://codepen.io/Shadosky/pen/aZzjNr
 * Return : a point in fuction on the value of step
*/
function draw(context, pointA, pointB, level = 1){
    // if(level == 3){
    //     ctx.fillStyle = "#41a85f";
    //     ctx.strokeStyle = "#41a85f";
    // } else if(level == 2){
    //     ctx.fillStyle = "#9365B8";
    //     ctx.strokeStyle = "#9365B8";
    // } else {
      ctx.fillStyle = "#5095cd";
      ctx.strokeStyle = "#0095cd";
    // }
    
    ctx.beginPath();
    ctx.moveTo(pointA.x,pointA.y);
    ctx.lineTo(pointB.x,pointB.y);
    ctx.stroke();
    // var lerpPoint = {'x':0, 'y':0};
    // lerpPoint.x = pA.x + step*(pB.x - pA.x);
    // lerpPoint.y = pA.y + step*(pB.y - pA.y);
    
    
    // rect(lerpPoint.x - 5, lerpPoint.y - 5, 10, 10);
    
    requestAnimationFrame(update);
    //return lerpPoint;
    
  }
  function update() {
    context.clearRect(0, 0, width, height);
    drawBall(x, y, 30);
    x = lerp(x, endX, 0.1);
    y = lerp(y, endY, 0.1);
    requestAnimationFrame(update);
  }
//   function lerp(min, max, fraction) {
//     return (max - min) * fraction + min;
//   }
  function lerp(startValue, targetValue, timeScale) {
    return startValue*timeScale + targetValue*(1-timeScale);
  }