import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { MediaItem, RemoteUserSettings } from '../models/interfaces';
import { ConnectionStatus, DeviceType, MediaType } from '../models/enums';

@Injectable({
  providedIn: 'root'
})
export class AppStateService {
  // Connection
  private xtpConnectedSubject = new BehaviorSubject<boolean>(false);
  xtpConnected$: Observable<boolean> = this.xtpConnectedSubject.asObservable();

  // Media list
  private mediaListSubject = new BehaviorSubject<MediaItem[]>([]);
  mediaList$: Observable<MediaItem[]> = this.mediaListSubject.asObservable();

  private mediaListDisplayedSubject = new BehaviorSubject<MediaItem[]>([]);
  mediaListDisplayed$: Observable<MediaItem[]> = this.mediaListDisplayedSubject.asObservable();

  // Playing media
  private playingMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
  playingMediaItem$: Observable<MediaItem | null> = this.playingMediaItemSubject.asObservable();

  // Selected media for metadata
  private selectedMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
  selectedMediaItem$: Observable<MediaItem | null> = this.selectedMediaItemSubject.asObservable();

  // Remote settings
  private remoteUserSettingsSubject = new BehaviorSubject<RemoteUserSettings | null>(null);
  remoteUserSettings$: Observable<RemoteUserSettings | null> = this.remoteUserSettingsSubject.asObservable();

  // Filter
  private filterCriteriaSubject = new BehaviorSubject<string>('');
  filterCriteria$: Observable<string> = this.filterCriteriaSubject.asObservable();

  private tagFilterCriteriaSubject = new BehaviorSubject<string[]>([]);
  tagFilterCriteria$: Observable<string[]> = this.tagFilterCriteriaSubject.asObservable();

  // Sort / Show
  private sortBySubject = new BehaviorSubject<string>(JSON.parse(window.localStorage.getItem('sortBy') || '"name"'));
  sortBy$: Observable<string> = this.sortBySubject.asObservable();

  private showSubject = new BehaviorSubject<string>(JSON.parse(window.localStorage.getItem('show') || '"All"'));
  show$: Observable<string> = this.showSubject.asObservable();

  // Thumb size
  private thumbSizeSubject = new BehaviorSubject<number>(JSON.parse(window.localStorage.getItem('thumbSize') || '200'));
  thumbSize$: Observable<number> = this.thumbSizeSubject.asObservable();

  // Devices
  private selectedInputDeviceSubject = new BehaviorSubject<DeviceType>(
    JSON.parse(window.localStorage.getItem('selectedSyncConnection') || '0') as DeviceType
  );
  selectedInputDevice$: Observable<DeviceType> = this.selectedInputDeviceSubject.asObservable();

  private selectedOutputDeviceSubject = new BehaviorSubject<DeviceType>(
    JSON.parse(window.localStorage.getItem('selectedOutputConnection') || '0') as DeviceType
  );
  selectedOutputDevice$: Observable<DeviceType> = this.selectedOutputDeviceSubject.asObservable();

  private outputConnectionStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  outputConnectionStatus$: Observable<ConnectionStatus> = this.outputConnectionStatusSubject.asObservable();

  private inputConnectionStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  inputConnectionStatus$: Observable<ConnectionStatus> = this.inputConnectionStatusSubject.asObservable();

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

  // Disable lazy load
  disableLazyLoad: boolean = JSON.parse(window.localStorage.getItem('disableLazyLoad') || 'false');

  get xtpConnected(): boolean { return this.xtpConnectedSubject.value; }
  get mediaList(): MediaItem[] { return this.mediaListSubject.value; }
  get playingMediaItem(): MediaItem | null { return this.playingMediaItemSubject.value; }
  get selectedMediaItem(): MediaItem | null { return this.selectedMediaItemSubject.value; }
  get remoteUserSettings(): RemoteUserSettings | null { return this.remoteUserSettingsSubject.value; }
  get filterCriteria(): string { return this.filterCriteriaSubject.value; }
  get sortBy(): string { return this.sortBySubject.value; }
  get show(): string { return this.showSubject.value; }
  get thumbSize(): number { return this.thumbSizeSubject.value; }
  get selectedInputDevice(): DeviceType { return this.selectedInputDeviceSubject.value; }
  get selectedOutputDevice(): DeviceType { return this.selectedOutputDeviceSubject.value; }
  get mediaLoading(): boolean { return this.mediaLoadingSubject.value; }
  get shufflePlayMode(): boolean { return this.shufflePlayModeSubject.value; }
  get mediaManagementEnabled(): boolean { return this.mediaManagementEnabledSubject.value; }
  get mediaActions(): Record<string, any> { return this.mediaActionsSubject.value; }

  setXtpConnected(connected: boolean): void {
    this.xtpConnectedSubject.next(connected);
  }

  setMediaList(list: MediaItem[]): void {
    this.mediaListSubject.next(list);
    this.applyFilterAndSort();
  }

  setPlayingMediaItem(item: MediaItem | null): void {
    this.playingMediaItemSubject.next(item);
  }

  setSelectedMediaItem(item: MediaItem | null): void {
    this.selectedMediaItemSubject.next(item);
  }

  setRemoteUserSettings(settings: RemoteUserSettings): void {
    this.remoteUserSettingsSubject.next(settings);
  }

  setFilterCriteria(criteria: string): void {
    this.filterCriteriaSubject.next(criteria);
    this.applyFilterAndSort();
  }

  setTagFilterCriteria(tags: string[]): void {
    this.tagFilterCriteriaSubject.next(tags);
    this.applyFilterAndSort();
  }

  setSortBy(sortBy: string): void {
    this.sortBySubject.next(sortBy);
    window.localStorage.setItem('sortBy', JSON.stringify(sortBy));
    this.applyFilterAndSort();
  }

  setShow(show: string): void {
    this.showSubject.next(show);
    window.localStorage.setItem('show', JSON.stringify(show));
    this.applyFilterAndSort();
  }

  setThumbSize(size: number): void {
    this.thumbSizeSubject.next(size);
    window.localStorage.setItem('thumbSize', JSON.stringify(size));
  }

  setSelectedInputDevice(device: DeviceType): void {
    this.selectedInputDeviceSubject.next(device);
    window.localStorage.setItem('selectedSyncConnection', String(device));
  }

  setSelectedOutputDevice(device: DeviceType): void {
    this.selectedOutputDeviceSubject.next(device);
    window.localStorage.setItem('selectedOutputConnection', String(device));
  }

  setOutputConnectionStatus(status: ConnectionStatus): void {
    this.outputConnectionStatusSubject.next(status);
  }

  setInputConnectionStatus(status: ConnectionStatus): void {
    this.inputConnectionStatusSubject.next(status);
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

  private applyFilterAndSort(): void {
    let list = [...this.mediaListSubject.value];

    // Filter by show
    const show = this.showSubject.value;
    if (show !== 'All') {
      switch (show) {
        case 'Unwatched':
          list = list.filter(m => !m.viewed || m.viewed === 0);
          break;
        case 'Watched':
          list = list.filter(m => m.viewed && m.viewed > 0);
          break;
      }
    }

    // Filter by text
    const filter = this.filterCriteriaSubject.value.toLowerCase();
    if (filter) {
      list = list.filter(m => m.name.toLowerCase().includes(filter));
    }

    // Filter by tags
    const tags = this.tagFilterCriteriaSubject.value;
    if (tags.length > 0) {
      list = list.filter(m => tags.some(t => m.tags?.includes(t)));
    }

    // Sort
    const sortBy = this.sortBySubject.value;
    list.sort((a, b) => {
      switch (sortBy) {
        case 'name':
          return a.name.localeCompare(b.name);
        case 'name-desc':
          return b.name.localeCompare(a.name);
        case 'date':
          return (b.metaData.dateAdded?.getTime() || 0) - (a.metaData.dateAdded?.getTime() || 0);
        case 'date-desc':
          return (a.metaData.dateAdded?.getTime() || 0) - (b.metaData.dateAdded?.getTime() || 0);
        default:
          return a.name.localeCompare(b.name);
      }
    });

    this.mediaListDisplayedSubject.next(list);
  }

  updateMediaItem(updated: MediaItem): void {
    const list = [...this.mediaListSubject.value];
    const index = list.findIndex(m => m.id === updated.id);
    if (index !== -1) {
      list[index] = updated;
      this.mediaListSubject.next(list);
      this.applyFilterAndSort();
    }
  }
}