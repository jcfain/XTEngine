

var volume = JSON.parse(window.localStorage.getItem("volume"));

const videoNode = document.getElementById("videoPlayer");
const videoSourceNode = document.getElementById("videoSource");

var controlsHideDebounce;
var mouseOverVideo = false;
var isFullScreen = false;

/* ==========================================================================
   #Custom HTML5 Video Player
   ========================================================================== 
   MIT https://github.com/Freshman-tech/custom-html5-video/
*/

// Select elements here
const videoControls = document.getElementById('video-controls');
const playButton = document.getElementById('play');
const playbackIcons = document.querySelectorAll('.playback-icons use');
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
const fullscreenButton = document.getElementById('fullscreen-button');
const videoContainer = document.getElementById('video-container');
const fullscreenIcons = fullscreenButton.querySelectorAll('use');
const pipButton = document.getElementById('pip-button');

const videoWorks = !!document.createElement('video').canPlayType;
if (videoWorks) {
	videoNode.controls = false;
	videoControls.classList.add('hide');
	videoControls.classList.remove('hidden');
}

// Add functions here

// togglePlay toggles the playback state of the video.
// If the video playback is paused or ended, the video is played
// otherwise, the video is paused
function togglePlay() {
	if (videoNode.paused || videoNode.ended) {
		videoNode.play();
	} else {
		videoNode.pause();
	}
}

// updatePlayButton updates the playback icon and tooltip
// depending on the playback state
function updatePlayButton() {
	playbackIcons.forEach((icon) => icon.classList.toggle('hidden'));

	if (videoNode.paused) {
		playButton.setAttribute('data-title', 'Play (k)');
	} else {
		playButton.setAttribute('data-title', 'Pause (k)');
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

// toggleFullScreen toggles the full screen state of the video
// If the browser is currently in fullscreen mode,
// then it should exit and vice versa.
function toggleFullScreen() {
	if (document.fullscreenElement) {
		document.exitFullscreen();
	} else if (document.webkitFullscreenElement) {
		// Need this to support Safari
		document.webkitExitFullscreen();
	} else if (videoContainer.webkitRequestFullscreen) {
		// Need this to support Safari
		videoContainer.webkitRequestFullscreen();
	} else {
		videoContainer.requestFullscreen();
	}
}

// updateFullscreenButton changes the icon of the full screen button
// and tooltip to reflect the current full screen state of the video
function updateFullscreenButton() {
	fullscreenIcons.forEach((icon) => icon.classList.toggle('hidden'));

	if (document.fullscreenElement) {
		fullscreenButton.setAttribute('data-title', 'Exit full screen (f)');
        videoNode.classList.remove("video", "video-shown")
	} else {
		fullscreenButton.setAttribute('data-title', 'Full screen (f)');
        videoNode.classList.add("video", "video-shown")
	}
}

// togglePip toggles Picture-in-Picture mode on the video
async function togglePip() {
	try {
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
	}
}

function hideControls() {
	if (videoNode.paused) {
		return;
	}

	videoControls.classList.add('hide');
}


function showControls() {
	videoControls.classList.remove('hide');
    if(controlsHideDebounce) {
        clearTimeout(controlsHideDebounce);
    }
    controlsHideDebounce = setTimeout(function () {
        hideControls();
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
// Add eventlisteners here

playButton.addEventListener('click', togglePlay);
videoNode.addEventListener('play', updatePlayButton);
videoNode.addEventListener('pause', updatePlayButton);
videoNode.addEventListener('loadedmetadata', initializeVideo);
videoNode.addEventListener('timeupdate', updateTimeElapsed);
videoNode.addEventListener('timeupdate', updateProgress);
videoNode.addEventListener('volumechange', updateVolumeIcon);
videoNode.addEventListener('click', togglePlay);
videoNode.addEventListener('click', animatePlayback);
videoContainer.addEventListener('mouseenter', showControls);
videoContainer.addEventListener('mouseleave', hideControls);
videoContainer.addEventListener('mouseenter', mouseEnterVideo);
videoContainer.addEventListener('mouseleave', mouseLeaveVideo);
videoContainer.addEventListener('mousemove', showControlsIfMouseInVideo);
// videoControls.addEventListener('mouseenter', showControls);
// videoControls.addEventListener('mouseleave', hideControls);
seek.addEventListener('mousemove', updateSeekTooltip);
seek.addEventListener('input', skipAhead);
volumeSlider.addEventListener('input', updateVolume);
volumeButton.addEventListener('click', toggleMute);
fullscreenButton.addEventListener('click', toggleFullScreen);
videoContainer.addEventListener('fullscreenchange', updateFullscreenButton);
/* Firefox */
videoContainer.addEventListener("mozfullscreenchange", updateFullscreenButton);
/* Chrome, Safari and Opera */
videoContainer.addEventListener("webkitfullscreenchange", updateFullscreenButton);
/* IE / Edge */
videoContainer.addEventListener("msfullscreenchange", updateFullscreenButton);
pipButton.addEventListener('click', togglePip);

document.addEventListener('DOMContentLoaded', () => {
	if (document.pictureInPictureEnabled) {
		pipButton.classList.add('hidden');
	}
});


videoNode.volume = volume ? volume : 0.5;
volumeSlider.value = videoNode.volume;
updateVolumeIcon();