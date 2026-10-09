import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { VideoPlayerComponent } from './components/video-player/video-player.component';
import { SettingsComponent } from './components/settings/settings.component';
import { ScriptStatusComponent } from './components/script-status/script-status.component';
import { WebsocketService } from './services/websocket.service';
import { SettingsService } from './services/settings.service';
import { ConnectionStatus } from './models/enums';

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule, FormsModule, VideoPlayerComponent, SettingsComponent, ScriptStatusComponent],
  templateUrl: './app.component.html',
  styleUrl: './app.component.css'
})
export class AppComponent implements OnInit {
  title = 'XTP Web';
  webVersion = 'v0.6.6b';
  connectionStatus = ConnectionStatus.Disconnected;
  settingsOpen = false;
  scriptStatusOpen = false;

  constructor(
    private websocketService: WebsocketService,
    private settingsService: SettingsService
  ) {}

  ngOnInit(): void {
    this.websocketService.connectionStatus$.subscribe(status => {
      this.connectionStatus = status;
    });
  }

  toggleSettings(): void {
    this.settingsOpen = !this.settingsOpen;
  }

  toggleScriptStatus(): void {
    this.scriptStatusOpen = !this.scriptStatusOpen;
  }

  isConnected(): boolean {
    return this.connectionStatus === ConnectionStatus.Connected;
  }

  statusText(): string {
    switch (this.connectionStatus) {
      case ConnectionStatus.Connected: return 'Connected';
      case ConnectionStatus.Connecting: return 'Connecting...';
      case ConnectionStatus.Error: return 'Error';
      default: return 'Disconnected';
    }
  }
}