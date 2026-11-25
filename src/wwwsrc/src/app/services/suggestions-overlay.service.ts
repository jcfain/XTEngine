import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { VideoService } from './video.service';
import { LocalSettingsService } from './local-settings.service';

/**
 * Moves the video suggestions panel between its normal grid position and the
 * full screen media sheet, where it sits as a flex item between the media
 * toolbar and the media list (so it is the topmost content of the sheet).
 * Mirrors MediaListOverlayService: a full screen element only renders its own
 * subtree, so the panel is physically re-parented with appendChild and moved
 * back to its original parent when full screen exits.
 *
 * The panel is re-parented whenever full screen is active (it moves with the
 * sheet, so it is only visible while the sheet is open) and is kept in the
 * grid by the "Disable full screen media list" setting.
 */
@Injectable({ providedIn: 'root' })
export class SuggestionsOverlayService {
  private panelEl: HTMLElement | null = null;
  private originalParent: HTMLElement | null = null;
  private sheetHost: HTMLElement | null = null;
  private disabled = false;
  private isFullScreen = false;

  // Full screen && not disabled: the panel is (or should be) re-parented into
  // the media sheet.
  private overlayActiveSubject = new BehaviorSubject<boolean>(false);
  overlayActive$: Observable<boolean> = this.overlayActiveSubject.asObservable();

  constructor(
    private videoService: VideoService,
    private localSettingsService: LocalSettingsService
  ) {
    this.videoService.isFullScreen$.subscribe(v => {
      this.isFullScreen = v;
      this.update();
    });
    this.localSettingsService.disableFullScreenMediaList$.subscribe(v => {
      this.disabled = v;
      this.update();
    });
  }

  registerPanelElement(el: HTMLElement): void {
    if (this.panelEl === el) {
      return;
    }
    this.panelEl = el;
    // Capture the original position only when the element is not already
    // parked in the sheet host (a fresh element after a mode switch is created
    // in the grid, so this re-captures correctly).
    if (el.parentElement !== this.sheetHost) {
      this.originalParent = el.parentElement;
    }
    // If the overlay is already active (e.g. the panel was re-created), move
    // the fresh element into the sheet now.
    if (this.overlayActiveSubject.value) {
      this.moveToSheet();
    }
  }

  registerSheetHost(el: HTMLElement): void {
    this.sheetHost = el;
    if (this.overlayActiveSubject.value) {
      this.moveToSheet();
    }
  }

  private update(): void {
    const overlayActive = this.isFullScreen && !this.disabled;
    if (overlayActive === this.overlayActiveSubject.value) {
      return;
    }
    this.overlayActiveSubject.next(overlayActive);
    if (overlayActive) {
      this.moveToSheet();
    } else {
      this.moveToDefault();
    }
  }

  private moveToSheet(): void {
    if (this.panelEl && this.sheetHost
        && this.panelEl.parentElement !== this.sheetHost) {
      // Defer the subtree move to the next frame so the browser can composite
      // the full screen transition first (same as MediaListOverlayService).
      // The panel moves with the sheet, which is parked off-screen until the
      // sheet opens, so the one-frame delay is invisible.
      requestAnimationFrame(() => {
        if (this.panelEl && this.sheetHost
            && this.panelEl.parentElement !== this.sheetHost) {
          this.sheetHost.appendChild(this.panelEl);
        }
      });
    }
  }

  private moveToDefault(): void {
    if (this.panelEl && this.originalParent
        && this.panelEl.parentElement !== this.originalParent) {
      requestAnimationFrame(() => {
        if (this.panelEl && this.originalParent
            && this.panelEl.parentElement !== this.originalParent) {
          this.originalParent.appendChild(this.panelEl);
        }
      });
    }
  }
}