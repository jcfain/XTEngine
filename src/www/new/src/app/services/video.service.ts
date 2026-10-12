import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { WebsocketService } from './websocket.service';
import { AppStateService } from './app-state.service';
import { MediaType } from '../models/enums';
import { MediaItem } from '../models/interfaces';
import { MediaListService } from './media-list-service';

@Injectable({
  providedIn: 'root'
})
export class VideoService {
  private volumeSubject = new BehaviorSubject<number>(0.5);
  volume$: Observable<number> = this.volumeSubject.asObservable();

  private isPlayingSubject = new BehaviorSubject<boolean>(false);
  isPlaying$: Observable<boolean> = this.isPlayingSubject.asObservable();

  private isFullScreenSubject = new BehaviorSubject<boolean>(false);
  isFullScreen$: Observable<boolean> = this.isFullScreenSubject.asObservable();

  private controlsVisibleSubject = new BehaviorSubject<boolean>(false);
  controlsVisible$: Observable<boolean> = this.controlsVisibleSubject.asObservable();

  private videoVisibleSubject = new BehaviorSubject<boolean>(false);
  videoVisible$: Observable<boolean> = this.videoVisibleSubject.asObservable();

  private playingMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
  playingMediaItem$: Observable<MediaItem | null> = this.playingMediaItemSubject.asObservable();

  private videoNode: HTMLVideoElement | null = null;
  private controlsHideDebounce: any;
  private forceControlsVisible = false;

  constructor(
    private appState: AppStateService,
    private websocketService: WebsocketService
  ) {
    const storedVolume = window.localStorage.getItem('volume');
    if (storedVolume) {
      this.volumeSubject.next(parseFloat(storedVolume));
    }
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

  set playingMediaItem(item: MediaItem | null) {
      this.playingMediaItemSubject.next(item);
  }


  get volume(): number {
    return this.volumeSubject.value;
  }

  get isPlaying(): boolean {
    return this.isPlayingSubject.value;
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

    if (this.playingMediaItem) {
      this.stopVideo();
    }

    if (item.type === MediaType.ExternalType) {
      console.warn('Cannot play external media items from this interface.');
      return;
    }

    this.playingMediaItem = item;

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
    if (this.appState.externalStreaming) {
      const filePath = '/media' + item.relativePath;
      window.open(filePath);
      return;
    }

    // Setup video source
    this.setVideoSource('/media' + item.relativePath);
    this.setVideoTitle(item.name);
    this.setPoster('/thumb/' + item.relativeThumb);
    this.loadVideo();
    this.showVideo();
  }

  stopVideo(): void {
    this.websocketService.sendStopMedia();
    this.hideVideo();
    this.pause();
    this.removeVideoSource();
    this.clearTitle();
    this.clearPoster();
    this.loadVideo();

    const playing = this.playingMediaItem;
    if (playing) {
      playing.playing = false;
      (playing as any).stopped = true;
      this.sendMediaState();
      this.playingMediaItem = null;
    }
  }

  setVolume(value: number): void {
    this.volumeSubject.next(value);
    if (this.videoNode) {
      this.videoNode.volume = value;
    }
    window.localStorage.setItem('volume', String(value));
  }

  volumeUp(): void {
    const current = this.volume;
    const next = Math.min(current + 0.05, 1.0);
    this.setVolume(next);
  }

  volumeDown(): void {
    const current = this.volume;
    const next = Math.max(current - 0.05, 0.0);
    this.setVolume(next);
  }

  toggleMute(): void {
    if (this.videoNode) {
      this.videoNode.muted = !this.videoNode.muted;
      if (this.videoNode.muted) {
        this.videoNode.dataset['volume'] = String(this.videoNode.volume);
        this.videoNode.volume = 0;
      } else {
        this.videoNode.volume = parseFloat(this.videoNode.dataset['volume'] || '0.5');
      }
    }
  }

  togglePlay(): void {
    if (this.videoNode) {
      if (this.videoNode.paused || this.videoNode.ended) {
        this.videoNode.play();
      } else {
        this.videoNode.pause();
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
      document.exitFullscreen();
    } else {
      videoContainer.requestFullscreen();
    }
  }

  showControls(force = false): void {
    this.forceControlsVisible = force;
    this.controlsVisibleSubject.next(true);
    if (this.controlsHideDebounce) {
      clearTimeout(this.controlsHideDebounce);
    }
    if (!force) {
      this.hideControlsAfterTimeout();
    }
  }

  hideControls(force = false): void {
    if (force) {
      this.forceControlsVisible = false;
    }
    if (this.forceControlsVisible) return;
    this.controlsVisibleSubject.next(false);
  }

  hideControlsAfterTimeout(): void {
    this.controlsHideDebounce = setTimeout(() => {
      this.hideControlsEvent();
    }, 3000);
  }

  hideControlsEvent(): void {
    if (this.forceControlsVisible || this.videoNode?.paused) {
      return;
    }
    this.hideControls();
  }

  disableForceControlsVisible(): void {
    this.forceControlsVisible = false;
  }

  onPlay(): void {
    this.isPlayingSubject.next(true);
  }

  onPause(): void {
    this.isPlayingSubject.next(false);
  }

  onVolumeChange(): void {
    if (this.videoNode) {
      this.volumeSubject.next(this.videoNode.volume);
      window.localStorage.setItem('volume', String(this.videoNode.volume));
    }
  }

  onFullScreenChange(): void {
    const isFs = !!document.fullscreenElement;
    this.isFullScreenSubject.next(isFs);
  }

  formatTime(timeInSeconds: number): { hours: string; minutes: string; seconds: string } {
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

  pause(): void {
    if (this.videoNode) {
      this.videoNode.pause();
    }
  }

  hideVideo(): void {
    this.videoVisible = false;
  }

  showVideo(): void {
    this.videoVisible = true;
  }

  skipTo(timeInSeconds: number): void {
    if (this.videoNode) {
      this.videoNode.currentTime = timeInSeconds;
    }
  }

  sendMediaState(): void {
    const playing = this.playingMediaItem;
    if (playing) {
      this.websocketService.send('mediaState', {
        itemID: playing.id,
        playing: playing.playing || false,
        time: this.video?.currentTime || 0
      });
    }
  }
}
