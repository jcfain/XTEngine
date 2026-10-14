import { Component, OnInit, OnDestroy, ViewChild, ElementRef, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Subscription } from 'rxjs';
import { VideoService } from '../../services/video.service';
import { MediaService } from '../../services/media.service';
import { MediaItem } from '../../models/interfaces';
import { MediaListService } from '../../services/media-list-service';
import { WebsocketService } from '../../services/websocket.service';
import { MessageHandlerService } from '../../services/message-handler.service';

@Component({
  selector: 'app-video-player',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './video-player.component.html',
  styleUrl: './video-player.component.css'
})
export class VideoPlayerComponent implements OnInit, OnDestroy, AfterViewInit {
  @ViewChild('videoNode', { static: false }) videoNodeRef!: ElementRef<HTMLVideoElement>;

  isPlaying = false;
  isFullScreen = false;
  controlsVisible = false;
  videoVisible = false;
  videoVisibleEmbedded = true;
  volume = 0.5;
  currentTime = '00:00:00';
  duration = '00:00:00';
  progress = 0;
  loading = false;
  mediaItem: MediaItem | null = null;

  private subscriptions: Subscription[] = [];

  constructor(
    private webSocketService: WebsocketService,
    private messageHandlerService: MessageHandlerService, 
    private videoService: VideoService,
    private mediaListService: MediaListService,
    private mediaService: MediaService
  ) {
    this.messageHandlerService.stopAllMedia$.subscribe(() => {this.stop()} )
  }

  ngOnInit(): void {
    this.subscriptions.push(
      this.videoService.isPlaying$.subscribe(v => this.isPlaying = v),
      this.videoService.isFullScreen$.subscribe(v => this.isFullScreen = v),
      this.videoService.controlsVisible$.subscribe(v => this.controlsVisible = v),
      this.videoService.volume$.subscribe(v => this.volume = v),
      this.videoService.videoVisible$.subscribe(v => this.videoVisible = v),
      this.videoService.playingMediaItem$.subscribe(v => this.mediaItem = v)
    );
  }

  ngAfterViewInit(): void {
    this.videoService.setVideoNode(this.videoNodeRef.nativeElement);
    const video = this.videoNodeRef.nativeElement;

    video.addEventListener('play', () => this.videoService.onPlay());
    video.addEventListener('pause', () => this.videoService.onPause());
    video.addEventListener('volumechange', () => this.videoService.onVolumeChange());
    video.addEventListener('loadedmetadata', () => this.onLoadedMetadata());
    video.addEventListener('timeupdate', () => this.onTimeUpdate());
    video.addEventListener('ended', () => this.onVideoEnd());
    video.addEventListener('waiting', () => this.loading = true);
    video.addEventListener('playing', () => this.loading = false);
    video.addEventListener('click', () => this.videoService.togglePlay());

    document.addEventListener('fullscreenchange', () => this.videoService.onFullScreenChange());
  }

  ngOnDestroy(): void {
    this.subscriptions.forEach(s => s.unsubscribe());
    this.videoService.setVideoNode(null);
    document.removeEventListener('fullscreenchange', () => this.videoService.onFullScreenChange());
  }

  onPlayNext(): void {
    this.mediaListService.playNextVideo();
  }

  onPlayPrevious(): void {
    this.mediaListService.playPreviousVideo();
  }

  onStop(): void {
    this.videoService.stopVideo();
  }

  onSkipToMoneyShot(): void {
    this.mediaService.skipToMoneyShot();
  }

  onSkipToNextAction(): void {
    this.mediaService.skipToNextAction();
  }

  togglePauseAllDeviceActions() {
    this.webSocketService.togglePauseAllDeviceActions();
  }

  onLoadedMetadata(): void {
    const video = this.videoNodeRef.nativeElement;
    const time = this.videoService.formatTime(Math.round(video.duration));
    this.duration = `${time.hours}:${time.minutes}:${time.seconds}`;
  }

  onTimeUpdate(): void {
    const video = this.videoNodeRef.nativeElement;
    const time = this.videoService.formatTime(Math.round(video.currentTime));
    this.currentTime = `${time.hours}:${time.minutes}:${time.seconds}`;
    this.progress = video.duration > 0 ? (video.currentTime / video.duration) * 100 : 0;
  }

  onVideoEnd(): void {
    // Hook for playNextVideo
  }

  togglePlay(): void {
    this.videoService.togglePlay();
  }

  stop() {
    this.videoService.stopVideo();
  }

  toggleFullScreen(): void {
    this.videoService.toggleFullScreen();
  }

  toggleMute(): void {
    this.videoService.toggleMute();
  }

  // volumeUp(): void {
  //   this.videoService.volumeUp();
  // }

  // volumeDown(): void {
  //   this.videoService.volumeDown();
  // }

  setVolume(value: string): void {
    const valueNum = parseFloat(value);
    this.videoService.setVolume(valueNum);
  }

  setPlayRate(value: string): void {
    const valueNum = parseFloat(value);
    this.videoService.setPlayRate(valueNum);
  }

  onSeek(event: Event): void {
    const value = parseFloat((event.target as HTMLInputElement).value);
    const video = this.videoNodeRef.nativeElement;
    if (video.duration > 0) {
      video.currentTime = (value / 100) * video.duration;
    }
  }

  showControls(): void {
    this.videoService.showControls();
  }

  hideControls(): void {
    this.videoService.hideControlsEvent();
  }

  isVideoFixedLowHeight(): boolean {
    return document.documentElement.clientHeight < 600 && !this.isFullScreen && this.videoVisible;
  }
}