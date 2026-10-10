import { Component, OnInit, Input, Output, EventEmitter } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { SettingsService } from '../../services/settings.service';
import { Setting } from '../../models/interfaces';
import { FormControlType } from '../../models/enums';
import { capitalizeFirstLetter } from '../../utils/utils';

interface SettingGroup {
  name: string;
  settings: Setting[];
}

@Component({
  selector: 'app-settings',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './settings.component.html',
  styleUrl: './settings.component.css'
})
export class SettingsComponent implements OnInit {
  @Input() isOpen = false;
  @Output() toggle = new EventEmitter<void>();

  groups: SettingGroup[] = [];

  constructor(private settingsService: SettingsService) {}

  ngOnInit(): void {
    this.settingsService.settings$.subscribe(settings => {
      const groupMap = new Map<string, Setting[]>();
      settings.forEach(s => {
        if (!groupMap.has(s.group)) {
          groupMap.set(s.group, []);
        }
        groupMap.get(s.group)!.push(s);
      });
      this.groups = Array.from(groupMap.entries()).map(([name, settings]) => ({
        name,
        settings
      }));
    });
  }

  onToggle(): void {
    this.toggle.emit();
  }

  getControlType(type: FormControlType): string {
    switch (type) {
      case FormControlType.Double:
      case FormControlType.Int:
        return 'number';
      case FormControlType.Text:
        return 'text';
      case FormControlType.Radio:
        return 'radio';
      case FormControlType.Combo:
        return 'select';
      case FormControlType.Checkbox:
        return 'checkbox';
      case FormControlType.DateTime:
        return 'datetime-local';
      case FormControlType.Date:
        return 'date';
      case FormControlType.Time:
        return 'time';
      default:
        return 'text';
    }
  }

  onValueChange(setting: Setting, value: any): void {
    this.settingsService.changeSetting(setting.key, value);
  }

  capitalizeFirstLetter(val: string): string {
    return capitalizeFirstLetter(val);
  }
}