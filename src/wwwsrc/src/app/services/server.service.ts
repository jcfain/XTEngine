import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { BehaviorSubject, Observable, Subscription, tap } from 'rxjs';
import { SettingsService } from './settings.service';
import { RemoteUserSettings, TCodeCommand, ActiveSession, ServerChannels } from '../models/interfaces';

@Injectable({
  providedIn: 'root'
})
export class ServerService {

  private mediaManagementEnabledSubject = new BehaviorSubject<boolean>(false);
  mediaManagementEnabled$: Observable<boolean> = this.mediaManagementEnabledSubject.asObservable();

  // T-Code commands
  private tcodeCommandsSubject = new BehaviorSubject<any[]>([]);
  tcodeCommands$: Observable<any[]> = this.tcodeCommandsSubject.asObservable();

  // Media actions
  private mediaActionsSubject = new BehaviorSubject<Record<string, string>>({});
  mediaActions$: Observable<Record<string, string>> = this.mediaActionsSubject.asObservable();

  // Remote settings
  private remoteUserSettingsSubject = new BehaviorSubject<RemoteUserSettings | null>(null);
  remoteUserSettings$: Observable<RemoteUserSettings | null> = this.remoteUserSettingsSubject.asObservable();

  // Remote settings
  private serverChannelsSubject = new BehaviorSubject<ServerChannels | null>(null);
  serverChannels$: Observable<ServerChannels | null> = this.serverChannelsSubject.asObservable();

  get remoteUserSettings(): RemoteUserSettings | null { return this.remoteUserSettingsSubject.value; }

  get serverChannels(): ServerChannels | null { return this.serverChannelsSubject.value; }
  
  remoteUserSettingsDirty = false;
  serverChannelsDirty = false;

  constructor(
    private http: HttpClient,
    private settingsService: SettingsService
  ) {
  }

  ping(): Observable<any> {
    return this.http.get<any>('/ping');
  }

  initServerSettings(): Observable<any> {
    return this.http.get<any>('/settings').pipe(
      tap(response => {
        this.remoteUserSettingsSubject.next(response);
        this.settingsService.load(response);
        this.getMediaActions();
        this.getTCodeCommands();
        this.getExported();
      })
    );
  }

  // TODO implement
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

  getServerChannels(): Subscription {
    return this.http.get<ServerChannels>('/channels').subscribe({
      next: (response) => {
        // const settings = this.remoteUserSettingsSubject.value;
        // if (settings) {
        //   settings.availableChannels = response.availableChannels;
        //   settings.selectedChannelProfile = response.selectedChannelProfile;
        //   settings.allChannelProfileNames = response.allChannelProfileNames;
        //   this.remoteUserSettingsSubject.next(settings);
        // }
        this.serverChannelsSubject.next(response);
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

  saveServerSettings(): Observable<any> {
    return this.http.post<any>('/settings', this.remoteUserSettingsSubject.value);
  }

  saveServerChannels(): Observable<any> {
    const channels = this.serverChannelsSubject.value;
    return this.http.post<any>('/channels', {
      availableChannels: channels?.availableChannels,
      multiplierEnabled: channels?.multiplierEnabled
    });
  }

  getActiveSessions(): Observable<ActiveSession[]> {
    return this.http.get<ActiveSession[]>('/activeSessions');
  }

  expireSession(sessionID: string): Observable<any> {
    return this.http.post<any>(`/expireSession?sessionID=${sessionID}`, {});
  }

  logout(): Observable<any> {
    return this.http.get<any>(`/logout`);
  }
}