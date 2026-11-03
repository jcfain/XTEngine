import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { VideoPlayerComponent } from './components/video-player/video-player.component';
import { SettingsComponent } from './components/settings/settings.component';
import { ScriptStatusComponent } from './components/script-status/script-status.component';
import { MediaListComponent } from './components/media-list/media-list.component';
import { MediaToolbarComponent } from './components/media-toolbar/media-toolbar.component';
import { MetadataComponent } from './components/metadata/metadata.component';
import { ServerService } from './services/server.service';
import { MediaService } from './services/media.service';
import { AuthService } from './services/auth.service';
import { AppHeaderComponent } from './components/app-header/app-header.component';
import { AppFooterComponent } from './components/app-footer/app-footer.component';
import { AuthComponent } from './components/auth/auth.component';
import { WebsocketService } from './services/websocket.service';
import { MessageHandlerService } from './services/message-handler.service';
import { AuthResult, ConnectionStatus } from './models/enums';
import { KeyboardService } from './services/keyboard.service';
import { AppStateService } from './services/app-state.service';

@Component({
    selector: 'app-root',
    imports: [
    FormsModule,
    VideoPlayerComponent,
    SettingsComponent,
    ScriptStatusComponent,
    MediaListComponent,
    MediaToolbarComponent,
    MetadataComponent,
    AppHeaderComponent,
    AppFooterComponent,
    AuthComponent
],
    templateUrl: './app.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './app.component.css'
})
export class AppComponent implements OnInit {
  settingsOpen = false;
  scriptStatusOpen = false;
  metadataOpen = false;
  websocketState = ConnectionStatus.Disconnected;
  pongFailed = false;

  serverCheckTimeout: number | undefined;

  constructor(
    private appStateService: AppStateService,
    private websocketService: WebsocketService,
    private serverService: ServerService,
    private mediaService: MediaService,
    private authService: AuthService,
    private messageHandlerService: MessageHandlerService,
    private keyboardService: KeyboardService
  ) {
    this.keyboardService.init();
  }

  get isAuthenticated(): boolean {
    return this.authService.isAuthenticated;
  }

  get websocketConnected(): boolean {
    return this.websocketState == ConnectionStatus.Connected;
  }

  get websocketConnecting(): boolean {
    return this.websocketState == ConnectionStatus.Connecting;
  }

  ngOnInit(): void {
    this.serverService.serverPong$.subscribe(v => {
      if(v == ConnectionStatus.Connecting) {
        return;
      }
      if(this.serverCheckTimeout) {
        clearTimeout(this.serverCheckTimeout);
      }
      this.serverCheckTimeout = undefined;
      if(v === ConnectionStatus.Connected) {
          this.pongFailed = false;
        // Fetch server settings once on load; the response tells us whether auth is
        // enabled (hasLaunchPass) and gives us the WebSocket port.
        this.serverService.initServerSettings();
      } else {
        this.pongFailed = true;
        this.serverCheckTimeout = setTimeout(() => { this.startServerConnection() }, 5000);
      }
    });
    this.serverService.remoteUserSettings$.subscribe(v => {
        if(v)
          this.websocketService.init(v.webSocketServerPort);
    });
    this.serverService.mediaActions$.subscribe(v => {
      this.websocketService.setMediaActions(v);
    });
    this.websocketService.connectionStatus$.subscribe(v => {
      this.websocketState = v;
      this.appStateService.setServerState(v);
      if(v === ConnectionStatus.Disconnected) {
        this.startServerConnection();
      }
    });
    // Fetch the media list exactly once, when the server reports the library is
    // ready over the WebSocket. Anchoring to this (instead of the auth state)
    // avoids a premature /media request that fires on the session cookie before
    // the server has validated the session. The server re-serializes the entire
    // library on every /media call with no caching, so a duplicate request
    // blocks the single-threaded HTTP server - and the thumbnail requests -
    // for several seconds on every page load.
    this.messageHandlerService.mediaLoaded$.subscribe(() => {
      this.mediaService.loadMediaList();
    });
  }

  onAuth(result: AuthResult) {
    if(result == AuthResult.Success) {
      window.location.replace(window.location.origin);
    } else if(this.websocketState == ConnectionStatus.Disconnected) {
      this.startServerConnection();
    }
  }

  startServerConnection() {
    //This leads to a pong and kicks off the connection change
    this.serverService.ping();
  }

  toggleSettings(): void {
    this.settingsOpen = !this.settingsOpen;
  }

  toggleScriptStatus(): void {
    this.scriptStatusOpen = !this.scriptStatusOpen;
  }

  logout() {
    this.authService.logout();
  }
}
