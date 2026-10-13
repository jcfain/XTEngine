import { EventEmitter, Injectable, NgZone } from '@angular/core';
import { WebsocketService } from './websocket.service';
import { MediaItem } from '../models/interfaces';
import { XLogLevel, ConnectionStatus, ChannelTimeType } from '../models/enums';
import { BehaviorSubject, Observable, Subject } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class MessageHandlerService {

  private settingChangeSubject = new BehaviorSubject<{key: string, value: any} | null>(null);
  settingChange$: Observable<{key: string, value: any} | null> = this.settingChangeSubject.asObservable();

  private stopAllMediaEvent = new EventEmitter();
  stopAllMedia$: Observable<boolean> = this.stopAllMediaEvent.asObservable();

  private outputDeviceStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  outputDeviceStatusSubject$: Observable<ConnectionStatus> = this.outputDeviceStatusSubject.asObservable();

  private inputDeviceStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  inputDeviceStatusSubject$: Observable<ConnectionStatus> = this.inputDeviceStatusSubject.asObservable();

  private mediaLoadingStatusSubject = new BehaviorSubject<string | null>(null);
  mediaLoadingStatusSubject$: Observable<string | null> = this.mediaLoadingStatusSubject.asObservable();

  private updateThumbSubject = new BehaviorSubject<{id: string, thumb: string, errorMessage?: string } | null>(null);
  updateThumbSubject$: Observable<{id: string, thumb: string, errorMessage?: string } | null> = this.updateThumbSubject.asObservable();

  private updateItemSubject = new BehaviorSubject<{item: MediaItem, roles:number[]} | null>(null);
  updateItemSubject$: Observable<{item: MediaItem, roles:number[]} | null> = this.updateItemSubject.asObservable();

  private addItemSubject = new BehaviorSubject<MediaItem | null>(null);
  addItemSubject$: Observable<MediaItem | null> = this.addItemSubject.asObservable();

  private deleteItemSubject = new BehaviorSubject<number | null>(null);
  deleteItemSubject$: Observable<number | null> = this.deleteItemSubject.asObservable();

  private textToSpeechSubject = new BehaviorSubject<string | null>(null);
  textToSpeechSubject$: Observable<string | null> = this.textToSpeechSubject.asObservable();

  private skipToNextActionSubject = new BehaviorSubject<number | null>(null);
  skipToNextActionSubject$: Observable<number | null> = this.skipToNextActionSubject.asObservable();

  private channelProfileChangedSubject = new BehaviorSubject<string | null>(null);
  channelProfileChangedSubject$: Observable<string | null> = this.channelProfileChangedSubject.asObservable();

  private statusOutputSubject = new BehaviorSubject<{message: string, percentage:number} | null>(null);
  statusOutputSubject$: Observable<{message: string, percentage:number} | null> = this.statusOutputSubject.asObservable();

  private tagsUpdateSubject = new BehaviorSubject<{allTags: string[], smartTags:string[], userTags: string[]} | null>(null);
  tagsUpdateSubject$: Observable<{allTags: string[], smartTags:string[], userTags: string[]} | null> = this.tagsUpdateSubject.asObservable();

  private channelPositionChange = new BehaviorSubject<{ channel: string, position: number, time: number, timeType: ChannelTimeType } | null>(null);
  channelPositionChange$: Observable<{ channel: string, position: number, time: number, timeType: ChannelTimeType } | null> = this.channelPositionChange.asObservable();

  private metadataProcessingFinished = new Subject<void>();
  metadataProcessingFinished$: Observable<void> = this.metadataProcessingFinished.asObservable();

  private mediaLoadingEvent = new Subject<void>();
  mediaLoadingEvent$: Observable<void> = this.mediaLoadingEvent.asObservable();

  private skipToMoneyShotEvent = new Subject<void>();
  skipToMoneyShotEvent$: Observable<void> = this.skipToMoneyShotEvent.asObservable();

  private mediaLoadedEvent = new Subject<void>();
  mediaLoadedEvent$: Observable<void> = this.mediaLoadedEvent.asObservable();

  private scriptTogglePausedEvent = new Subject<void>();
  scriptTogglePausedEvent$: Observable<void> = this.scriptTogglePausedEvent.asObservable();

  private logoutEvent = new Subject<void>();
  logoutEvent$: Observable<void> = this.logoutEvent.asObservable();

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
          console.log('Settings exported:', obj['message'], obj['path'], obj['success']);
          break;
        }

        case 'settingsImported': {
          const obj = data['message'];
          console.log('Settings imported:', obj['message'], obj['path'], obj['success']);
          break;
        }

        case 'stopAllMedia':
          // this.videoService.stopVideo();
          this.stopAllMediaEvent.emit();
          break;

        case 'outputDeviceStatus': {
          const status = data['message'];
          // this.appState.setOutputConnectionStatus(status['status'] as ConnectionStatus);
          this.outputDeviceStatusSubject.next(status['status'] as ConnectionStatus);
          break;
        }

        case 'inputDeviceStatus': {
          const status = data['message'];
          // this.appState.setInputConnectionStatus(status['status'] as ConnectionStatus);
          this.inputDeviceStatusSubject.next(status['status'] as ConnectionStatus);
          break;
        }

        case 'mediaLoaded':
          // this.mediaService.onMediaLoaded();
          this.mediaLoadedEvent.next();
          break;

        case 'mediaLoading':
          // this.mediaService.onMediaLoading();
          this.mediaLoadingEvent.next();
          break;

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
          this.updateItemSubject.next({ "item": message['item'], "roles": message['roles'] });
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
          console.log('Script paused:', messageObj.isPaused);
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