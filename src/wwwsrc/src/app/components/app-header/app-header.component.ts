import { Component, EventEmitter, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { AppStateService } from '../../services/app-state.service';
import { MessageHandlerService } from '../../services/message-handler.service';

import { FormsModule } from '@angular/forms';
import { ScriptStatusService } from '../../services/script-status.service';
import { AuthService } from '../../services/auth.service';
import { ServerService } from '../../services/server.service';
import { XrPlayerService } from '../xr-player/xr-player.service';

@Component({
    selector: 'app-header',
    imports: [FormsModule],
    templateUrl: './app-header.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './app-header.component.css'
})
export class AppHeaderComponent implements OnInit {
  @Output()
  settingsClick = new EventEmitter();
  @Output()
  scriptStatusClick = new EventEmitter();
  @Output()
  logoutClick = new EventEmitter();
  // WebXR VR media player (self-contained feature in components/xr-player)
  @Output()
  vrClick = new EventEmitter();

  xrPlayerEnabled = false;

  get logoutVisible() {
    return this.authEnabled && this.isAuthenticated;
  }
  
  private isAuthenticated = false;
  private authEnabled: boolean | undefined = true;


  constructor(
    private authService: AuthService,
    private serverService: ServerService,
    private scriptStatusService: ScriptStatusService,
    private xrService: XrPlayerService
  ) {
    this.authService.isAuthenticated$.subscribe(v => {
      this.isAuthenticated = v;
    });
    this.serverService.remoteUserSettings$.subscribe(v => {
      this.authEnabled = v?.hasWebPass;
    })
    this.xrService.supported$.subscribe(v => this.xrPlayerEnabled = v);
  }

  ngOnInit(): void { }

  onSettingsClick($event: MouseEvent) {
    this.settingsClick.emit();
  }

  onVrClick() {
    this.vrClick.emit();
  }

  onScriptStatusClick($event: MouseEvent) {
    this.scriptStatusService.toggleScriptStatusHeader($event);
    // this.scriptStatusClick.emit();
  }

  onLogoutClick() {
    this.logoutClick.emit();
  }
}
