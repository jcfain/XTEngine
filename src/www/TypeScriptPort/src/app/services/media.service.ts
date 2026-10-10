import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';
import { HttpClient } from '@angular/common/http';
import { MediaItem } from '../models/interfaces';
import { MediaType } from '../models/enums';
import { AppStateService } from './app-state.service';
import { WebsocketService } from './websocket.service';
import { VideoService } from './video.service';

@Injectable({
  providedIn: 'root'
})
export class MediaService {
  constructor(
    private http: HttpClient,
    private appState: AppStateService,
    private websocketService: WebsocketService,
    private videoService: VideoService
  ) {}

  getMediaList(): Observable<any> {
    return this.http.get<any>('/mediaList');
  }

  loadMediaList(): void {
    this.http.get<any>('/mediaList').subscribe({
      next: (response) => {
        const items: MediaItem[] = response.items || [];
        this.appState.setMediaList(items);
      },
      error: (err) => {
        console.error('Error loading media list:', err);
      }
    });
  }

  playVideo(item: MediaItem): void {
    if (this.appState.playingMediaItem?.id === item.id) {
      return;
    }

    if (this.appState.playingMediaItem) {
      this.stopVideo();
    }

    if (item.type === MediaType.ExternalType) {
      console.warn('Cannot play external media items from this interface.');
      return;
    }

    this.appState.setPlayingMediaItem(item);

    // Handle subtitles
    if (item.subtitle) {
      const video = this.videoService.video;
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
    this.videoService.setVideoSource('/media' + item.relativePath);
    this.videoService.setVideoTitle(item.name);
    this.videoService.setPoster('/thumb/' + item.relativeThumb);
    this.videoService.loadVideo();
  }

  stopVideo(): void {
    this.websocketService.sendStopMedia();
    this.videoService.hideVideo();
    this.videoService.pause();
    this.videoService.removeVideoSource();
    this.videoService.clearTitle();
    this.videoService.clearPoster();
    this.videoService.loadVideo();

    const playing = this.appState.playingMediaItem;
    if (playing) {
      playing.playing = false;
      (playing as any).stopped = true;
      this.sendMediaState();
      this.appState.setPlayingMediaItem(null);
    }
  }

  playNextVideo(): void {
    const list = this.appState.mediaList;
    const playing = this.appState.playingMediaItem;
    if (!playing || list.length === 0) return;

    const index = list.findIndex(m => m.id === playing.id);
    if (index === -1) return;

    if (this.appState.shufflePlayMode) {
      const next = this.getNextShuffleMediaItem();
      if (next) this.playVideo(next);
    } else {
      const nextIndex = index + 1;
      if (nextIndex < list.length) {
        this.playVideo(list[nextIndex]);
      }
    }
  }

  playPreviousVideo(): void {
    const list = this.appState.mediaList;
    const playing = this.appState.playingMediaItem;
    if (!playing || list.length === 0) return;

    const index = list.findIndex(m => m.id === playing.id);
    if (index <= 0) return;

    this.playVideo(list[index - 1]);
  }

  private getNextShuffleMediaItem(): MediaItem | null {
    const list = this.appState.mediaList;
    if (list.length === 0) return null;
    const randomIndex = Math.floor(Math.random() * list.length);
    return list[randomIndex];
  }

  sendMediaState(): void {
    const playing = this.appState.playingMediaItem;
    if (playing) {
      this.websocketService.send('mediaState', {
        itemID: playing.id,
        playing: playing.playing || false,
        time: this.videoService.video?.currentTime || 0
      });
    }
  }

  skipToMoneyShot(): void {
    const playing = this.appState.playingMediaItem;
    if (playing) {
      const moneyShotSecs = (playing as any).metaData?.moneyShotSecs;
      if (moneyShotSecs && moneyShotSecs > 0) {
        this.websocketService.sendSkipToMoneyShot();
      }
    }
  }

  skipToNextAction(): void {
    this.websocketService.sendSkipToNextAction();
  }

  deleteMediaItem(item: MediaItem): void {
    this.websocketService.sendDeleteMediaItem({ id: item.id });
  }

  updateThumb(item: MediaItem): void {
    this.websocketService.sendUpdateThumb({ id: item.id });
  }

  updateThumbAtPos(item: MediaItem): void {
    const pos = this.videoService.video?.currentTime || 0;
    this.websocketService.send('saveSingleThumb', { itemID: item.id, pos });
  }

  setMoneyShotAtPos(item: MediaItem): void {
    const pos = this.videoService.video?.currentTime || 0;
    this.websocketService.send('setMoneyShot', { itemID: item.id, pos });
  }

  onMediaLoaded(): void {
    this.appState.setMediaLoading(false);
  }

  onMediaLoading(): void {
    this.appState.setMediaLoading(true);
  }

  onMediaLoadingStatus(status: string): void {
    console.log('Media loading status:', status);
  }

  onUpdateThumb(message: any): void {
    const item = this.appState.mediaList.find(m => m.id === message.id);
    if (item) {
      item.relativeThumb = '/thumb/' + message.thumb;
      this.appState.updateMediaItem(item);
    }
  }

  saveMetaData(item: MediaItem): void {
    this.websocketService.send('saveMetaData', { itemID: item.id, metaData: (item as any).metaData });
  }

  toggleShufflePlay(): void {
    this.appState.setShufflePlayMode(!this.appState.shufflePlayMode);
  }
}