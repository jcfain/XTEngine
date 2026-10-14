import { Injectable, NgZone } from '@angular/core';
import { BehaviorSubject, Observable, Subject } from 'rxjs';
import { ConnectionStatus, DeviceType } from '../models/enums';
import { WebSocketMessage } from '../models/interfaces';

@Injectable({
  providedIn: 'root'
})
export class WebsocketService {

  private websocket: WebSocket | null = null;
  private wsUri: string = '';

  private connectionStatusSubject = new BehaviorSubject<ConnectionStatus>(ConnectionStatus.Disconnected);
  connectionStatus$: Observable<ConnectionStatus> = this.connectionStatusSubject.asObservable();

  private messageSubject = new Subject<WebSocketMessage>();
  messages$: Observable<WebSocketMessage> = this.messageSubject.asObservable();

  constructor(private ngZone: NgZone) {}

  get connected(): boolean {
    return this.connectionStatusSubject.value === ConnectionStatus.Connected;
  }

  statusText(): string {
    switch (this.connectionStatusSubject.value) {
      case ConnectionStatus.Connected: return 'Connected';
      case ConnectionStatus.Connecting: return 'Connecting...';
      case ConnectionStatus.Error: return 'Error';
      default: return 'Disconnected';
    }
  }

  connect(uri: string): void {
    this.wsUri = uri;
    this.connectionStatusSubject.next(ConnectionStatus.Connecting);

    this.websocket = new WebSocket(uri);

    this.websocket.onopen = () => {
      this.ngZone.run(() => {
        this.connectionStatusSubject.next(ConnectionStatus.Connected);
      });
    };

    this.websocket.onmessage = (evt: MessageEvent) => {
      this.ngZone.run(() => {
        try {
          const message: WebSocketMessage = JSON.parse(evt.data as string);
          this.messageSubject.next(message);
        } catch (e) {
          console.error('Failed to parse WebSocket message:', e);
        }
      });
    };

    this.websocket.onclose = () => {
      this.ngZone.run(() => {
        this.connectionStatusSubject.next(ConnectionStatus.Disconnected);
      });
    };

    this.websocket.onerror = () => {
      this.ngZone.run(() => {
        this.connectionStatusSubject.next(ConnectionStatus.Error);
      });
    };
  }

  disconnect(): void {
    if (this.websocket) {
      this.websocket.close();
      this.websocket = null;
      this.connectionStatusSubject.next(ConnectionStatus.Disconnected);
    }
  }

  send(command: string, payload?: any): void {
    if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
      let obj: WebSocketMessage;
      if (!payload) {
        obj = { "command": command }
      } else {
        obj = { "command": command, "message": payload }
      }
      // const message: WebSocketMessage = { command, ...payload };
      this.websocket.send(JSON.stringify(obj));
    }
  }

  // Convenience methods for common commands
  sendSystemReady(): void {
    this.send('systemReady');
  }

  sendMetadataProcess(): void {
    this.send('metadataProcess');
  }

  sendMetadataCleanProcess(): void {
    this.send('metadataCleanProcess');
  }

  sendCleanupThumbsProcess(): void {
    this.send('cleanupThumbsProcess');
  }

  sendClean1024Process(): void {
    this.send('clean1024Process');
  }

  sendTCode(tcode: string): void {
    this.send('tcode', { tcode });
  }

  sendTCodeRange(channelName: string, min: number, max: number): void {
    this.send('tcodeRange', { channelName, min, max });
  }

  sendMediaAction(action: string): void {
    this.send('mediaAction', { action });
  }

  sendInputDeviceConnectionChange(device: DeviceType, checked: boolean): void {
    this.send('inputDeviceConnectionChange', { device, checked });
  }

  sendOutputDeviceConnectionChange(device: DeviceType, checked: boolean): void {
    this.send('outputDeviceConnectionChange', { device, checked });
  }

  sendSkipToNextAction(): void {
    this.send('skipToNextAction');
  }

  sendSkipToMoneyShot(): void {
    this.send('skipToMoneyShot');
  }

  sendUpdateThumb(item: Record<string, any>): void {
    this.send('updateThumb', item);
  }

  sendDeleteMediaItem(item: Record<string, any>): void {
    this.send('deleteMediaItem', item);
  }

  sendUpdateMetadata(metadataKey: string): void {
    this.send('updateMetadata', { metadataKey });
  }

  sendQuickExport(): void {
    this.send('quickExport');
  }

  sendQuickImport(filename: string): void {
    this.send('quickImport', { filename });
  }

  setSelectedProfile(profileName: string): void {
    this.send('setSelectedProfile', { profileName });
  }

  addChannelProfile(profileName: string): void {
    this.send('addChannelProfile', { profileName });
  }

  deleteChannelProfile(profileName: string): void {
    this.send('deleteChannelProfile', { profileName });
  }

  cloneChannelProfile(fromName: string, toName: string): void {
    this.send('cloneChannelProfile', { fromName, toName });
  }

  sendDeviceHome(): void {
    this.send('deviceHome');
  }

  togglePauseAllDeviceActions(): void {
    this.send('togglePauseAllDeviceActions');
  }

  sendPlayMedia(obj: Record<string, any>): void {
    this.send('playMedia', obj);
  }

  sendStopMedia(): void {
    this.send('stopMedia');
  }

  sendSwapScript(scriptInfo: Record<string, any>): void {
    this.send('swapScript', scriptInfo);
  }

  settingChange(key: string, value: any): void {
    this.send('settingChange', { key, value });
  }

  sendPlaybackRate(value: number): void {
    this.send('setPlaybackRate', { value });
  }
}