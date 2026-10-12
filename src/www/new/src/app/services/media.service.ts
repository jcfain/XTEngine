import { Injectable } from '@angular/core';
import { Observable, of } from 'rxjs';
import { HttpClient } from '@angular/common/http';
import { MediaItem } from '../models/interfaces';
import { MediaType } from '../models/enums';
import { AppStateService } from './app-state.service';
import { WebsocketService } from './websocket.service';
import { VideoService } from './video.service';
import { MediaListService } from './media-list-service';

@Injectable({
  providedIn: 'root'
})
export class MediaService {
  constructor(
    private http: HttpClient,
    private appState: AppStateService,
    private websocketService: WebsocketService,
    private mediaListService: MediaListService,
    private videoService: VideoService
  ) {}

  getMediaList(): Observable<any> {
    return this.http.get<any>('/media');
  }

  loadMediaList(): void {
    this.http.get<any>('/media').subscribe({
      next: (response) => {
        const items: MediaItem[] = Array.isArray(response) ? response : (response.items || []);
        this.mediaListService.setMediaList(items);
      },
      error: (err) => {
        console.error('Error loading media list:', err);
      }
    });
  }

  skipToMoneyShot(): void {
    const playing = this.mediaListService.playingMediaItem;
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

  saveMetaData(item: MediaItem): void {
    this.websocketService.send('saveMetaData', { itemID: item.id, metaData: (item as any).metaData });
  }

  toggleShufflePlay(): void {
    this.appState.setShufflePlayMode(!this.appState.shufflePlayMode);
  }
}