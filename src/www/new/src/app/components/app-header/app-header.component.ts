import { Component, EventEmitter, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { AppStateService } from '../../services/app-state.service';
import { MessageHandlerService } from '../../services/message-handler.service';

import { FormsModule } from '@angular/forms';
import { ScriptStatusService } from '../../services/script-status.service';

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

  constructor(private scriptStatusService: ScriptStatusService) {}

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
