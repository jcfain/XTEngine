import { Injectable } from '@angular/core';
import { VideoService } from './video.service';

/**
 * Keeps the CDK overlay container inside the full screen element.
 *
 * All modals/dialogs (media item properties, alt scripts, alerts, select
 * dialogs, ...) are rendered by the CDK into a single .cdk-overlay-container
 * that it appends to <body>. A full screen element only renders its own
 * subtree, so while the video is full screen the container is re-parented
 * into the video container (making the modals render on top of the video)
 * and moved back to <body> when full screen exits.
 *
 * Overlay panes are position: fixed, and the full screen element both fills
 * the viewport and becomes the containing block for fixed descendants, so
 * the panes' viewport coordinates stay correct after the move.
 */
@Injectable({ providedIn: 'root' })
export class FullscreenOverlayContainerService {
  private bodyObserver: MutationObserver | null = null;

  constructor(private videoService: VideoService) {
    this.videoService.isFullScreen$.subscribe(isFullScreen => {
      if (isFullScreen) {
        this.enterFullScreen();
      } else {
        this.exitFullScreen();
      }
    });
  }

  private get overlayContainer(): HTMLElement | null {
    return document.querySelector('.cdk-overlay-container');
  }

  private get videoContainer(): HTMLElement | null {
    return document.getElementById('video-container');
  }

  private enterFullScreen(): void {
    const container = this.overlayContainer;
    const host = this.videoContainer;
    if (container && host && container.parentElement !== host) {
      host.appendChild(container);
    }
    // The CDK creates the container lazily on first overlay use - watch
    // <body> and move it in if it appears while we are already full screen.
    if (!this.bodyObserver) {
      this.bodyObserver = new MutationObserver(() => {
        if (!this.videoService.isFullScreen) {
          return;
        }
        const c = this.overlayContainer;
        const h = this.videoContainer;
        if (c && h && c.parentElement === document.body) {
          h.appendChild(c);
        }
      });
      this.bodyObserver.observe(document.body, { childList: true });
    }
  }

  private exitFullScreen(): void {
    const container = this.overlayContainer;
    if (container && container.parentElement !== document.body) {
      document.body.appendChild(container);
    }
  }
}