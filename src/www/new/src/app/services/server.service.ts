import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { BehaviorSubject, Observable } from 'rxjs';
import { AppStateService } from './app-state.service';
import { SettingsService } from './settings.service';
import { WebsocketService } from './websocket.service';
import { ServerState } from '../models/enums';
import { RemoteUserSettings, ScriptInfo, TCodeCommand } from '../models/interfaces';

@Injectable({
  providedIn: 'root'
})
export class ServerService {

  private serverState = new BehaviorSubject<ServerState>(ServerState.Disconnected);
  serverState$: Observable<ServerState> = this.serverState.asObservable();

  private mediaManagementEnabledSubject = new BehaviorSubject<boolean>(false);
  mediaManagementEnabled$: Observable<boolean> = this.mediaManagementEnabledSubject.asObservable();

  // T-Code commands
  private tcodeCommandsSubject = new BehaviorSubject<any[]>([]);
  tcodeCommands$: Observable<any[]> = this.tcodeCommandsSubject.asObservable();

  // Media actions
  private mediaActionsSubject = new BehaviorSubject<Record<string, any>>({});
  mediaActions$: Observable<Record<string, any>> = this.mediaActionsSubject.asObservable();

  // Remote settings
  private remoteUserSettingsSubject = new BehaviorSubject<RemoteUserSettings | null>(null);
  remoteUserSettings$: Observable<RemoteUserSettings | null> = this.remoteUserSettingsSubject.asObservable();

  get remoteUserSettings(): RemoteUserSettings | null { return this.remoteUserSettingsSubject.value; }

  constructor(
    private http: HttpClient,
    private settingsService: SettingsService,
    private websocketService: WebsocketService
  ) {}

  initServerSettings(): void {
    this.http.get<any>('/settings').subscribe({
      next: (response) => {
        this.websocketService.init(response.webSocketServerPort);
        this.remoteUserSettingsSubject.next(response);
        this.settingsService.load(response);
        this.getMediaActions();
        this.getTCodeCommands();
        this.getExported();
        this.serverState.next(ServerState.Connected);
      },
      error: (err) => {
        console.error('Error getting server settings:', err);
      }
    });
  }

  // getAvailableSerialPorts(): void {
  //   this.http.get<any>('/availableSerialPorts').subscribe({
  //     next: (response) => {
  //       this.availableSerialPorts.next(response);
  //     },
  //     error: (err) => {
  //       console.error('Error getting server serial ports:', err);
  //     }
  //   });
  // }

  getMediaActions(): void {
    this.http.get<any>('/mediaActions').subscribe({
      next: (response) => {
        this.mediaActionsSubject.next(response);
      },
      error: (err) => {
        console.error('Error getting media actions:', err);
      }
    });
  }

  getTCodeCommands(): void {
    this.http.get<any>('/tcodeCommands').subscribe({
      next: (response) => {
        this.tcodeCommandsSubject.next(response.commands || []);
      },
      error: (err) => {
        console.error('Error getting tcode commands:', err);
      }
    });
  }

  saveTCodeCommands(commands: TCodeCommand[]): Observable<any> {
    return this.http.post<any>('/tcodeCommands', commands);
  }

  getServerChannels(): void {
    this.http.get<any>('/channels').subscribe({
      next: (response) => {
        const settings = this.remoteUserSettingsSubject.value;
        if (settings) {
          settings.availableChannels = response.availableChannels;
          settings.selectedChannelProfile = response.selectedChannelProfile;
          settings.allChannelProfileNames = response.allChannelProfileNames;
          this.remoteUserSettingsSubject.next(settings);
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


  // sendMediaAction(action: string): void {
  //   this.websocketService.sendMediaAction(action);
  // }

  // sendDeviceHome(): void {
  //   const actions = this.mediaActionsSubject.value;
  //   this.websocketService.sendMediaAction(actions['TCodeHomeAll'] || 'TCodeHomeAll');
  // }

  // togglePauseAllDeviceActions(): void {
  //   const actions = this.mediaActionsSubject.value;
  //   this.websocketService.sendMediaAction(actions['TogglePauseAllDeviceActions'] || 'TogglePauseAllDeviceActions');
  // }

  // sendQuickExport(): void {
  //   this.websocketService.sendQuickExport();
  // }

  // sendQuickImport(filename: string): void {
  //   this.websocketService.sendQuickImport(filename);
  // }

  // startMetadataProcess(): void {
  //   this.websocketService.sendMetadataProcess();
  // }

  // startMetadataCleanProcess(): void {
  //   this.websocketService.sendMetadataCleanProcess();
  // }

  // startThumbCleanupProcess(): void {
  //   this.websocketService.sendCleanupThumbsProcess();
  // }

  // startClean1024Process(): void {
  //   this.websocketService.sendClean1024Process();
  // }

  // sendUpdateMetadata(metadataKey: string): void {
  //   this.websocketService.sendUpdateMetadata(metadataKey);
  // }

  // sendSwapScript(scriptInfo: ScriptInfo): void {
  //   this.websocketService.sendSwapScript(scriptInfo);
  // }

  saveServerSettings(): Observable<any> {
    return this.http.post<any>('/settings', this.remoteUserSettingsSubject.value);
  }
}