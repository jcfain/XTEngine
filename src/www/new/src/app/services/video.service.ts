import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { WebsocketService } from './websocket.service';

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

  private videoNode: HTMLVideoElement | null = null;
  private controlsHideDebounce: any;
  private forceControlsVisible = false;

  constructor(private websocketService: WebsocketService) {
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
    if (this.videoNode) {
      this.videoNode.classList.remove('video-shown', 'video-shown-embeded');
    }
  }

  showVideo(): void {
    if (this.videoNode) {
      this.videoNode.classList.add('video-shown', 'video-shown-embeded');
    }
  }

  skipTo(timeInSeconds: number): void {
    if (this.videoNode) {
      this.videoNode.currentTime = timeInSeconds;
    }
  }
}
