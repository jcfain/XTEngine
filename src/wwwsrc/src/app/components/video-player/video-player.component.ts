import { Component, OnInit, OnDestroy, ViewChild, ViewChildren, QueryList, ElementRef, AfterViewInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { Subscription } from 'rxjs';
import { VideoService } from '../../services/video.service';
import { MediaItem } from '../../models/interfaces';
import { MediaListService } from '../../services/media-list-service';
import { WebsocketService } from '../../services/websocket.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { MediaItemPropertiesComponent } from '../media-item-properties/media-item-properties.component';
import { MediaItemAltScriptsComponent } from '../media-item-alt-scripts/media-item-alt-scripts.component';
import { MediaListOverlayService } from '../../services/media-list-overlay.service';
import { ScriptStatusComponent } from '../script-status/script-status.component';

@Component({
    selector: 'app-video-player',
    imports: [FormsModule, MediaItemPropertiesComponent, MediaItemAltScriptsComponent, ScriptStatusComponent],
    templateUrl: './video-player.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './video-player.component.css'
})
export class VideoPlayerComponent implements OnInit, OnDestroy, AfterViewInit {
  @ViewChild('videoNode', { static: false }) videoNodeRef!: ElementRef<HTMLVideoElement>;
  @ViewChild('progressInput', { static: false }) progressBarNodeRef!: ElementRef<HTMLVideoElement>;
  @ViewChild('progressTooltip', { static: false }) seekToolTipNodeRef!: ElementRef<HTMLVideoElement>;
  // Host for the media list overlay (re-parented here while full screen)
  @ViewChild('fullscreenMediaListHost', { static: false }) fullscreenMediaListHost?: ElementRef<HTMLDivElement>;
  // The controls only exist while the video is visible (inside an @if
  // block), so track them with ViewChildren queries instead of plain
  // ViewChild.
  @ViewChildren('topControls') topControlsQuery?: QueryList<ElementRef<HTMLElement>>;
  @ViewChildren('bottomControls') bottomControlsQuery?: QueryList<ElementRef<HTMLElement>>;

  isPlaying = false;
  isPaused = false;
  isFullScreen = false;
  get controlsVisible(): boolean {
    return this.mediaItem != null && this.m_controlsVisible;
  };
  videoVisible = false;
  videoVisibleEmbedded = true;
  set volume(value: number) {
    this.localSettingsService.setVolume(value);
  }
  get volume(): number {
    return this.m_volume;
  }
  m_volume = 0.5;
  currentTime = '00:00:00';
  duration = '00:00:00';
  durationSecs = 0.0;
  progressTooltipText = "00:00:00";
  loading = false;
  mediaItem: MediaItem | null = null;
  muted: boolean = false;
  m_progress = 0;
  scriptStatusOpen = false;
  mediaSettingsOpen = false;
  altScriptsOpen = false;
  inputDeviceConnected = false;

  set progress(value: string) {
    this.m_progress = parseFloat(value);
    this.onSeek(value);
  }
  get progress(): string {
    return String(this.m_progress);
  }

  private subscriptions: Subscription[] = [];
  private m_controlsVisible = false;
  private deviceIsPaused = false;
  private bottomControlsState: { el: HTMLElement | null; observer: ResizeObserver | null } = { el: null, observer: null };
  private topControlsState: { el: HTMLElement | null; observer: ResizeObserver | null } = { el: null, observer: null };

  constructor(
    private localSettingsService: LocalSettingsService,
    private webSocketService: WebsocketService,
    private messageHandlerService: MessageHandlerService, 
    private videoService: VideoService,
    private mediaListService: MediaListService,
    private mediaListOverlayService: MediaListOverlayService
  ) {
    this.subscriptions.push(
      this.messageHandlerService.stopAllMedia$.subscribe(() => {
        this.stop()
      }),
        this.messageHandlerService.skipToNextAction$.subscribe(v => {
          if(v)
            this.skipToNextAction(v);
        }),
        this.messageHandlerService.skipToMoneyShot$.subscribe(v => {
            this.skipToMoneyShot();
        }),
        this.messageHandlerService.scriptTogglePaused$.subscribe(v => {
          this.deviceIsPaused = v;
        })
    );
  }

  ngOnInit(): void {
    this.subscriptions.push(
      this.videoService.isPlaying$.subscribe(v => { this.isPlaying = v; }),
      this.videoService.isPaused$.subscribe(v => { this.isPaused = v; }),
      this.videoService.isFullScreen$.subscribe(v => {
        this.isFullScreen = v;
        // The embedded (half-height) video is only used when NOT full screen.
        // Syncing from the service (instead of toggling on the
        // fullscreenchange event) also covers the CSS fake full screen mode
        // used on iOS, where no fullscreenchange event fires.
        this.videoVisibleEmbedded = !v;
      }),
      this.videoService.controlsVisible$.subscribe(v => this.m_controlsVisible = v),
      this.videoService.videoVisible$.subscribe(v => this.videoVisible = v),
      this.videoService.playingMediaItem$.subscribe(v => this.mediaItem = v),
      this.localSettingsService.volume$.subscribe(v => this.m_volume = v),
      this.localSettingsService.mute$.subscribe(v => this.muted = v)
    );
  }

  ngAfterViewInit(): void {
    this.videoService.setVideoNode(this.videoNodeRef.nativeElement);
    if (this.fullscreenMediaListHost) {
      this.mediaListOverlayService.registerFullscreenHost(this.fullscreenMediaListHost.nativeElement);
    }
    // The controls are created/destroyed with the video visibility, so watch
    // the queries for changes and observe whichever instance exists.
    this.bottomControlsQuery?.changes.subscribe(() =>
      this.trackControlsHeight(this.bottomControlsQuery, this.bottomControlsState, '--video-controls-height'));
    this.topControlsQuery?.changes.subscribe(() =>
      this.trackControlsHeight(this.topControlsQuery, this.topControlsState, '--video-top-controls-height'));
    this.trackControlsHeight(this.bottomControlsQuery, this.bottomControlsState, '--video-controls-height');
    this.trackControlsHeight(this.topControlsQuery, this.topControlsState, '--video-top-controls-height');
  }

  ngOnDestroy(): void {
    this.subscriptions.forEach(s => s.unsubscribe());
    this.videoService.setVideoNode(null);
    this.bottomControlsState.observer?.disconnect();
    this.bottomControlsState.observer = null;
    this.topControlsState.observer?.disconnect();
    this.topControlsState.observer = null;
  }

  // Exposes a controls bar height as a CSS variable on the video container.
  // The media list overlay sits between the top and bottom controls on touch
  // devices and uses the variables to offset itself by exactly the bar
  // heights, which vary with screen size as the control buttons wrap.
  private trackControlsHeight(
    query: QueryList<ElementRef<HTMLElement>> | undefined,
    state: { el: HTMLElement | null; observer: ResizeObserver | null },
    cssVar: string
  ): void {
    const el = query?.first?.nativeElement ?? null;
    if (el === state.el) {
      return;
    }
    state.observer?.disconnect();
    state.observer = null;
    state.el = el;
    if (el) {
      state.observer = new ResizeObserver(() => {
        const h = el.offsetHeight;
        if (h > 0) {
          document.getElementById('video-container')?.style.setProperty(cssVar, h + 'px');
        }
      });
      state.observer.observe(el);
    }
  }

  onFullScreenChange() {
    this.videoService.onFullScreenChange();
  }

  onPlayNext(): void {
    this.mediaListService.playNextVideo();
  }

  onPlayPrevious(): void {
    this.mediaListService.playPreviousVideo();
  }

  onPlay(): void {
    this.videoService.onPlay();
  }

  onPause(): void {
    this.videoService.onPause();
  }

  onStop(): void {
    this.videoService.stopVideo();
  }

  onWaiting() {
    this.loading = true;
  }

  onPlaying() {
    this.loading = false;
  }

  onSkipToMoneyShot(): void {
    //this.mediaService.skipToMoneyShot();
    this.webSocketService.sendSkipToMoneyShot();
  }

  onSkipToNextAction(): void {
    // this.mediaService.skipToNextAction();
    this.webSocketService.sendSkipToNextAction();
  }

  togglePauseAllDeviceActions() {
    this.webSocketService.togglePauseAllDeviceActions();
  }

  skipToMoneyShot(): void {
    const playing = this.videoService.playingMediaItem;
    if (playing) {
      const moneyShotSecs = (playing as any).metaData?.moneyShotSecs;
      if (moneyShotSecs && moneyShotSecs > 0) {
        this.onSeek(moneyShotSecs);
      }
    }
  }

  skipToNextAction(nextAction: number): void {
      this.videoService.onSeek(nextAction);
  }

  onLoadedMetadata(): void {
    const video = this.videoNodeRef.nativeElement;
    const videoDurationSecs = Math.round(video.duration);
    const time = this.formatTime(videoDurationSecs);
    this.duration = `${time.hours}:${time.minutes}:${time.seconds}`;
    this.durationSecs = videoDurationSecs;
  }

  onTimeUpdate(): void {
    const video = this.videoNodeRef.nativeElement;
    const time = this.formatTime(Math.round(video.currentTime));
    this.currentTime = `${time.hours}:${time.minutes}:${time.seconds}`;
    this.m_progress = video.currentTime;
    // this.m_progress = video.duration > 0 ? (video.currentTime / video.duration) * 100 : 0;
    this.videoService.onPositionChange(video.currentTime);
  }

  // updateSeekTooltip uses the position of the mouse on the progress bar to
  // roughly work out what point in the video the user will skip to if
  // the progress bar is clicked at that point
  updateSeekTooltip(event: MouseEvent) {
    const progressBar = this.progressBarNodeRef.nativeElement;
    const seekToolTip = this.seekToolTipNodeRef.nativeElement;
    const maxvalue = progressBar.getAttribute('max') || "0";
    const skipTo = Math.round(
      (event.offsetX / progressBar.clientWidth) *
      parseInt(maxvalue, 10)
    );
    seekToolTip.setAttribute('data-seek', String(skipTo));
    const t = this.formatTime(skipTo);
    this.progressTooltipText = `${t.hours}:${t.minutes}:${t.seconds}`;
    const rect = progressBar.getBoundingClientRect();
    seekToolTip.style.left = `${event.pageX - rect.left}px`;
  }

  onVideoEnd(): void {
    this.playNext();
  }

  togglePause(): void {
    this.videoService.togglePause();
  }

  // Clicking the video toggles pause, but only for real mouse clicks. On touch
  // devices a tap fires touchstart followed by a synthetic click, so we record
  // the touch time and ignore the click if a touch just happened.
  private lastVideoTouchTime = 0;

  // Tapping the video toggles the controls: show them if hidden, hide them if
  // shown (unless forceControlsVisible is set, in which case hideControls is a no-op).
  onVideoTouch(): void {
    this.lastVideoTouchTime = Date.now();
    if (this.controlsVisible) {
      this.videoService.hideControls();
    } else {
      this.videoService.showControls();
    }
  }

  onVideoClick(): void {
    if (Date.now() - this.lastVideoTouchTime < 500) {
      return;
    }
    this.togglePause();
  }

  stop() {
    if(this.scriptStatusOpen)
      this.toggleScriptStatus();
    this.videoService.stopVideo();
  }

  toggleFullScreen(): void {
    this.videoService.toggleFullScreen();
  }

  toggleMute(): void {
    this.localSettingsService.setMute(!this.muted);
  }

  playNext(): void {
    this.mediaListService.playNextVideo();
  }

  playPrevious(): void {
    this.mediaListService.playPreviousVideo();
  }

  // volumeUp(): void {
  //   this.videoService.volumeUp();
  // }

  // volumeDown(): void {
  //   this.videoService.volumeDown();
  // }

  setPlayRate(value: string): void {
    const valueNum = parseFloat(value);
    this.videoService.setPlayRate(valueNum);
    this.videoService.showControls();
  }

  onSeek(valueString: string): void {
    // const value = parseFloat((event.target as HTMLInputElement).value);
    const value = parseFloat(valueString);
    this.videoService.onSeek(value);
  }

  showControls(): void {
    // A mobile tap fires a synthetic mousemove right after touchstart, which would
    // immediately re-show the controls we just hid on the second tap. Ignore the
    // call if a touch just happened.
    if (Date.now() - this.lastVideoTouchTime < 500) {
      return;
    }
    if(this.videoVisible)
      this.videoService.showControls();
  }

  hideControls(): void {
    this.videoService.hideControls();
  }

  hideControlsDebounce(): void {
    this.videoService.hideControlsDebounce();
  }

  isVideoFixedLowHeight(): boolean {
    return document.documentElement.clientHeight < 600 && !this.isFullScreen && this.videoVisible;
  }

  openMediaSettings(): void {
    if (!this.mediaItem) {
      return;
    }
    this.mediaSettingsOpen = true;
  }

  closeMediaSettings(): void {
    this.mediaSettingsOpen = false;
  }

  openAltScripts(): void {
    if (!this.mediaItem) {
      return;
    }
    this.altScriptsOpen = true;
  }

  closeAltScripts(): void {
    this.altScriptsOpen = false;
  }

  // The script status panel is rendered in-flow inside the video container
  // (see .script-status-panel) - it just toggles its visibility here.
  toggleScriptStatus(): void {
    this.scriptStatusOpen = !this.scriptStatusOpen;
  }
  
  getScriptPausedStatusImage() {
    if(this.deviceIsPaused)
      return "://images/icons/play.svg";
    else
    return "://images/icons/pause.svg";
  }

  private formatTime(timeInSeconds: number): { hours: string; minutes: string; seconds: string } {
    if (!timeInSeconds) {
      return { hours: '00', minutes: '00', seconds: '00' };
    }
    const hours = Math.floor(timeInSeconds / 3600);
    const minutes = Math.floor((timeInSeconds % 3600) / 60);
    const seconds = Math.floor(timeInSeconds % 60);
    return {
      hours: hours < 10 ? '0' + hours : String(hours),
      minutes: minutes < 10 ? '0' + minutes : String(minutes),
      seconds: seconds < 10 ? '0' + seconds : String(seconds),
    };
  }
}