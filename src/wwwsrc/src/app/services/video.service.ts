import { Injectable } from '@angular/core';
import { BehaviorSubject, debounceTime, distinctUntilChanged, Observable, Subject, throttleTime } from 'rxjs';
import { WebsocketService } from './websocket.service';
import { AppStateService } from './app-state.service';
import { MediaType } from '../models/enums';
import { MediaItem, XTPWebMediaState } from '../models/interfaces';
import { LocalSettingsService } from './local-settings.service';
import { HttpClient } from '@angular/common/http';
import { MessageHandlerService } from './message-handler.service';

@Injectable({
  providedIn: 'root'
})
export class VideoService {
  private volumeSubject = new BehaviorSubject<number>(0.5);
  volume$: Observable<number> = this.volumeSubject.asObservable();

  private isPlayingSubject = new BehaviorSubject<boolean>(false);
  isPlaying$: Observable<boolean> = this.isPlayingSubject.asObservable();

  private isPausedSubject = new BehaviorSubject<boolean>(false);
  isPaused$: Observable<boolean> = this.isPausedSubject.asObservable();

  private isFullScreenSubject = new BehaviorSubject<boolean>(false);
  isFullScreen$: Observable<boolean> = this.isFullScreenSubject.asObservable();

  private controlsVisibleSubject = new BehaviorSubject<boolean>(false);
  controlsVisible$: Observable<boolean> = this.controlsVisibleSubject.asObservable();
  // Visibility of the full screen media list overlay: follows the controls,
  // except while a keep-alive source outside the list (e.g. the script
  // status modal) is holding the controls visible - then the overlay
  // auto-hides on its own (see updateMediaListOverlayVisible).
  private mediaListOverlayVisibleSubject = new BehaviorSubject<boolean>(false);
  mediaListOverlayVisible$: Observable<boolean> = this.mediaListOverlayVisibleSubject.asObservable();
  private videoVisibleSubject = new BehaviorSubject<boolean>(false);
  videoVisible$: Observable<boolean> = this.videoVisibleSubject.asObservable();

  private playingMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
  playingMediaItem$: Observable<MediaItem | null> = this.playingMediaItemSubject.asObservable();

  private playingMediaItemPositionSubject = new BehaviorSubject<number | null>(null);
  playingMediaItemPosition$: Observable<number | null> = this.playingMediaItemPositionSubject.asObservable();

  private controlsHideDebounceSubject = new Subject<void>();
  // controlsVisibleDebounce$: Observable<boolean> = this.controlsVisibleDebounceSubject.asObservable();

  private videoNode: HTMLVideoElement | null = null;
  // Number of active "keep controls visible" sources (script status modal,
  // media list interaction, ...). The controls auto-hide only when the last
  // source releases, so overlapping sources can't cancel each other out.
  private keepAliveCount = 0;
  // Whether the media list wants the controls kept alive (it ORs its hover /
  // touch / focus event streams into a single call - see setMediaListActive).
  private mediaListActive = false;
  // iOS Safari has no element full screen API - full screen is faked with
  // CSS (see enterFakeFullScreen) and tracked with this flag.
  private fakeFullScreen = false;
  private externalStreaming = false;
  private isMuted = false;

  constructor(
    private http: HttpClient,
    private localSettingsService: LocalSettingsService,
    private websocketService: WebsocketService
  ) {
    this.controlsHideDebounceSubject.pipe(
      debounceTime(3000))
      .subscribe(() => {
        // While any keep-alive source is active (script status modal, media
        // list hover/touch/focus) the controls (and with them the full screen
        // media list overlay) stay visible; the countdown restarts when the
        // last source releases.
        if (this.keepAliveCount === 0)
          this.controlsVisibleSubject.next(false);
        this.updateMediaListOverlayVisible();
      });
    // Time-sync: send at most once per second. This is driven by the
    // timeupdate event (via onPositionChange -> playingMediaItemPosition$),
    // matching the old project's onVideoTimeUpdate behaviour.
    this.playingMediaItemPosition$.pipe(
      throttleTime(1000, undefined, { leading: true, trailing: true })
    ).subscribe(v => {
      this.sendMediaState();
    });
    this.localSettingsService.externalStreaming$.subscribe(v => {
      this.externalStreaming = v;
    });
    this.localSettingsService.mute$.subscribe(v => {
      this.isMuted = v;
    });
    // Keep isFullScreen$ in sync with the browser at all times. The
    // component-level fullscreenchange handler only fires while the video
    // player's template is listening; a document-level listener catches
    // every transition (including ones the UI didn't initiate) so the media
    // list overlay state can never get stuck out of sync.
    document.addEventListener('fullscreenchange', () => {
      if (!this.fakeFullScreen) {
        this.isFullScreenSubject.next(!!document.fullscreenElement);
      }
    });
  }

  postMediaState(state: XTPWebMediaState): Observable<any> {
    return this.http.post<any>(`/xtpweb`, state);
  }

  setVideoNode(node: HTMLVideoElement | null): void {
    this.videoNode = node;
  }

  get video(): HTMLVideoElement | null {
    return this.videoNode;
  }

  get playingMediaItem(): MediaItem | null {
    return this.playingMediaItemSubject.value;
  }

  set playingMediaItem(value: MediaItem | null)  {
    if(value == null)
      return;
    if(this.isItemPlaying(value))
      this.playingMediaItemSubject.next(value);
  }

  // set playingMediaItem(item: MediaItem | null) {
  //     this.playingMediaItemSubject.next(item);
  // }

  get currentPosition(): number {
    return this.playingMediaItemPositionSubject.value || 0;
  }

  get volume(): number {
    return this.volumeSubject.value;
  }

  get isPlaying(): boolean {
    return this.isPlayingSubject.value;
  }

  get isPaused(): boolean {
    return this.isPausedSubject.value;
  }

  get isFullScreen(): boolean {
    return this.isFullScreenSubject.value;
  }

  get controlsVisible(): boolean {
    return this.controlsVisibleSubject.value;
  }

  get videoVisible(): boolean {
    return this.videoVisibleSubject.value;
  }
  private set videoVisible(v: boolean) {
    this.videoVisibleSubject.next(v);
  }

  playVideo(item: MediaItem): void {
    if (this.playingMediaItem?.id === item.id) {
      return;
    }

    // if (this.playingMediaItem) {
    //   this.clearVideo();
    // }

    if (item.type === MediaType.ExternalType) {
      console.warn('Cannot play external media items from this interface.');
      return;
    }

    this.playingMediaItemSubject.next(item);

    // Handle subtitles
    if (item.subtitle) {
      const video = this.video;
      if (video) {
        const tracks = video.querySelectorAll('track');
        let track: HTMLTrackElement;
        if (!tracks.length) {
          track = document.createElement('track');
          track.default = true;
          video.appendChild(track);
        } else {
          track = tracks[0] as HTMLTrackElement;
        }
        if (item.subtitle.endsWith('vtt')) {
          track.src = '/media' + item.subtitleRelative;
        }
      }
    }

    // External streaming
    if (this.externalStreaming) {
      const filePath = '/media' + item.relativePath;
      window.open(filePath);
      return;
    }

    // Setup video source
    this.setVideoSource('/media' + item.relativePath);
    // this.setVideoTitle(item.name);
    this.setPoster('/thumb/' + item.relativeThumb);
    this.loadVideo();
    this.showVideo();

    // A new media item has started: reset the paused flag so the reported
    // state is "playing", then notify the backend immediately. (The
    // per-second updates are driven by the timeupdate event.)
    this.isPausedSubject.next(false);
    this.sendMediaState();
  }

  clearVideo(): void {
    this.websocketService.sendStopMedia();
    this.pause();
    this.removeVideoSource();
    // this.clearTitle();
    this.clearPoster();
    this.loadVideo();

    const playing = this.playingMediaItem;
    if (playing) {
      playing.playing = false;
      (playing as any).stopped = true;
      this.sendMediaState();
      this.playingMediaItemSubject.next(null);
    }
  }
  stopVideo(): void {
    this.clearVideo();
    this.hideControls();
    this.exitFullscreen();
    this.hideVideo();
  }

  onSetVolume(value: number): void {
    this.localSettingsService.setVolume(value);
  }

  volumeUp(): void {
    const current = this.volume;
    const next = Math.min(current + 0.05, 1.0);
    this.onSetVolume(next);
  }

  volumeDown(): void {
    const current = this.volume;
    const next = Math.max(current - 0.05, 0.0);
    this.onSetVolume(next);
  }

  toggleMute(): void {
    this.setMute(!this.isMuted);
  }

  setMute(isMuted: boolean) {
    this.localSettingsService.setMute(isMuted);
  }

  togglePause(): void {
    if (this.videoNode && this.isMediaItemPlaying()) {
      if (this.videoNode.paused || this.videoNode.ended) {
        this.unpause();
      } else {
        this.pause();
      }
    }
  }

  setPlayRate(value: number): void {
    if (this.videoNode) {
      this.videoNode.playbackRate = value;
    }
    this.websocketService.sendPlaybackRate(value);
  }

  resetPlayRate(): void {
    if (this.videoNode) {
      this.videoNode.playbackRate = 1;
    }
  }

  toggleFullScreen(): void {
    const videoContainer = document.getElementById('video-container');
    if (!videoContainer) return;
    if (document.fullscreenElement) {
      this.exitFullscreen(videoContainer);
    } else if (this.fakeFullScreen) {
      this.exitFakeFullScreen(videoContainer);
    } else if (videoContainer.requestFullscreen) {
      videoContainer.requestFullscreen().then(() => {
        // Trust the browser state over the promise - on some mobile browsers
        // the promise resolves but the element is not (yet) reported as the
        // fullscreen element, which left the media list overlay hidden.
        this.isFullScreenSubject.next(!!document.fullscreenElement);
        // Show the controls (and with them the media list overlay) right
        // after entering full screen - on touch devices nothing else resets
        // the auto-hide countdown, so without this the overlay would slide
        // away 3s later and appear to never have shown.
        this.showControls();
      }, (err) => {
        console.warn('[VideoService] requestFullscreen failed:', err);
        this.isFullScreenSubject.next(false);
      });
    } else {
      // iOS Safari has no element full screen API - fake it with CSS
      this.enterFakeFullScreen(videoContainer);
    }
  }

  exitFullscreen(videoContainer?: HTMLElement | null): void {
    if(!this.isFullScreenSubject.value)
      return;
    if (!videoContainer)
      videoContainer = document.getElementById('video-container');
    if (!videoContainer) return;
    if (document.fullscreenElement) {
      document.exitFullscreen().then(() => {
        this.isFullScreenSubject.next(false);
      });
    } else if (this.fakeFullScreen) {
      this.exitFakeFullScreen(videoContainer);
    }
  }

  private enterFakeFullScreen(videoContainer: HTMLElement): void {
    videoContainer.classList.add('video-container-fake-fullscreen');
    this.fakeFullScreen = true;
    this.isFullScreenSubject.next(true);
    this.showControls();
  }

  private exitFakeFullScreen(videoContainer: HTMLElement): void {
    videoContainer.classList.remove('video-container-fake-fullscreen');
    this.fakeFullScreen = false;
    this.isFullScreenSubject.next(false);
  }

  /** One keep-alive source starts wanting the controls visible. */
  assertControlsAlive(): void {
    this.keepAliveCount++;
    this.showControls();
  }

  /**
   * One keep-alive source stops wanting the controls visible. The controls
   * only (start to) hide when the last source releases.
   */
  releaseControlsKeepAlive(): void {
    this.keepAliveCount = Math.max(0, this.keepAliveCount - 1);
    if (this.keepAliveCount === 0) {
      this.hideControlsDebounce();
    }
    this.updateMediaListOverlayVisible();
  }

  showControls(forceVisible = false): void {
    if (forceVisible) {
      this.assertControlsAlive();
    }
    if (!this.controlsVisibleSubject.value) {
      this.controlsVisibleSubject.next(true);
    }
    this.controlsHideDebounceSubject.next();
    this.updateMediaListOverlayVisible();
  }

  hideControls(forceHide = false): void {
    if (forceHide) {
      this.releaseControlsKeepAlive();
    }
    if (this.keepAliveCount > 0) return;
    this.controlsVisibleSubject.next(false);
    this.updateMediaListOverlayVisible();
  }

  hideControlsDebounce(forceHide = false): void {
    if (forceHide) {
      this.releaseControlsKeepAlive();
    }
    if (this.keepAliveCount > 0) return;
    this.controlsHideDebounceSubject.next();
  }

  /**
   * Media list keep-alive (full screen overlay): the component reports
   * whether any of its hover / touch / focus event streams is active - mouse
   * over the list, a finger on it, or focus in one of its filter inputs
   * (typing). While active the controls (and with them the overlay) stay
   * visible; when it releases the auto-hide countdown starts.
   */
  setMediaListActive(active: boolean): void {
    if (this.mediaListActive === active) {
      return;
    }
    this.mediaListActive = active;
    if (active) {
      this.assertControlsAlive();
    } else {
      this.releaseControlsKeepAlive();
    }
    this.updateMediaListOverlayVisible();
  }

  /**
   * The full screen media list overlay follows the controls, except while a
   * keep-alive source outside the list (e.g. the script status modal) is
   * holding the controls visible - then the overlay auto-hides on its own
   * so it doesn't sit over the video while the user is in the modal.
   */
  private updateMediaListOverlayVisible(): void {
    const visible = this.controlsVisibleSubject.value
        && (this.mediaListActive || this.keepAliveCount === 0);
    if (this.mediaListOverlayVisibleSubject.value !== visible) {
      this.mediaListOverlayVisibleSubject.next(visible);
    }
  }

  disableForceControlsVisible(): void {
    this.releaseControlsKeepAlive();
  }

  onPlay(): void {
    this.isPlayingSubject.next(true);
    this.isPausedSubject.next(false);
    this.sendMediaState();
  }

  onPause(): void {
    this.isPausedSubject.next(true);
    this.sendMediaState();
  }

  onVolumeChange(value: number): void {
    this.onSetVolume(value);
  }

  // toggleMute(isMuted?: boolean): void {
  //   if (this.videoNode) {
  //     this.videoNode.muted = isMuted || !this.videoNode.muted;
  //     let volume = 0;
  //     if (this.videoNode.muted) {
  //       this.videoNode.dataset['volume'] = String(this.videoNode.volume);
  //     } else {
  //       volume = parseFloat(this.videoNode.dataset['volume'] || '0.5');
  //     }
  //   }
  // }

  // onToggleMute(isMuted: boolean): void {
  //   // if (this.videoNode) {
  //   //   this.toggleMute(isMuted);
  //     this.localSettingsService.setMute(isMuted);
  //   // }
  // }

  onFullScreenChange(): void {
    const isFs = !!document.fullscreenElement || this.fakeFullScreen;
    this.isFullScreenSubject.next(isFs);
  }

  setVideoSource(url: string): void {
    this.removeVideoSource();
    if (this.videoNode) {
      const source = document.createElement('source');
      source.id = 'videoSourceNode';
      source.src = url;
      this.videoNode.appendChild(source);
    }
  }

  removeVideoSource(): void {
    if (this.videoNode) {
      const source = this.videoNode.querySelector('source');
      if (source) {
        this.videoNode.removeChild(source);
      }
    }
  }

  setVideoTitle(title: string): void {
    if (this.videoNode) {
      this.videoNode.setAttribute('title', title);
    }
  }

  clearTitle(): void {
    if (this.videoNode) {
      this.videoNode.removeAttribute('title');
    }
  }

  setPoster(poster: string): void {
    if (this.videoNode) {
      this.videoNode.setAttribute('poster', poster);
    }
  }

  clearPoster(): void {
    if (this.videoNode) {
      this.videoNode.removeAttribute('poster');
    }
  }

  loadVideo(): void {
    if (this.videoNode) {
      this.videoNode.load();
    }
  }

  isItemPlaying(item: MediaItem): boolean {
    return this.playingMediaItemSubject.value?.id == item.id;
  }

  isMediaItemPlaying(): boolean {
    return this.playingMediaItemSubject.value != null;
  }

  unpause(): void {
    if (this.videoNode && this.isMediaItemPlaying()) {
      this.videoNode.play();
      this.isPausedSubject.next(false);
    }
  }

  pause(): void {
    if (this.videoNode && this.isMediaItemPlaying()) {
      this.videoNode.pause();
      this.isPausedSubject.next(true);
    }
  }

  hideVideo(): void {
    this.exitFullscreen()
    this.videoVisible = false;
  }

  showVideo(): void {
    this.videoVisible = true;
  }

  // skipTo(timeInSeconds: number): void {
  //   if (this.videoNode) {
  //     this.videoNode.currentTime = timeInSeconds;
  //     this.playingMediaItemPositionSubject.next(this.videoNode.currentTime);
  //   }
  // }

  onPositionChange(timeInSeconds: number) {
    if (this.isMediaItemPlaying() && this.video && this.video.duration > 0) {
      this.playingMediaItemPositionSubject.next(timeInSeconds);
    }
  }

  onSeek(timeInSeconds: number) {
    if (this.isMediaItemPlaying() && this.video && this.video.duration > 0) {
      this.video.currentTime = timeInSeconds;
      // this.video.currentTime = (timeInSeconds / 100) * this.video.duration;
      this.onPositionChange(this.video.currentTime);
    }
  }

  sendMediaState(): void {
    const playingmediaItem = this.playingMediaItem;
    // if (playing) {
    //   this.websocketService.send('mediaState', {
    //     itemID: playing.id,
    //     playing: playing.playing || false,
    //     time: this.video?.currentTime || 0
    //   });
    // }
    
		if (this.videoNode && playingmediaItem) {
			// debug("[sendMediaState] XTPWeb playingmediaItem: time:"+ videoNode.currentTime);
			this.postMediaState({
				"path": playingmediaItem.path,
				"playing": !this.isPaused,
				"currentTime": this.videoNode.currentTime,
				"duration": this.videoNode.duration,
				"playbackSpeed": this.videoNode.playbackRate
			}).subscribe();
		} else {
			// debug("[sendMediaState] XTPWeb playingmediaItem: null: time: 0");
			this.postMediaState({
				"path": undefined,
				"playing": false,
				"currentTime": 0,
				"duration": 0,
				"playbackSpeed": 1.0
			}).subscribe();
		}
  }
}
