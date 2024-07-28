const scriptStatusElement = document.getElementById("scriptStatus");
const scriptStatusContainerElement = document.getElementById("scriptStatusContainer");
const scriptStatusButton = document.getElementById("scriptStatusButton");

let currentMedia = undefined;
let currentChannels = {};
let isShown = false;

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
    let userChannel = remoteUserSettings.availableChannelsArray.find(x => x.channel == channel);
    let div = document.createElement("div");
    div.id = channel;
    div.classList.add("script-status-div");
    let element = document.createElement("progress");
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
    setupchannel(channel);
    let channelsObj = currentChannels[channel];
    if(channelsObj.timeout)
        clearTimeout(channelsObj.timeout);
    channelsObj.progress.value = value;
    channelsObj.timeout = setTimeout(function() {
        channelsObj.element.classList.add("hidden");
    }, 10000);

    //let mapped = scale(value, 0, 100, 0, canvas.width)
    // context = element.getContext("2d");
    // let mapped = scale(value, 0, 100, 0, canvas.width)
    // lerp(context, )
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
  function lerp(min, max, fraction) {
    return (max - min) * fraction + min;
  }