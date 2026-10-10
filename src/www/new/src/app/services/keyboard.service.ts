import { Injectable, NgZone } from '@angular/core';
import { VideoService } from './video.service';
import { MediaService } from './media.service';

@Injectable({
  providedIn: 'root'
})
export class KeyboardService {
  constructor(
    private videoService: VideoService,
    private mediaService: MediaService,
    private ngZone: NgZone
  ) {
    document.addEventListener('keyup', (event: KeyboardEvent) => {
      this.ngZone.run(() => this.handleKey(event));
    });
  }

  private handleKey(event: KeyboardEvent): void {
    // Don't handle shortcuts when typing in an input
    const target = event.target as HTMLElement;
    if (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.tagName === 'SELECT') {
      return;
    }

    switch (event.key) {
      case 'k':
        this.videoService.togglePlay();
        if (this.videoService.video?.paused) {
          this.videoService.showControls();
        } else {
          this.videoService.hideControls();
        }
        break;
      case 'm':
        this.videoService.toggleMute();
        break;
      case 'f':
        this.videoService.toggleFullScreen();
        break;
      case 'ArrowRight':
        this.mediaService.playNextVideo();
        break;
      case 'ArrowLeft':
        this.mediaService.playPreviousVideo();
        break;
      case '+':
      case '=':
        this.videoService.volumeUp();
        break;
      case '-':
        this.videoService.volumeDown();
        break;
      case 'a':
        this.mediaService.skipToNextAction();
        break;
      case 'c':
        this.mediaService.skipToMoneyShot();
        break;
      case 'x':
        this.mediaService.stopVideo();
        break;
    }
  }
}