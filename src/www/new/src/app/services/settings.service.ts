import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { Setting, AllSettings } from '../models/interfaces';
import { FormControlType, SettingProfile } from '../models/enums';
import { WebsocketService } from './websocket.service';

@Injectable({
  providedIn: 'root'
})
export class SettingsService {
  private loaded = false;
  private debounceer: { [id: string]: any } = {};
  private debounceerTimeout = 1000;

  private settingsSubject = new BehaviorSubject<Setting[]>([]);
  settings$: Observable<Setting[]> = this.settingsSubject.asObservable();

  private mediaManagementEnabledSubject = new BehaviorSubject<boolean>(false);
  mediaManagementEnabled$: Observable<boolean> = this.mediaManagementEnabledSubject.asObservable();

  constructor(private websocketService: WebsocketService) {}

  get mediaManagementEnabled(): boolean {
    return this.mediaManagementEnabledSubject.value;
  }

  load(allSettings: AllSettings): void {
    if (this.loaded) return;
    const settings: Setting[] = [];
    allSettings['settings'].forEach(x => {
      if (x.internal) return;
      settings.push(x);
    });
    this.settingsSubject.next(settings);
    this.loaded = true;
  }

  getSetting(key: string): Setting | undefined {
    return this.settingsSubject.value.find(s => s.key === key);
  }

  getSettingsByGroup(group: string): Setting[] {
    return this.settingsSubject.value.filter(s => s.group === group);
  }

  getGroups(): string[] {
    const groups = new Set<string>();
    this.settingsSubject.value.forEach(s => groups.add(s.group));
    return Array.from(groups);
  }

  changeSetting(key: string, value: any): void {
    switch (key) {
      case 'enableMediaManagement':
        this.mediaManagementEnabledSubject.next(value);
        break;
    }
    this.websocketService.settingChange(key, value);
  }

  onSaved(key: string, value: any): void {
    // Handle post-save side effects
    switch (key) {
      case 'settingsBackupDirectory':
        // getExported();
        break;
    }
  }

  setSpecialProperties(key: string, value: any, control: HTMLInputElement): void {
    switch (key) {
      case 'playbackRateStep':
        control.min = '0.0';
        control.step = '0.01';
        break;
      case 'httpChunkSizeMB':
        control.min = '0.5';
        break;
      case 'scheduleLibraryLoadTime':
        control.min = '00:00';
        control.max = '23:59';
        break;
      case 'enableMediaManagement':
        this.mediaManagementEnabledSubject.next(value);
        break;
      case 'viewedThreshold':
        control.min = '1';
        control.max = '99';
        control.step = '1';
        break;
    }
  }
}