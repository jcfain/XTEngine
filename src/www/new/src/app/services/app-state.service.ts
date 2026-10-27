import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { MediaItem, RemoteUserSettings } from '../models/interfaces';
import { ConnectionStatus, MediaType } from '../models/enums';

@Injectable({
  providedIn: 'root'
})
export class AppStateService {
  // Connection
  private xtpConnectedSubject = new BehaviorSubject<boolean>(false);
  xtpConnected$: Observable<boolean> = this.xtpConnectedSubject.asObservable();

  // Remote settings
  private remoteUserSettingsSubject = new BehaviorSubject<RemoteUserSettings | null>(null);
  remoteUserSettings$: Observable<RemoteUserSettings | null> = this.remoteUserSettingsSubject.asObservable();

  // Devices
  // private selectedInputDeviceSubject = new BehaviorSubject<ConnectionInterface>(
  //   JSON.parse(window.localStorage.getItem('selectedSyncConnection') || '0') as ConnectionInterface
  // );
  // selectedInputDevice$: Observable<ConnectionInterface> = this.selectedInputDeviceSubject.asObservable();

  // private selectedOutputDeviceSubject = new BehaviorSubject<ConnectionInterface>(
  //   JSON.parse(window.localStorage.getItem('selectedOutputConnection') || '0') as ConnectionInterface
  // );
  // selectedOutputDevice$: Observable<ConnectionInterface> = this.selectedOutputDeviceSubject.asObservable();

  // private outputConnectionStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  // outputConnectionStatus$: Observable<ConnectionStatus> = this.outputConnectionStatusSubject.asObservable();

  // private inputConnectionStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  // inputConnectionStatus$: Observable<ConnectionStatus> = this.inputConnectionStatusSubject.asObservable();

  // Misc
  private mediaLoadingSubject = new BehaviorSubject<boolean>(false);
  mediaLoading$: Observable<boolean> = this.mediaLoadingSubject.asObservable();

  private shufflePlayModeSubject = new BehaviorSubject<boolean>(false);
  shufflePlayMode$: Observable<boolean> = this.shufflePlayModeSubject.asObservable();

  private mediaManagementEnabledSubject = new BehaviorSubject<boolean>(false);
  mediaManagementEnabled$: Observable<boolean> = this.mediaManagementEnabledSubject.asObservable();

  // T-Code commands
  private tcodeCommandsSubject = new BehaviorSubject<any[]>([]);
  tcodeCommands$: Observable<any[]> = this.tcodeCommandsSubject.asObservable();

  // Media actions
  private mediaActionsSubject = new BehaviorSubject<Record<string, any>>({});
  mediaActions$: Observable<Record<string, any>> = this.mediaActionsSubject.asObservable();

  // Browser info
  userAgent: string = navigator.userAgent;
  userAgentIsDeo: boolean = this.userAgent.indexOf('Deo VR') !== -1;
  userAgentIsHereSphere: boolean = this.userAgent.indexOf('HereSphere') !== -1;
  userAgentIsMobile: boolean = this.userAgent.indexOf('Mobile') !== -1;

  // External streaming
  externalStreaming: boolean = JSON.parse(window.localStorage.getItem('externalStreaming') || 'false');

  // Disable the builtin browser lazy load
  disableBrowserLazyLoad: boolean = JSON.parse(window.localStorage.getItem('disableLazyLoad') || 'false');

  get xtpConnected(): boolean { return this.xtpConnectedSubject.value; }
  get remoteUserSettings(): RemoteUserSettings | null { return this.remoteUserSettingsSubject.value; }
  get mediaLoading(): boolean { return this.mediaLoadingSubject.value; }
  get shufflePlayMode(): boolean { return this.shufflePlayModeSubject.value; }
  get mediaManagementEnabled(): boolean { return this.mediaManagementEnabledSubject.value; }
  get mediaActions(): Record<string, any> { return this.mediaActionsSubject.value; }

  setXtpConnected(connected: boolean): void {
    this.xtpConnectedSubject.next(connected);
  }

  setRemoteUserSettings(settings: RemoteUserSettings): void {
    this.remoteUserSettingsSubject.next(settings);
  }

  setMediaLoading(loading: boolean): void {
    this.mediaLoadingSubject.next(loading);
  }

  setShufflePlayMode(enabled: boolean): void {
    this.shufflePlayModeSubject.next(enabled);
  }

  setMediaManagementEnabled(enabled: boolean): void {
    this.mediaManagementEnabledSubject.next(enabled);
  }

  setTcodeCommands(commands: any[]): void {
    this.tcodeCommandsSubject.next(commands);
  }

  setMediaActions(actions: Record<string, any>): void {
    this.mediaActionsSubject.next(actions);
  }
}