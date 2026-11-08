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
import { ConnectionStateService, ConnectionState } from './services/connection-state.service';

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
  connectionState = ConnectionState.Disconnected;

  constructor(
    private appStateService: AppStateService,
    private websocketService: WebsocketService,
    private serverService: ServerService,
    private mediaService: MediaService,
    private authService: AuthService,
    private messageHandlerService: MessageHandlerService,
    private keyboardService: KeyboardService,
    private connectionStateService: ConnectionStateService
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
    this.connectionStateService.state$.subscribe(state => {
      this.connectionState = state;
      // Keep AppStateService in sync for any components that read it
      if (state === ConnectionState.Connected) {
        this.appStateService.setServerState(ConnectionStatus.Connected);
      } else {
        this.appStateService.setServerState(ConnectionStatus.Disconnected);
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
  }

  onAuth(result: AuthResult) {
    // Auth state changes are handled by the state machine via isAuthenticated$
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
