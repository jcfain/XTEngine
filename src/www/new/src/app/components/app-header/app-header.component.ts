import { Component, EventEmitter, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { AppStateService } from '../../services/app-state.service';
import { MessageHandlerService } from '../../services/message-handler.service';

import { FormsModule } from '@angular/forms';
import { ScriptStatusService } from '../../services/script-status.service';
import { AuthService } from '../../services/auth.service';

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

  isAuthenticated = false;

  constructor(
    private authService: AuthService,
    private scriptStatusService: ScriptStatusService
  ) {
    this.authService.isAuthenticated$.subscribe(v => {
      this.isAuthenticated = v;
    });
  }

  ngOnInit(): void { }

  onSettingsClick($event: MouseEvent) {
    this.settingsClick.emit();
  }

  onScriptStatusClick($event: MouseEvent) {
    this.scriptStatusService.toggleScriptStatus($event);
    // this.scriptStatusClick.emit();
  }

  onLogoutClick() {
    this.logoutClick.emit();
  }
}
