import { Component, ElementRef, EventEmitter, OnDestroy, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { AuthService } from '../../services/auth.service';
import { ServerService } from '../../services/server.service';
import { XrPlayerService } from '../xr-player/xr-player.service';
import { AppMenuService } from '../../services/app-menu.service';
import { VideoService } from '../../services/video.service';

/**
 * Hamburger menu for the media list toolbar. Replaces the old app header:
 * the global actions (Settings, TCode, VR Media Player, Logout) now live in a
 * compact dropdown anchored to the toolbar's left edge.
 */
@Component({
  selector: 'app-menu',
  imports: [],
  templateUrl: './app-menu.component.html',
  changeDetection: ChangeDetectionStrategy.Eager,
  styleUrl: './app-menu.component.css'
})
export class AppMenuComponent implements OnInit, OnDestroy {
  // The TCode drawer is owned by the media toolbar (its parent), so the menu
  // just reports the click and lets the toolbar toggle the drawer.
  @Output() tcodeClick = new EventEmitter<void>();

  menuOpen = false;
  xrPlayerEnabled = false;
  get logoutVisible() {
    return this.authEnabled && this.isAuthenticated;
  }
  // Settings and the VR player render in the root component (outside the full
  // screen element's subtree), so they can't be shown while the video is full
  // screen - hide those items in that case.
  get settingsVisible() {
    return !this.isFullScreen;
  }
  get vrVisible() {
    return this.xrPlayerEnabled && !this.isFullScreen;
  }

  private isAuthenticated = false;
  private authEnabled: boolean | undefined = true;
  private isFullScreen = false;

  constructor(
    private elementRef: ElementRef<HTMLElement>,
    private authService: AuthService,
    private serverService: ServerService,
    private xrService: XrPlayerService,
    private appMenuService: AppMenuService,
    private videoService: VideoService
  ) {
    this.authService.isAuthenticated$.subscribe(v => {
      this.isAuthenticated = v;
    });
    this.serverService.remoteUserSettings$.subscribe(v => {
      this.authEnabled = v?.hasWebPass;
    });
    this.xrService.supported$.subscribe(v => this.xrPlayerEnabled = v);
    this.videoService.isFullScreen$.subscribe(v => this.isFullScreen = v);
  }

  ngOnInit(): void {
    document.addEventListener('click', this.onDocumentClick);
  }

  ngOnDestroy(): void {
    document.removeEventListener('click', this.onDocumentClick);
  }

  // Close the dropdown when clicking anywhere outside of it.
  private onDocumentClick = (event: MouseEvent): void => {
    if (this.menuOpen && !this.elementRef.nativeElement.contains(event.target as Node)) {
      this.menuOpen = false;
    }
  };

  toggleMenu(): void {
    this.menuOpen = !this.menuOpen;
  }

  onSettings(): void {
    this.menuOpen = false;
    this.appMenuService.openSettings();
  }

  onVr(): void {
    this.menuOpen = false;
    this.appMenuService.openVr();
  }

  onLogout(): void {
    this.appMenuService.logout();
  }

  onTcode(): void {
    this.menuOpen = false;
    this.tcodeClick.emit();
  }
}