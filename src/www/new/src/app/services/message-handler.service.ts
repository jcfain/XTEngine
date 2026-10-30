import { EventEmitter, Injectable, NgZone } from '@angular/core';
import { WebsocketService } from './websocket.service';
import { MediaItem } from '../models/interfaces';
import { XLogLevel, ConnectionStatus, ChannelTimeType, ConnectionInterface } from '../models/enums';
import { BehaviorSubject, Observable, Subject } from 'rxjs';

interface SettingChanged {key: string, value: any};
interface StatusOutput {message: string, percentage:number};
interface TagsChange {allTags: string[], smartTags:string[], userTags: string[]};
interface ChannelsPositionChange { channel: string, position: number, time: number, timeType: ChannelTimeType };
interface ThumbUpdate {id: string, thumb: string, errorMessage?: string };
interface MediaItemUpdate {item: MediaItem, roles:number[]};
interface SettingsBackupUpdate {message: string, path:string, success: boolean};

@Injectable({
  providedIn: 'root'
})
export class MessageHandlerService {

  private settingChangeSubject = new BehaviorSubject<SettingChanged | null>(null);
  settingChange$: Observable<SettingChanged | null> = this.settingChangeSubject.asObservable();

  private outputDeviceStatusSubject = new BehaviorSubject<{type: ConnectionInterface, status: ConnectionStatus}>({type: ConnectionInterface.None, status: ConnectionStatus.Disconnected});
  outputDeviceStatus$: Observable<{type: ConnectionInterface, status: ConnectionStatus}> = this.outputDeviceStatusSubject.asObservable();

  private inputDeviceStatusSubject = new BehaviorSubject<{type: ConnectionInterface, status: ConnectionStatus}>({type: ConnectionInterface.None, status: ConnectionStatus.Disconnected});
  inputDeviceStatus$: Observable<{type: ConnectionInterface, status: ConnectionStatus}> = this.inputDeviceStatusSubject.asObservable();

  private mediaLoadingStatusSubject = new BehaviorSubject<string | null>(null);
  /// Description of the current loading state
  mediaLoadingStatus$: Observable<string | null> = this.mediaLoadingStatusSubject.asObservable();

  // private updateThumbSubject = new BehaviorSubject<ThumbUpdate | null>(null);
  // updateThumb$: Observable<ThumbUpdate | null> = this.updateThumbSubject.asObservable();
  private updateThumbSubject = new Subject<ThumbUpdate | null>();
  updateThumb$: Observable<ThumbUpdate | null> = this.updateThumbSubject.asObservable();

  private updateMediaItemSubject = new BehaviorSubject<MediaItemUpdate | null>(null);
  updateMediaItem$: Observable<MediaItemUpdate | null> = this.updateMediaItemSubject.asObservable();

  private addItemSubject = new BehaviorSubject<MediaItem | null>(null);
  addItem$: Observable<MediaItem | null> = this.addItemSubject.asObservable();

  private deleteItemSubject = new BehaviorSubject<number | null>(null);
  deleteItem$: Observable<number | null> = this.deleteItemSubject.asObservable();

  private textToSpeechSubject = new BehaviorSubject<string | null>(null);
  textToSpeech$: Observable<string | null> = this.textToSpeechSubject.asObservable();

  private skipToNextActionSubject = new BehaviorSubject<number | null>(null);
  skipToNextAction$: Observable<number | null> = this.skipToNextActionSubject.asObservable();

  private channelProfileChangedSubject = new BehaviorSubject<string | null>(null);
  channelProfileChanged$: Observable<string | null> = this.channelProfileChangedSubject.asObservable();

  private statusOutputSubject = new BehaviorSubject<StatusOutput | null>(null);
  statusOutput$: Observable<StatusOutput | null> = this.statusOutputSubject.asObservable();

  private tagsUpdateSubject = new BehaviorSubject<TagsChange | null>(null);
  tagsUpdateSubject$: Observable<TagsChange | null> = this.tagsUpdateSubject.asObservable();

  private channelPositionChange = new BehaviorSubject<ChannelsPositionChange | null>(null);
  channelPositionChange$: Observable<ChannelsPositionChange | null> = this.channelPositionChange.asObservable();

  private scriptTogglePausedEvent = new BehaviorSubject<boolean>(false);
  scriptTogglePaused$: Observable<boolean> = this.scriptTogglePausedEvent.asObservable();

  private stopAllMediaSubject = new Subject<void>();
  stopAllMedia$: Observable<void> = this.stopAllMediaSubject.asObservable();

  private metadataProcessingFinished = new Subject<void>();
  metadataProcessingFinished$: Observable<void> = this.metadataProcessingFinished.asObservable();

  private mediaLoadingSubject = new Subject<void>();
  mediaLoading$: Observable<void> = this.mediaLoadingSubject.asObservable();

  private skipToMoneyShotEvent = new Subject<void>();
  skipToMoneyShot$: Observable<void> = this.skipToMoneyShotEvent.asObservable();

  private mediaLoadedEvent = new Subject<void>();
  mediaLoaded$: Observable<void> = this.mediaLoadedEvent.asObservable();

  private logoutEvent = new Subject<void>();
  logout$: Observable<void> = this.logoutEvent.asObservable();

  private settingsExportedEvent = new Subject<SettingsBackupUpdate>();
  settingsExported$: Observable<SettingsBackupUpdate> = this.settingsExportedEvent.asObservable();

  private settingsImportedEvent = new Subject<SettingsBackupUpdate>();
  settingsImported$: Observable<SettingsBackupUpdate> = this.settingsImportedEvent.asObservable();

  // private changePlayrateSubject  
  // // private mediaActionSubject
  
  private statusMessage = '';
  private statusPercentage = 0;

  constructor(
    private websocketService: WebsocketService,
    // private appState: AppStateService,
    // private mediaService: MediaService,
    // private mediaListService: MediaListService,
    // private serverService: ServerService,
    // private settingsService: SettingsService,
    // private videoService: VideoService,
    private ngZone: NgZone
  ) {
    this.websocketService.messages$.subscribe(message => {
      this.ngZone.run(() => this.handleMessage(message));
    });
  }

  private handleMessage(data: any): void {
    try {
      switch (data['command']) {
        case 'messageSend': {
          const obj = data['message'];
          const message = obj['message'].replaceAll('\n', '<br>');
          const level = obj['loglevel'];
          switch (level) {
            case XLogLevel.Critical:
              console.error(message);
              break;
            case XLogLevel.Information:
              console.log(message);
              break;
            case XLogLevel.Warning:
              console.warn(message);
              break;
          }
          break;
        }
        case 'userError':
        case 'userWarning':
        case 'systemError':
        case 'systemWarning':
          console.warn(data['command'] + ':', data['message'].replaceAll('\n', '<br>'));
          break;

        case 'settingChange': {
          const obj = data['message'];
          const key = obj['key'];
          const value = obj['value'];
          // this.settingsService.onSaved(key, value);
          this.settingChangeSubject.next({"key": key, "value": value});
          break;
        }

        case 'settingsExported': {
          const obj = data['message'];
          this.settingsExportedEvent.next(obj);
          console.log('Settings exported:', obj['message'], obj['path'], obj['success']);
          break;
        }

        case 'settingsImported': {
          const obj = data['message'];
          this.settingsImportedEvent.next(obj);
          console.log('Settings imported:', obj['message'], obj['path'], obj['success']);
          break;
        }

        case 'stopAllMedia':
          // this.videoService.stopVideo();
          this.stopAllMediaSubject.next();
          break;

        case 'outputDeviceStatus': {
          const message = data['message'];
				  var deviceType = message["deviceName"] as ConnectionInterface;
          var status = message['status'] as ConnectionStatus;
          this.outputDeviceStatusSubject.next({type: deviceType, status: status});
          break;
        }

        case 'inputDeviceStatus': {
          const message = data['message'];
				  var deviceType = message["deviceName"] as ConnectionInterface;
          var status = message['status'] as ConnectionStatus;
          this.inputDeviceStatusSubject.next({type: deviceType, status: status});
          break;
        }

        case 'mediaLoaded':
          // this.mediaService.onMediaLoaded();
          this.mediaLoadedEvent.next();
          break;

        case 'mediaLoading':
          // this.mediaService.onMediaLoading();
          this.mediaLoadingSubject.next();
          break;
        // Description of the current loading state
        case 'mediaLoadingStatus':
          // this.mediaService.onMediaLoadingStatus(data['message']);
          this.mediaLoadingStatusSubject.next(data['message']);
          break;

        case 'updateThumb':
          // this.mediaListService.onUpdateThumb(data['message']);
          this.updateThumbSubject.next(data['message']);
          break;

        case 'updateItem': {
          const message = data['message'];
          // this.messageListService.updateItem(message['item'], message['roles']);
          this.updateMediaItemSubject.next({ "item": message['item'], "roles": message['roles'] });
          break;
        }

        case 'addItem': {
          const message = data['message'];
          // this.messageListService.addItem(message['item']);
          this.addItemSubject.next(message['item']);
          break;
        }

        case 'deleteItem': {
          const message = data['message'];
          // this.messageListService.deleteItem(message['itemID']);
          this.deleteItemSubject.next(message['itemID']);
          break;
        }

        case 'textToSpeech': {
          const message = data['message'];
          this.onTextToSpeech(message);
          // this.onTextToSpeechSubject.next(message);
          break;
        }

        case 'skipToNextAction': {
          const message = data['message'];
          // this.videoService.skipTo(message);
          this.skipToNextActionSubject.next(message);
          break;
        }

        case 'skipToMoneyShot':
          // this.videoService.skipToMoneyShot();
          this.skipToMoneyShotEvent.next();
          break;

        case 'logout':
          window.location.replace(window.location.origin);
          break;

        case 'channelProfileChanged':
          const profile: string = data['message'];
          // this.serverService.getServerChannels();
          this.channelProfileChangedSubject.next(profile);
          break;

        case 'statusOutput': {
          const messageObj = data['message'];
          this.statusMessage = messageObj['message'];
          this.statusPercentage = messageObj['percentage'];
          break;
        }

        case 'tagsUpdate': {
          const messageObj = data['message'];
          const smartTags = messageObj['smartTags'];
          const userTags = messageObj['userTags'];
          // const settings = this.appState.remoteUserSettings;
          // if (settings) {
            // (settings as any)['allTags'] = [...userTags, ...smartTags];
            // (settings as any)['userTags'] = [...userTags];
            // (settings as any)['smartTags'] = [...smartTags];
            // this.appState.setRemoteUserSettings(settings);
            this.tagsUpdateSubject.next({"allTags": [...userTags, ...smartTags], "smartTags": [...smartTags], "userTags": [...userTags]})
          // }
          break;
        }

        case 'channelPositionChange': {
          const messageObj = data['message'];
          const channel = messageObj['channel'];
          const position = parseInt(messageObj['position']);
          const time = parseInt(messageObj['time']);
          const timeType = parseInt(messageObj['timeType']);
          // Forward to script status component via event or direct call
          console.log(`Channel ${channel}: ${position} in ${time}ms (type ${timeType})`);
          this.channelPositionChange.next({ "channel": channel, "position": position, "time": time, "timeType": timeType });
          break;
        }

        case 'mediaAction':
          break;

        case 'scriptTogglePaused': {
          const messageObj = data['message'];
          // console.log('Script paused:', messageObj.isPaused);
          this.scriptTogglePausedEvent.next(messageObj.isPaused);
          break;
        }

        case 'changePlayrate':
          break;

        case 'metadataProcessingFinished':
          // this.mediaService.loadMediaList();
          this.metadataProcessingFinished.next();
          break;
      }
    } catch (e: any) {
      console.error(e.toString());
    }
  }

  // private updateItem(item: MediaItem, roles: any): void {
  //   this.mediaListService.updateMediaItem(item);
  // }

  private onTextToSpeech(message: string): void {
    if ('speechSynthesis' in window) {
      const utterance = new SpeechSynthesisUtterance(message);
      window.speechSynthesis.speak(utterance);
    }
  }

  getStatusMessage(): string {
    return this.statusMessage;
  }

  getStatusPercentage(): number {
    return this.statusPercentage;
  }
}