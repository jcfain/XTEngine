import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';
import { AppStateService } from './app-state.service';
import { SettingsService } from './settings.service';
import { WebsocketService } from './websocket.service';

@Injectable({
  providedIn: 'root'
})
export class ServerService {
  constructor(
    private http: HttpClient,
    private appState: AppStateService,
    private settingsService: SettingsService,
    private websocketService: WebsocketService
  ) {}

  getServerSettings(): Observable<any> {
    return this.http.get<any>('/settings');
  }

  initServerSettings(): void {
    this.http.get<any>('/settings').subscribe({
      next: (response) => {
        this.appState.setRemoteUserSettings(response);
        this.settingsService.load(response);
        this.getMediaActions();
        this.getTCodeCommands();
        this.getExported();
        this.initWebSocket();
      },
      error: (err) => {
        console.error('Error getting server settings:', err);
      }
    });
  }

  getMediaActions(): void {
    this.http.get<any>('/mediaActions').subscribe({
      next: (response) => {
        this.appState.setMediaActions(response);
      },
      error: (err) => {
        console.error('Error getting media actions:', err);
      }
    });
  }

  getTCodeCommands(): void {
    this.http.get<any>('/tcodeCommands').subscribe({
      next: (response) => {
        this.appState.setTcodeCommands(response.commands || []);
      },
      error: (err) => {
        console.error('Error getting tcode commands:', err);
      }
    });
  }

  getServerChannels(): void {
    this.http.get<any>('/channels').subscribe({
      next: (response) => {
        const settings = this.appState.remoteUserSettings;
        if (settings) {
          settings.availableChannels = response.availableChannels;
          settings.selectedChannelProfile = response.selectedChannelProfile;
          settings.allChannelProfileNames = response.allChannelProfileNames;
          this.appState.setRemoteUserSettings(settings);
        }
      },
      error: (err) => {
        console.error('Error getting channels:', err);
      }
    });
  }

  getExported(): Observable<any> {
    return this.http.get<any>('/exported');
  }

  deleteExported(filename: string): Observable<any> {
    return this.http.delete<any>(`/exported/${filename}`);
  }

  restoreExported(filename: string): Observable<any> {
    return this.http.post<any>(`/exported/${filename}/restore`, {});
  }

  initWebSocket(): void {
    const settings = this.appState.remoteUserSettings;
    if (!settings) return;

    const wsUri = `ws://${window.location.hostname}:${settings.webSocketServerPort}`;
    this.websocketService.connect(wsUri);
  }

  restartXTP(): void {
    this.websocketService.send('restartService');
  }

  refreshXTPLibrary(): void {
    this.websocketService.send('reloadLibrary');
  }

  sendTCode(tcode: string): void {
    this.websocketService.sendTCode(tcode);
  }

  sendTCodeRange(channelName: string, min: number, max: number): void {
    this.websocketService.sendTCodeRange(channelName, min, max);
  }

  sendMediaAction(action: string): void {
    this.websocketService.sendMediaAction(action);
  }

  setSelectedProfile(profileName: string): void {
    this.websocketService.setSelectedProfile(profileName);
  }

  addChannelProfile(profileName: string): void {
    this.websocketService.addChannelProfile(profileName);
  }

  deleteChannelProfile(profileName: string): void {
    this.websocketService.deleteChannelProfile(profileName);
  }

  cloneChannelProfile(fromName: string, toName: string): void {
    this.websocketService.cloneChannelProfile(fromName, toName);
  }

  sendDeviceHome(): void {
    const actions = this.appState.mediaActions;
    this.websocketService.sendMediaAction(actions['TCodeHomeAll'] || 'TCodeHomeAll');
  }

  togglePauseAllDeviceActions(): void {
    const actions = this.appState.mediaActions;
    this.websocketService.sendMediaAction(actions['TogglePauseAllDeviceActions'] || 'TogglePauseAllDeviceActions');
  }

  sendQuickExport(): void {
    this.websocketService.sendQuickExport();
  }

  sendQuickImport(filename: string): void {
    this.websocketService.sendQuickImport(filename);
  }

  startMetadataProcess(): void {
    this.websocketService.sendMetadataProcess();
  }

  startMetadataCleanProcess(): void {
    this.websocketService.sendMetadataCleanProcess();
  }

  startThumbCleanupProcess(): void {
    this.websocketService.sendCleanupThumbsProcess();
  }

  startClean1024Process(): void {
    this.websocketService.sendClean1024Process();
  }

  sendUpdateMetadata(metadataKey: string): void {
    this.websocketService.sendUpdateMetadata(metadataKey);
  }

  sendSwapScript(scriptInfo: Record<string, any>): void {
    this.websocketService.sendSwapScript(scriptInfo);
  }

  sendSettingChange(key: string, value: any): void {
    this.websocketService.settingChange(key, value);
  }

  tcodeDeviceConnectRetry(): void {
    const settings = this.appState.remoteUserSettings;
    if (settings?.connection?.output?.selectedDevice) {
      this.websocketService.send('connectOutputDevice', { deviceName: settings.connection.output.selectedDevice });
    }
  }
}