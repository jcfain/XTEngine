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
import { ConnectionStatus } from './models/enums';
import { KeyboardService } from './services/keyboard.service';

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
  websocketConnected = false;
  websocketConnecting = true;

  constructor(
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

  ngOnInit(): void {
    // Fetch server settings once on load; the response tells us whether auth is
    // enabled (hasLaunchPass) and gives us the WebSocket port.
    this.serverService.initServerSettings();
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
    this.websocketService.connectionStatus$.subscribe(v => {
      this.websocketConnected = v == ConnectionStatus.Connected;
      this.websocketConnecting = v == ConnectionStatus.Connecting;
    })
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
