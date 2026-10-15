import { Component, EventEmitter, OnInit, Output } from '@angular/core';
import { AppStateService } from '../../services/app-state.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

@Component({
  selector: 'app-header',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './app-header.component.html',
  styleUrl: './app-header.component.css'
})
export class AppHeaderComponent implements OnInit {
  @Output()
  settingsClick = new EventEmitter();
  @Output()
  scriptStatusClick = new EventEmitter();
  @Output()
  logoutClick = new EventEmitter();

  constructor() {}

  ngOnInit(): void { }

  onSettingsClick() {
    this.settingsClick.emit();
  }

  onScriptStatusClick() {
    this.scriptStatusClick.emit();
  }

  onLogoutClick() {
    this.logoutClick.emit();
  }
}
