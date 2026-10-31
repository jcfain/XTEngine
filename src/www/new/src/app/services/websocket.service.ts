import { Injectable, NgZone } from '@angular/core';
import { BehaviorSubject, Observable, Subject } from 'rxjs';
import { ConnectionInterface, ConnectionStatus } from '../models/enums';
import { ScriptInfo, WebSocketMessage } from '../models/interfaces';

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

  init(port: number): void {
    const wsUri = this.buildWebSocketUri(port);
    this.connect(wsUri);
  }

  private buildWebSocketUri(port: number): string {
    const loc = window.location;
    const secure = loc.protocol === 'https:';
    const scheme = secure ? 'wss' : 'ws';

    // Explicit override for unusual proxy setups.
    let override: string | null = null;
    try { override = localStorage.getItem('xtpWebSocketUrl'); } catch (e) {}
    if (override) return override;

    // Behind a reverse proxy the page is served from the proxy origin. The
    // backend WebSocket server is a separate plain (non-TLS) port that the
    // proxy does not expose to the client, so the upgrade must be routed
    // through the proxy on the same origin using a dedicated path.
    if (secure || loc.port === '' || loc.port === '80') {
      return `${scheme}://${loc.host}/ws`;
    }

    // Direct access to the HTTP server on a non-default port.
    return `${scheme}://${loc.hostname}:${port}`;
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

  restartXTP(): void {
    this.send('restartService');
  }

  refreshXTPLibrary(): void {
    this.send('reloadLibrary');
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
    this.send('tcode', tcode+"\n" );
  }

  sendTCodeRange(channelName: string, min: number, max: number): void {
    this.send('setChannelRange', { channelName, min, max });
  }

  sendMediaAction(action: string): void {
    this.send('mediaAction', action );
  }

  sendInputDeviceConnectionChange(device: ConnectionInterface, checked: boolean): void {
    this.send('connectInputDevice', { deviceName: device, enabled: checked });
  }

  sendOutputDeviceConnectionChange(device: ConnectionInterface, checked: boolean): void {
    this.send('connectOutputDevice', { deviceName: device, enabled: checked });
  }

  sendSkipToNextAction(): void {
    this.send('skipToNextAction');
  }

  sendSkipToMoneyShot(): void {
    this.send('skipToMoneyShot');
  }

  sendUpdateThumb(itemID: string): void {
    this.send('saveSingleThumb', { itemID: itemID, pos: 0 });
  }

  sendUpdateThumbAt(itemID: string, pos: number): void {
    this.send('saveSingleThumb', { itemID: itemID, pos: pos });
  }

  sendUpdateMoneyShotAt(itemID: string, pos: number) {
    this.send('setMoneyShot', { itemID: itemID, pos: pos });
  }

  sendDeleteMediaItem(itemID: string): void {
    this.send('deleteMediaItem', { itemID: itemID });
  }

  sendUpdateMetadata(metadataKey: string): void {
    this.send('updateMetadata', { metadataKey });
  }

  sendQuickExport(): void {
    this.send('settingsQuickExport');
  }

  sendQuickImport(filename: string): void {
    this.send('settingsQuickImport', { filename } );
  }

  setSelectedProfile(profileName: string): void {
    this.send('changeChannelProfile', profileName );
  }

  addChannelProfile(profileName: string): void {
    this.send('addChannelProfile', profileName );
  }

  deleteChannelProfile(profileName: string): void {
    this.send('deleteChannelProfile', profileName );
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

  tcodeDeviceConnectRetry(selectedDevice: ConnectionInterface) {
    this.send("connectOutputDevice", { deviceName: selectedDevice });
  }

  sendPlayMedia(itemID: string): void {
    this.send('playMedia', {id: itemID});
  }

  sendStopMedia(): void {
    this.send('stopMedia');
  }

  sendSwapScript(scriptInfo: ScriptInfo): void {
    this.send('swapScript', scriptInfo);
  }

  settingChange(key: string, value: any): void {
    this.send('settingChange', { key: key, value: value });
  }

  sendPlaybackRate(value: number): void {
    this.send('setPlaybackRate', value );
  }
}