import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { VideoPlayerComponent } from './components/video-player/video-player.component';
import { ExternalPlayerComponent } from './components/external-player/external-player.component';
import { XrPlayerComponent } from './components/xr-player/xr-player.component';
import { SettingsComponent } from './components/settings/settings.component';
import { ScriptStatusComponent } from './components/script-status/script-status.component';
import { MediaListComponent } from './components/media-list/media-list.component';
import { ServerService } from './services/server.service';
import { MediaService } from './services/media.service';
import { AuthService } from './services/auth.service';
import { AppFooterComponent } from './components/app-footer/app-footer.component';
import { AuthComponent } from './components/auth/auth.component';
import { AlertsComponent } from './components/alerts/alerts.component';
import { ConfirmDialogComponent } from './components/confirm-dialog/confirm-dialog.component';
import { WebsocketService } from './services/websocket.service';
import { MessageHandlerService } from './services/message-handler.service';
import { AlertService } from './services/alert.service';
import { AuthResult, ConnectionStatus } from './models/enums';
import { KeyboardService } from './services/keyboard.service';
import { AppStateService } from './services/app-state.service';
import { ConnectionStateService, ConnectionState } from './services/connection-state.service';
import { DeviceConnectionService } from './services/device-connection.service';
import { MediaListService } from './services/media-list-service';
import { VideoService } from './services/video.service';
import { FullscreenOverlayContainerService } from './services/fullscreen-overlay-container.service';
import { LocalSettingsService } from './services/local-settings.service';
import { AppMenuService } from './services/app-menu.service';

@Component({
    selector: 'app-root',
    imports: [
    FormsModule,
    VideoPlayerComponent,
    ExternalPlayerComponent,
    XrPlayerComponent,
    SettingsComponent,
    ScriptStatusComponent,
    MediaListComponent,
    AppFooterComponent,
    AuthComponent,
    AlertsComponent,
    ConfirmDialogComponent
],
    templateUrl: './app.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './app.component.css'
})
export class AppComponent implements OnInit {
  scriptStatusOpen = false;
  // Settings modal for external player mode only - in normal mode the video
  // player renders its own instance inside the video container (so it also
  // shows in full screen). mediaLoaded is the same gate the modal always
  // had: the settings tabs bind to the server settings, which are only
  // fetched once the media list has loaded.
  settingsOpen = false;
  mediaLoaded = false;
  // WebXR VR media player overlay (self-contained feature in
  // components/xr-player - remove it by deleting the folder and these
  // two references).
  vrPlayerOpen = false;
  connectionState = ConnectionState.Disconnected;
  // "External player" local setting: media plays in a separate player
  // instead of the in-page video player, so the compact external player
  // component replaces the video player in the layout.
  externalStreaming = false;

  constructor(
    private appStateService: AppStateService,
    private websocketService: WebsocketService,
    private serverService: ServerService,
    private mediaService: MediaService,
    private mediaListService: MediaListService,
    private videoService: VideoService,
    private authService: AuthService,
    private messageHandlerService: MessageHandlerService,
    private alertService: AlertService,
    private keyboardService: KeyboardService,
    private connectionStateService: ConnectionStateService,
    private deviceConnectionService: DeviceConnectionService,
    private localSettingsService: LocalSettingsService,
    // Injected for its side effect: keeps the CDK overlay container inside
    // the full screen element so modals render while the video is full screen.
    private fullscreenOverlayContainerService: FullscreenOverlayContainerService,
    private appMenuService: AppMenuService
  ) {
    this.keyboardService.init();
  }

  get isAuthenticated(): boolean {
    return this.authService.isAuthenticated;
  }

  get isConnected(): boolean {
    return this.connectionState === ConnectionState.Connected;
  }

  get isConnecting(): boolean {
    return this.connectionState === ConnectionState.Disconnected
        || this.connectionState === ConnectionState.Pinging
        || this.connectionState === ConnectionState.Configured;
  }

  get isAwaitingAuth(): boolean {
    return this.connectionState === ConnectionState.AwaitingAuth;
  }

  ngOnInit(): void {
    this.localSettingsService.externalStreaming$.subscribe(v => {
      this.externalStreaming = v;
    });

    // The hamburger menu (inside the media list toolbar) drives these
    // app-level dialogs/actions via the AppMenuService.
    this.appMenuService.settingsToggle$.subscribe(() => {
      // In normal mode the video player owns the settings modal and handles
      // the toggle itself - only the external player mode (no video player
      // in the page) uses this app-level instance.
      if (!this.externalStreaming || !this.mediaLoaded) {
        return;
      }
      this.settingsOpen = !this.settingsOpen;
    });
    this.appMenuService.vrOpen$.subscribe(() => this.vrPlayerOpen = true);

    this.connectionStateService.state$.subscribe(state => {
      this.connectionState = state;
      // Keep AppStateService in sync for any components that read it
      if (state === ConnectionState.Connected) {
        this.deviceConnectionService.init();
      } else {
        // Bust the client-side media list cache when the backend exits.
        // This forces a fresh fetch on reconnect and avoids the "stale list
        // displayed + loading progress" conflict that blocks the main thread.
        this.mediaListService.setMediaList([]);
      }
    });

    this.serverService.mediaActions$.subscribe(v => {
      this.websocketService.setMediaActions(v);
    });

    // Fetch the media list exactly once, when the server reports the library is
    // ready over the WebSocket.
    this.messageHandlerService.mediaLoaded$.subscribe(() => {
      this.mediaService.loadMediaList();
    });

    this.connectionStateService.start();
    this.deviceConnectionService.init();
  }

  onAuth(result: AuthResult) {
    // Auth state changes are handled by the state machine via isAuthenticated$
  }

  toggleScriptStatus(): void {
    this.scriptStatusOpen = !this.scriptStatusOpen;
  }

  logout() {
    this.authService.logout();
  }
}
