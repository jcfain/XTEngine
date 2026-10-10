import { Injectable, NgZone } from '@angular/core';
import { WebsocketService } from './websocket.service';
import { AppStateService } from './app-state.service';
import { MediaService } from './media.service';
import { ServerService } from './server.service';
import { SettingsService } from './settings.service';
import { VideoService } from './video.service';
import { ScriptStatusComponent } from '../components/script-status/script-status.component';
import { MediaItem } from '../models/interfaces';
import { XLogLevel, ConnectionStatus } from '../models/enums';

@Injectable({
  providedIn: 'root'
})
export class MessageHandlerService {
  private statusMessage = '';
  private statusPercentage = 0;

  constructor(
    private websocketService: WebsocketService,
    private appState: AppStateService,
    private mediaService: MediaService,
    private serverService: ServerService,
    private settingsService: SettingsService,
    private videoService: VideoService,
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
          this.settingsService.onSaved(key, value);
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
          this.mediaService.stopVideo();
          break;

        case 'outputDeviceStatus': {
          const status = data['message'];
          this.appState.setOutputConnectionStatus(status['status'] as ConnectionStatus);
          break;
        }

        case 'inputDeviceStatus': {
          const status = data['message'];
          this.appState.setInputConnectionStatus(status['status'] as ConnectionStatus);
          break;
        }

        case 'mediaLoaded':
          this.mediaService.onMediaLoaded();
          break;

        case 'mediaLoading':
          this.mediaService.onMediaLoading();
          break;

        case 'mediaLoadingStatus':
          this.mediaService.onMediaLoadingStatus(data['message']);
          break;

        case 'updateThumb':
          this.mediaService.onUpdateThumb(data['message']);
          break;

        case 'updateItem': {
          const message = data['message'];
          this.updateItem(message['item'], message['roles']);
          break;
        }

        case 'addItem': {
          const message = data['message'];
          this.addItem(message['item']);
          break;
        }

        case 'deleteItem': {
          const message = data['message'];
          this.deleteItem(message['itemID']);
          break;
        }

        case 'textToSpeech': {
          const message = data['message'];
          this.onTextToSpeech(message);
          break;
        }

        case 'skipToNextAction': {
          const message = data['message'];
          this.videoService.skipTo(message);
          break;
        }

        case 'skipToMoneyShot':
          this.mediaService.skipToMoneyShot();
          break;

        case 'logout':
          window.location.replace(window.location.origin);
          break;

        case 'channelProfileChanged':
          this.serverService.getServerChannels();
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
          const settings = this.appState.remoteUserSettings;
          if (settings) {
            (settings as any)['allTags'] = [...userTags, ...smartTags];
            (settings as any)['userTags'] = [...userTags];
            (settings as any)['smartTags'] = [...smartTags];
            this.appState.setRemoteUserSettings(settings);
          }
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
          this.mediaService.loadMediaList();
          break;
      }
    } catch (e: any) {
      console.error(e.toString());
    }
  }

  private updateItem(item: MediaItem, roles: any): void {
    this.appState.updateMediaItem(item);
  }

  private addItem(item: MediaItem): void {
    const list = [...this.appState.mediaList];
    if (!list.find(m => m.id === item.id)) {
      list.push(item);
      this.appState.setMediaList(list);
    }
  }

  private deleteItem(itemID: number): void {
    const list = this.appState.mediaList.filter(m => m.id !== itemID);
    this.appState.setMediaList(list);
  }

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