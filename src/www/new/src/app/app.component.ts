import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VideoPlayerComponent } from './components/video-player/video-player.component';
import { SettingsComponent } from './components/settings/settings.component';
import { ScriptStatusComponent } from './components/script-status/script-status.component';
import { MediaListComponent } from './components/media-list/media-list.component';
import { MediaToolbarComponent } from './components/media-toolbar/media-toolbar.component';
import { ConnectionsComponent } from './components/connections/connections.component';
import { TCodeCommandsComponent } from './components/tcode-commands/tcode-commands.component';
import { MetadataComponent } from './components/metadata/metadata.component';
import { WebsocketService } from './services/websocket.service';
import { SettingsService } from './services/settings.service';
import { ServerService } from './services/server.service';
import { MediaService } from './services/media.service';
import { AppStateService } from './services/app-state.service';
import { ConnectionStatus } from './models/enums';
import { MessageHandlerService } from './services/message-handler.service';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [
    CommonModule, 
    FormsModule, 
    VideoPlayerComponent, 
    SettingsComponent, 
    ScriptStatusComponent, 
    MediaListComponent, 
    MediaToolbarComponent, 
    ConnectionsComponent, 
    TCodeCommandsComponent, 
    MetadataComponent
  ],
  templateUrl: './app.component.html',
  styleUrl: './app.component.css'
})
export class AppComponent implements OnInit {
  title = 'XTP Web';
  webVersion = 'v0.6.6b';
  settingsOpen = false;
  scriptStatusOpen = false;
  metadataOpen = false;

  constructor(
    private messageHandlerService: MessageHandlerService,
    private serverService: ServerService,
    private mediaService: MediaService,
    private appState: AppStateService
  ) {}

  ngOnInit(): void {
    this.messageHandlerService.outputDeviceStatus$.subscribe(status => { 
      this.appState.setOutputConnectionStatus(status)
    });
    this.messageHandlerService.inputDeviceStatus$.subscribe(status => { 
      this.appState.setInputConnectionStatus(status)
    });

    // Initialize: load server settings, then media list
    this.serverService.initServerSettings();
    this.mediaService.loadMediaList();
  }

  toggleSettings(): void {
    this.settingsOpen = !this.settingsOpen;
  }

  toggleScriptStatus(): void {
    this.scriptStatusOpen = !this.scriptStatusOpen;
  }

  openMetadata(): void {
    this.metadataOpen = true;
  }
}
