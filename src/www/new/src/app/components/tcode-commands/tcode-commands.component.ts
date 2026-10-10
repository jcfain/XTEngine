import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';

interface TCodeCommand {
  name: string;
  command: string;
  hidden: boolean;
}

@Component({
  selector: 'app-tcode-commands',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './tcode-commands.component.html',
  styleUrl: './tcode-commands.component.css'
})
export class TCodeCommandsComponent implements OnInit {
  commands: TCodeCommand[] = [];
  newCommandName = '';
  newCommandValue = '';
  showSetup = false;

  constructor(
    private appState: AppStateService,
    private serverService: ServerService
  ) {}

  ngOnInit(): void {
    this.appState.tcodeCommands$.subscribe(commands => {
      this.commands = commands as TCodeCommand[];
    });
  }

  sendCommand(command: TCodeCommand): void {
    this.serverService.sendTCode(command.command);
  }

  toggleSetup(): void {
    this.showSetup = !this.showSetup;
  }

  addCommand(): void {
    if (!this.newCommandName.trim() || !this.newCommandValue.trim()) return;
    const newCmd: TCodeCommand = {
      name: this.newCommandName.trim(),
      command: this.newCommandValue.trim(),
      hidden: false
    };
    this.commands.push(newCmd);
    this.newCommandName = '';
    this.newCommandValue = '';
    // Save to server
    this.serverService.sendSettingChange('tcodeCommands', this.commands);
  }

  deleteCommand(index: number): void {
    this.commands.splice(index, 1);
    this.serverService.sendSettingChange('tcodeCommands', this.commands);
  }

  toggleHidden(command: TCodeCommand): void {
    command.hidden = !command.hidden;
    this.serverService.sendSettingChange('tcodeCommands', this.commands);
  }
}