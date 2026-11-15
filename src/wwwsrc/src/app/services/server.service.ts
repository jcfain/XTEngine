import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { BehaviorSubject, Observable, Subscription, tap, defer, switchMap } from 'rxjs';
import { SettingsService } from './settings.service';
import { MessageHandlerService } from './message-handler.service';
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

  // The channel profile the user is currently viewing (kept in sync by the
  // Motion tab). Used to detect a stale cache at save time (profile switched but
  // the refetch hasn't landed yet).
  private selectedChannelProfileSubject = new BehaviorSubject<string | null>(null);
  selectedChannelProfile$: Observable<string | null> = this.selectedChannelProfileSubject.asObservable();

  get remoteUserSettings(): RemoteUserSettings | null { return this.remoteUserSettingsSubject.value; }

  get serverChannels(): ServerChannels | null { return this.serverChannelsSubject.value; }
  
  remoteUserSettingsDirty = false;
  serverChannelsDirty = false;

  constructor(
    private http: HttpClient,
    private settingsService: SettingsService,
    private messageHandlerService: MessageHandlerService
  ) {
    // Keep the channel cache in sync with profile switches, regardless of which
    // tab is open. The server also emits channelProfileChanged ~100ms after a
    // save (setAxis) where the profile is unchanged; only refetch when the
    // profile actually changed so we don't clobber the user's in-flight edits
    // (which live in the same object refs as the cache).
    this.messageHandlerService.channelProfileChanged$.subscribe(profile => {
      if (profile && this.serverChannelsSubject.value?.selectedChannelProfile !== profile) {
        this.getServerChannels();
      }
    });
  }

  setSelectedChannelProfile(profile: string): void {
    this.selectedChannelProfileSubject.next(profile);
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

  // Observable variant of getServerChannels() for chaining (e.g., fetch-then-save).
  fetchServerChannels(): Observable<ServerChannels> {
    return this.http.get<ServerChannels>('/channels').pipe(
      tap(response => this.serverChannelsSubject.next(response))
    );
  }

  getServerChannels(): Subscription {
    return this.fetchServerChannels().subscribe({
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
    return defer(() => {
      const cached = this.serverChannelsSubject.value;
      const intended = this.selectedChannelProfileSubject.value;
      // Refresh before saving if the cache is empty (init fetch missed) or is for
      // a different profile than the one the user is viewing (stale after a
      // profile switch whose refetch hasn't landed). Safe: no in-flight edits
      // exist when the cache is empty, and stale edits belong to the profile the
      // user switched away from.
      const stale = !cached || (intended !== null && cached.selectedChannelProfile !== intended);
      if (stale) {
        return this.fetchServerChannels().pipe(
          switchMap(() => this.http.post<any>('/channels', {
            availableChannels: this.serverChannelsSubject.value?.availableChannels,
            multiplierEnabled: this.serverChannelsSubject.value?.multiplierEnabled
          }))
        );
      }
      return this.http.post<any>('/channels', {
        availableChannels: cached.availableChannels,
        multiplierEnabled: cached.multiplierEnabled
      });
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