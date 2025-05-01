var volume = JSON.parse(window.localStorage.getItem("volume"));

const videoNode = document.getElementById("videoPlayer");
videoNode.addEventListener("loadeddata", onVideoLoad);
videoNode.addEventListener("play", onVideoPlay);
videoNode.addEventListener("playing", onVideoPlaying);
videoNode.addEventListener("stalled", onVideoStall);
videoNode.addEventListener("waiting", onVideoStall);
videoNode.addEventListener("pause", onVideoPause);
videoNode.addEventListener("volumechange", onVolumeChange);
videoNode.addEventListener("ended", onVideoEnd);
videoNode.addEventListener("timeupdate", onVideoTimeUpdate);

var controlsHideDebounce;
var controlsVisible = false;
var forceControlsVisible = false;
var mouseOverVideo = false;
var isFullScreen = false;
var isLandScape = false;
var isMobile = /iPhone|iPad|iPod|Android/i.test(navigator.userAgent);
var isMouseOverControls = false;
var isTouchingControls = false;
var isUsingTouch = false;
var isUsingMouse = false;
var isUsingPen = false;
var isVideoHidden = true;
/* if(screen.orientation)
	isLandScape = screen.orientation.type  === 'landscape-primary'; */

/* ==========================================================================
   #Custom HTML5 Video Player
   ========================================================================== 
   MIT https://github.com/Freshman-tech/custom-html5-video/
*/

// Select elements here
const videoControls = document.getElementsByName('video-controls');

const topControls = document.getElementById('top-controls');
const bottomControls = document.getElementById('bottom-controls');
const playButton = document.getElementById('play');
const playIcon = document.getElementById('playIcon');
const pauseIcon = document.getElementById('pauseIcon');
const timeElapsed = document.getElementById('time-elapsed');
const duration = document.getElementById('duration');
const progressBar = document.getElementById('progress-bar');
const seek = document.getElementById('seek');
const seekTooltip = document.getElementById('seek-tooltip');
const volumeButton = document.getElementById('volume-button');
const volumeIcons = document.querySelectorAll('.volume-button use');
const volumeMute = document.querySelector('use[href="#volume-mute"]');
const volumeLow = document.querySelector('use[href="#volume-low"]');
const volumeHigh = document.querySelector('use[href="#volume-high"]');
const volumeSlider = document.getElementById('volume');
const playbackAnimation = document.getElementById('playback-animation');
const bodyContainer = document.getElementById('body-container');
const videoContainer = document.getElementById('video-container');
const fullscreenButton = document.getElementById('fullscreen-button');
const fullscreenIcons = fullscreenButton.querySelectorAll('use');
//const pipButton = document.getElementById('pip-button');
const loadingAnimation = document.getElementById('loading-animation');

const videoWorks = !!document.createElement('video').canPlayType;
if (videoWorks) {
	videoNode.controls = false;
	for (let item of videoControls) {
		item.classList.add('hide');
		item.classList.remove('hidden');
	}
}

// Add functions here

function setupVideoSource(url) {
	removeVideoSource();
	let videoSourceNode = document.createElement("source");
	videoSourceNode.id = "videoSourceNode";
	videoSourceNode.addEventListener('error', onVideoError, true);
	videoNode.appendChild(videoSourceNode);
	videoSourceNode.src = url;
}
function removeVideoSource() {
	let videoSourceNode = document.getElementById("videoSourceNode");
	if(videoSourceNode) {
		//videoSourceNode.removeAttribute('src');
		videoNode.removeChild(videoSourceNode);
	}
}

// togglePlay toggles the playback state of the video.
// If the video playback is paused or ended, the video is played
// otherwise, the video is paused
function togglePlay() {
	if(!controlsVisible)
		return;
	if (videoNode.paused || videoNode.ended) {
		videoNode.play();
	} else {
		videoNode.pause();
	}
}

function videoNodeClick() {
	if(videoNode.paused) {
		videoNode.play();
	} else {
		videoNode.pause();
	}
}

// updatePlayButton updates the playback icon and tooltip
// depending on the playback state
function updatePlayButton() {
	if (videoNode.paused) {
		playButton.setAttribute('data-title', 'Play (k)');
		pauseIcon.classList.add('hidden');
		playIcon.classList.remove('hidden');
	} else {
		playButton.setAttribute('data-title', 'Pause (k)');
		pauseIcon.classList.remove('hidden');
		playIcon.classList.add('hidden');
	}
}

// formatTime takes a time length in seconds and returns the time in
// minutes and seconds
function formatTime(timeInSeconds) {
    if(timeInSeconds) {
        const result = new Date(timeInSeconds * 1000);
        var hours = Math.floor(timeInSeconds / 3600);
        var minutes = result.getMinutes();
        var seconds = result.getSeconds();
        return {
            hours: (hours < 10 ? "0" : "") + hours,
            minutes: (minutes < 10 ? "0" : "") + minutes,
            seconds: (seconds < 10 ? "0" : "") + seconds,
        };
    }
    return {
        hours: "00",
        minutes: "00",
        seconds: "00",
    };
}

// initializeVideo sets the video duration, and maximum value of the
// progressBar
function initializeVideo() {
	const videoDuration = Math.round(videoNode.duration);
	seek.setAttribute('max', videoDuration);
	progressBar.setAttribute('max', videoDuration);
	const time = formatTime(videoDuration);
	duration.innerText = `${time.hours}:${time.minutes}:${time.seconds}`;
	duration.setAttribute('datetime', `${time.hours}h ${time.minutes}m ${time.seconds}s`);
}

// updateTimeElapsed indicates how far through the video
// the current playback is by updating the timeElapsed element
function updateTimeElapsed() {
	const time = formatTime(Math.round(videoNode.currentTime));
	timeElapsed.innerText = `${time.hours}:${time.minutes}:${time.seconds}`;
	timeElapsed.setAttribute('datetime', `${time.hours}h ${time.minutes}m ${time.seconds}s`);
}

// updateProgress indicates how far through the video
// the current playback is by updating the progress bar
function updateProgress() {
	seek.value = Math.floor(videoNode.currentTime);
	progressBar.value = Math.floor(videoNode.currentTime);
}

// updateSeekTooltip uses the position of the mouse on the progress bar to
// roughly work out what point in the video the user will skip to if
// the progress bar is clicked at that point
function updateSeekTooltip(event) {
	const skipTo = Math.round(
		(event.offsetX / event.target.clientWidth) *
		parseInt(event.target.getAttribute('max'), 10)
	);
	seek.setAttribute('data-seek', skipTo);
	const t = formatTime(skipTo);
	seekTooltip.textContent = `${t.hours}:${t.minutes}:${t.seconds}`;
	const rect = videoNode.getBoundingClientRect();
	seekTooltip.style.left = `${event.pageX - rect.left}px`;
}

// skipAhead jumps to a different point in the video when the progress bar
// is clicked
function skipAhead(event) {
	if(!controlsVisible)
		return;
	const skipTo = event.target.dataset.seek
		? event.target.dataset.seek
		: event.target.value;
	videoNode.currentTime = skipTo;
	progressBar.value = skipTo;
	seek.value = skipTo;
}

// updateVolume updates the video's volume
// and disables the muted state if active
function updateVolume() {
	if (videoNode.muted) {
		videoNode.muted = false;
	}

	videoNode.volume = volumeSlider.value;
}
function updateVolumeClick() {
	if(!controlsVisible)
		return;
	updateVolume();
}

function volumeUp() {
	var currentVolume = parseFloat(volumeSlider.value);
	var newVolume = currentVolume+0.05;
	if(newVolume <= volumeSlider.max) {
		volumeSlider.value = newVolume;
		updateVolume();
		updateVolumeIcon();
	}
}
function volumeDown() {
	var currentVolume = parseFloat(volumeSlider.value);
	var newVolume = currentVolume-0.05;
	if(newVolume >= volumeSlider.min) {
		volumeSlider.value = newVolume;
		updateVolume();
		updateVolumeIcon();
	}
}
function setPlayRate(value) {
	videoNode.playbackRate = value;
	sendWebsocketMessage("setPlaybackRate", value);
}
function resetPlayRate() {
	videoNode.playbackRate = 1;
	document.getElementById("playbackRateInput").value = 1.0;
}
// updateVolumeIcon updates the volume icon so that it correctly reflects
// the volume of the video
function updateVolumeIcon() {
	volumeIcons.forEach((icon) => {
		icon.classList.add('hidden');
	});

	volumeButton.setAttribute('data-title', 'Mute (m)');

	if (videoNode.muted || videoNode.volume === 0) {
		volumeMute.classList.remove('hidden');
		volumeButton.setAttribute('data-title', 'Unmute (m)');
	} else if (videoNode.volume > 0 && videoNode.volume <= 0.5) {
		volumeLow.classList.remove('hidden');
	} else {
		volumeHigh.classList.remove('hidden');
	}
}

// toggleMute mutes or unmutes the video when executed
// When the video is unmuted, the volume is returned to the value
// it was set to before the video was muted
function toggleMute() {
	videoNode.muted = !videoNode.muted;

	if (videoNode.muted) {
		volumeSlider.setAttribute('data-volume', volumeSlider.value);
		volumeSlider.value = 0;
	} else {
		volumeSlider.value = volumeSlider.dataset.volume;
	}
}
function toggleMuteClick() {
	if(!controlsVisible)
		return;
	toggleMute();
}

// animatePlayback displays an animation when
// the video is played or paused
function animatePlayback() {
	playbackAnimation.animate(
		[
			{
				opacity: 1,
				transform: 'scale(1)',
			},
			{
				opacity: 0,
				transform: 'scale(1.3)',
			},
		],
		{
			duration: 500,
		}
	);
}
function animatePlaybackVideoNodeClick() {
	if(!controlsVisible)
		return;
	animatePlayback();
}

function toggleFullScreenClick() {
	if(!controlsVisible)
		return;
	toggleFullScreen();
}
// toggleFullScreen toggles the full screen state of the video
// If the browser is currently in fullscreen mode,
// then it should exit and vice versa.
function toggleFullScreen() {
	if(!isVideoShown()) {
		return;
	}
	if (!showEmbedded())
		showFullScreen();
}

function showEmbedded() {
	if (document.fullscreenElement) {
		//console.log("exitFullscreen");
		if(isFullScreen) 
			document.exitFullscreen();
		// videoNode.classList.add("video", "video-shown-embeded");
		// setVideoFixedIfLowHeight();
		return true;
	} else if (document.webkitFullscreenElement) {
		// Need this to support Safari
		//console.log("webkitExitFullscreen");
		if(isFullScreen) 
			document.webkitExitFullscreen();
		// videoNode.classList.add("video", "video-shown-embeded");
		// setVideoFixedIfLowHeight();
		return true;
	} else if (document.mozFullscreenElement) {
		// Need this to support Safari
		if(isFullScreen) 
			document.mozExitFullscreen();
		//console.log("mozExitFullscreen");
		// videoNode.classList.add("video", "video-shown-embeded");
		// setVideoFixedIfLowHeight();
		return true;
	} 
	console.log("NO ExitFullscreen!");
	return false;
}
function showFullScreen() {
	if(!isVideoShown()) {
		return;
	}
	if (videoContainer.requestFullscreen) {
		//console.log("requestFullscreen");
		// videoNode.classList.remove("video", "video-shown-embeded");
		if(!isFullScreen) 
			videoContainer.requestFullscreen();
	} else if (videoContainer.webkitRequestFullscreen) {
		// Need this to support Safari
		//console.log("webkitRequestFullscreen");
		// videoNode.classList.remove("video", "video-shown-embeded");
		if(!isFullScreen) 
			videoContainer.webkitRequestFullscreen();
	} else if (videoContainer.mozRequestFullScreen) {
        // This is how to go into fullscren mode in Firefox
        // Note the "moz" prefix, which is short for Mozilla.
		// videoNode.classList.remove("video", "video-shown-embeded");
		if(!isFullScreen) 
        	videoContainer.mozRequestFullScreen();
    } 
}

// fullscreenChange changes the icon of the full screen button
// and tooltip to reflect the current full screen state of the video
// Also shows fixed embeded if landscape.
function fullscreenChange() {
	fullscreenIcons.forEach((icon) => icon.classList.toggle('hidden'));

	if (document.fullscreenElement) {
		fullscreenButton.setAttribute('data-title', 'Exit full screen (f)');
		videoNode.classList.remove("video", "video-shown-embeded");
		isFullScreen = true;
	} else {
		fullscreenButton.setAttribute('data-title', 'Full screen (f)');
		if(!isVideoHidden)
			videoNode.classList.add("video-shown-embeded");
		videoNode.classList.add("video");
		isFullScreen = false;
	}
	setVideoFixedIfLowHeight();
}

// togglePip toggles Picture-in-Picture mode on the video
async function togglePip() {
/* 	try {
		if (videoNode !== document.pictureInPictureElement) {
			pipButton.disabled = true;
			await videoNode.requestPictureInPicture();
		} else {
			await document.exitPictureInPicture();
		}
	} catch (error) {
		console.error(error);
	} finally {
		pipButton.disabled = false;
	} */
}

function disableForceControlsVisible() {
	forceControlsVisible = false;
}

function hideControlsEvent() {
	if (forceControlsVisible || videoNode.paused || 
		(isMouseOverControls && isUsingMouse) || 
		(isTouchingControls && isUsingTouch) || 
		(isMouseOverControls && !isMobile) || 
		(isTouchingControls && isMobile)) {
		return;
	}
	hideControls();
}
function hideControls(force) {
	if(typeof force == "boolean" && !force)
		forceControlsVisible = force;
	if(forceControlsVisible)
		return;
	for (let item of videoControls) 
		item.classList.add('hide');
	controlsVisible = false;
}

function showControls(force = false) {
	if(typeof force == "boolean" && !forceControlsVisible)
		forceControlsVisible = force;
    setTimeout(function () {
		controlsVisible = true;
    }, 500);
	for (let item of videoControls) 
		item.classList.remove('hide');
    if(controlsHideDebounce) {
        clearTimeout(controlsHideDebounce);
    }
	if(!forceControlsVisible)
		hideControlsAfterTimeout();
}
function hideControlsAfterTimeout() {
    controlsHideDebounce = setTimeout(function () {
        hideControlsEvent();
    }, 3000);
}

function showControlsIfMouseInVideo(event) {
    if(mouseOverVideo) {
        showControls();
    }
}

function mouseEnterVideo() {
    mouseOverVideo = true;
}

function mouseLeaveVideo() {
    mouseOverVideo = false;
}

function showVideo() {
	videoNode.classList.add("video-shown", "video-shown-embeded");
	setVideoFixedIfLowHeight();
	isVideoHidden = false;
}

function hideVideo() {
	isVideoHidden = true;
	dataLoaded();
	hideControls(true);
	// videoContainer.classList.remove('video-container-fixed');
	if(isFullScreen)
		showEmbedded();
	videoNode.classList.remove("video-shown", "video-shown-embeded");
}

function isVideoShown() {
	return videoNode.classList.contains("video-shown");
}

function dataLoaded() {
	loadingAnimation.classList.add('hidden');
}
function dataLoading() {
	if(isVideoShown())
		loadingAnimation.classList.remove('hidden');
}

// function checkInView(container, element, partial) {

//     //Get container properties
//     let cTop = container.scrollTop;
//     let cBottom = cTop + container.clientHeight;

//     //Get element properties
//     let eTop = element.offsetTop;
//     let eBottom = eTop + element.clientHeight;

//     //Check if in view    
//     let isTotal = (eTop >= cTop && eBottom <= cBottom);
//     let isPartial = partial && (
//       (eTop < cTop && eBottom > cTop) ||
//       (eBottom > cBottom && eTop < cBottom)
//     );

//     //Return outcome
//     return  (isTotal  || isPartial);
// }

function checkOrientation() {
	if(screen.orientation) {
		isLandScape = screen.orientation.type.includes('landscape');;
	} else if(window.orientation) {
		isLandScape = Math.abs(window.orientation) == 90;
	}
}
function setVideoFixedIfLowHeight() {
	if(document.documentElement.clientHeight < 600 && !isFullScreen && isVideoShown())
	{
		videoContainer.classList.add('video-container-fixed');
	}
	else {
		videoContainer.classList.remove('video-container-fixed');
	}
}
function setPointerType(event) {
	// Call the appropriate pointer type handler
	switch (event.pointerType) {
	  case 'mouse':
		isUsingMouse = true;
		break;
	  case 'pen':
		isUsingPen = true;
		break;
	  case 'touch':
		isUsingTouch = true;
		break;
	  default:
		console.log(`pointerType ${event.pointerType} is not supported`);
	}
}

function controlPointerEventEnd(event) {
    if(controlsHideDebounce) {
        clearTimeout(controlsHideDebounce);
    }
	hideControlsAfterTimeout();
}

// Add eventlisteners here
playButton.addEventListener('click', togglePlay);
videoNode.addEventListener('play', updatePlayButton);
videoNode.addEventListener('playing', dataLoaded);
videoNode.addEventListener('loadeddata', hideControls);
videoNode.addEventListener('loadeddata', dataLoaded);
videoNode.addEventListener('pause', updatePlayButton);
videoNode.addEventListener('loadedmetadata', initializeVideo);
videoNode.addEventListener('timeupdate', updateTimeElapsed);
videoNode.addEventListener('timeupdate', updateProgress);
videoNode.addEventListener('volumechange', updateVolumeIcon);
if(!isMobile) {
	videoNode.addEventListener('click', videoNodeClick);
	videoNode.addEventListener('click', animatePlaybackVideoNodeClick);
}
videoContainer.addEventListener('mouseenter', showControls);
videoContainer.addEventListener('mouseleave', hideControlsEvent);
videoContainer.addEventListener('mouseenter', mouseEnterVideo);
videoContainer.addEventListener('mouseleave', mouseLeaveVideo);
videoContainer.addEventListener('mousemove', showControlsIfMouseInVideo);

topControls.addEventListener('mouseenter', function(e) {
	if(controlsVisible)
		isMouseOverControls = true;
});
bottomControls.addEventListener('mouseenter', function(e) {
	if(controlsVisible)
		isMouseOverControls = true;
});
topControls.addEventListener('mouseleave', function(e) {
	isMouseOverControls = false;
	controlPointerEventEnd(e);
});
bottomControls.addEventListener('mouseleave', function(e) {
	isMouseOverControls = false;
	controlPointerEventEnd(e);
});
topControls.addEventListener('touchstart', function(e) {
	if(controlsVisible)
		isTouchingControls = true;
});
bottomControls.addEventListener('touchstart', function(e) {
	if(controlsVisible)
		isTouchingControls = true;
});
topControls.addEventListener('touchend', function(e) {
	isTouchingControls = false;
	controlPointerEventEnd(e);
});
bottomControls.addEventListener('touchend', function(e) {
	isTouchingControls = false;
	controlPointerEventEnd(e);
});
topControls.addEventListener('pointerdown', setPointerType, false);
bottomControls.addEventListener('pointerdown', setPointerType, false);
seek.addEventListener('mousemove', updateSeekTooltip);
seek.addEventListener('input', skipAhead);
volumeSlider.addEventListener('input', updateVolumeClick);
volumeButton.addEventListener('click', toggleMuteClick);
fullscreenButton.addEventListener('click', toggleFullScreenClick);
videoContainer.addEventListener('fullscreenchange', fullscreenChange);
/* Firefox */
videoContainer.addEventListener("mozfullscreenchange", fullscreenChange);
/* Chrome, Safari and Opera */
videoContainer.addEventListener("webkitfullscreenchange", fullscreenChange);
/* IE / Edge */
videoContainer.addEventListener("msfullscreenchange", fullscreenChange);
/* pipButton.addEventListener('click', togglePip);

document.addEventListener('DOMContentLoaded', () => {
	if (document.pictureInPictureEnabled) {
		pipButton.classList.add('hidden');
	}
}); */
/* if(screen.orientation) {
	screen.orientation.addEventListener('change', function(e) {
		checkOrientation();
		//console.log("screen.orientation: "+e.currentTarget.type);
	});
}  */

window.addEventListener('resize', function(e) {
	checkOrientation();
	setVideoFixedIfLowHeight();
});
	

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
function onVideoError(event) {
	//systemError("There was an error loading media: "+ event.target.error + " " + event.target.parentNode.error);
	if(videoSourceNode.src && videoSourceNode.src.length > 0) {
		systemError("There was an error loading media");
		videoSourceNode.src = undefined;
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
	debug("Video loaded");
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
		debug("Video stall timeout");
		playNextVideo();
		dataLoaded();
		videoStallTimeout = undefined;
	}, 10000);
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
	debug("Video ended");
	// if(funscriptSyncWorker) {
	// 	funscriptSyncWorker.postMessage(JSON.stringify({"command": "terminate"}));
	// }
	playNextVideo();
}

/* var scrollEventLimiter;
bodyContainer.addEventListener('scroll', (event) => {
	if(scrollEventLimiter)
		clearTimeout(scrollEventLimiter);
	scrollEventLimiter = setTimeout(() => {
		scrollEventLimiter = undefined;
		setVideoFixedIfLowHeight();
	}, 50);
}); */

videoNode.volume = volume ? volume : 0.5;
volumeSlider.value = videoNode.volume;
updateVolumeIcon();