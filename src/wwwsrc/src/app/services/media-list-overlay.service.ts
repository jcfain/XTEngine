import { Injectable } from '@angular/core';
import { VideoService } from './video.service';
import { LocalSettingsService } from './local-settings.service';
import { BehaviorSubject, Observable } from 'rxjs';

/**
 * Moves the single media list element between its normal position in the app
 * body and an overlay position inside the video container while the video is
 * full screen. A full screen element only renders its own subtree, so the
 * element is physically re-parented with appendChild and moved back to its
 * original parent when full screen exits.
 *
 * Direct DOM re-parenting (instead of ViewContainerRef.insert) is required
 * because in Angular 22 the VCR API cannot move views created by templates:
 * ViewContainerRef.get() returns null for them (the container's VIEW_REFS
 * array is only populated for views inserted via the VCR API). Angular's
 * change detection is unaffected - it updates bindings in place on the
 * nodes, independent of DOM position, and destroys the view by removing the
 * node from whatever parent it currently has.
 */
@Injectable({ providedIn: 'root' })
export class MediaListOverlayService {
  private mediaListEl: HTMLElement | null = null;
  private originalParent: HTMLElement | null = null;
  private fullscreenHost: HTMLElement | null = null;
  // "Disable full screen media list" local setting: when checked the list
  // is never re-parented into the video container, even in full screen.
  private disabled = false;
  private overlayActive = false;
  private overlayActiveSubject = new BehaviorSubject<boolean>(false);
  /** Emits whenever the full-screen overlay state changes so portal hosts
   *  (e.g. context menus) can re-parent themselves into/out of the
   *  full-screen subtree. */
  overlayActive$: Observable<boolean> = this.overlayActiveSubject.asObservable();

  constructor(
    private videoService: VideoService,
    private localSettingsService: LocalSettingsService
  ) {
    const update = () => {
      const active = this.videoService.isFullScreen && !this.disabled;
      if (active === this.overlayActive) {
        return;
      }
      this.overlayActive = active;
      this.overlayActiveSubject.next(active);
      if (active) {
        this.moveToFullscreen();
      } else {
        this.moveToDefault();
      }
    };
    this.videoService.isFullScreen$.subscribe(update);
    this.localSettingsService.disableFullScreenMediaList$.subscribe(v => {
      this.disabled = v;
      update();
    });
  }

  registerMediaListElement(el: HTMLElement): void {
    if (this.mediaListEl === el) {
      return;
    }
    this.mediaListEl = el;
    // Capture the original position only when the element is not already
    // parked in the full screen host (a fresh element after a reconnect is
    // created in the app body, so this re-captures correctly).
    if (el.parentElement !== this.fullscreenHost) {
      this.originalParent = el.parentElement;
    }
    // If the overlay is already active (e.g. the media list was re-created
    // after a reconnect), move the fresh element into the overlay now.
    if (this.overlayActive) {
      this.moveToFullscreen();
    }
  }

  /**
   * Returns the element the context-menu portal should be appended to.
   * In full-screen overlay mode the video container is the full-screen
   * element (only its subtree renders), so the portal must live there.
   * In normal mode document.body is fine.
   */
  getPortalTarget(): HTMLElement {
    if (this.overlayActive && this.fullscreenHost) {
      return this.fullscreenHost;
    }
    return document.body;
  }

  registerFullscreenHost(el: HTMLElement): void {
    this.fullscreenHost = el;
  }

  private moveToFullscreen(): void {
    if (this.mediaListEl && this.fullscreenHost
        && this.mediaListEl.parentElement !== this.fullscreenHost) {
      // Defer the (large) subtree move to the next frame so the browser can
      // composite the full screen transition first - moving it synchronously
      // in the transition's first frame janks it. The overlay is parked
      // off-screen (translateY(100%)) until the sheet opens, so the one-frame
      // delay is invisible. Re-check in the callback in case the state changed
      // before the frame ran.
      requestAnimationFrame(() => {
        if (this.mediaListEl && this.fullscreenHost
            && this.mediaListEl.parentElement !== this.fullscreenHost) {
          this.fullscreenHost.appendChild(this.mediaListEl);
        }
      });
    }
  }

  private moveToDefault(): void {
    if (this.mediaListEl && this.originalParent
        && this.mediaListEl.parentElement !== this.originalParent) {
      // Same one-frame deferral as moveToFullscreen so the exit transition
      // isn't janked by the subtree move.
      requestAnimationFrame(() => {
        if (this.mediaListEl && this.originalParent
            && this.mediaListEl.parentElement !== this.originalParent) {
          this.originalParent.appendChild(this.mediaListEl);
        }
      });
    }
  }
}