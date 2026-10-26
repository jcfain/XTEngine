import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { VideoPlayerComponent } from './components/video-player/video-player.component';
import { SettingsComponent } from './components/settings/settings.component';
import { ScriptStatusComponent } from './components/script-status/script-status.component';
import { MediaListComponent } from './components/media-list/media-list.component';
import { MediaToolbarComponent } from './components/media-toolbar/media-toolbar.component';
import { TCodeCommandsComponent } from './components/tcode-commands/tcode-commands.component';
import { MetadataComponent } from './components/metadata/metadata.component';
import { ServerService } from './services/server.service';
import { MediaService } from './services/media.service';
import { AuthService } from './services/auth.service';
import { AppHeaderComponent } from './components/app-header/app-header.component';
import { MessageHandlerService } from './services/message-handler.service';
import { AppStateService } from './services/app-state.service';
import { OutputDeviceToolsComponent } from './components/device-tools/output/output-device-tools.component';
import { AppFooterComponent } from './components/app-footer/app-footer.component';

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
    AppFooterComponent
],
    templateUrl: './app.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './app.component.css'
})
export class AppComponent implements OnInit {
  settingsOpen = false;
  scriptStatusOpen = false;
  metadataOpen = false;

  constructor(
    private serverService: ServerService,
    private mediaService: MediaService,
    private authService: AuthService
  ) {
  }

  ngOnInit(): void {
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

  logout() {
    this.authService.logout();
  }
}
