import { Injectable, NgZone } from '@angular/core';
import { BehaviorSubject, Observable, Subject } from 'rxjs';
import { ConnectionInterface, ConnectionStatus, MediaActions } from '../models/enums';
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

  private mediaActions: Record<string, string> | undefined = undefined;

  // Keep-alive: send a ping every 30s so idle-timeout devices (proxies/firewalls/
  // NATs) don't drop the socket. Force a reconnect if no pong arrives in 90s.
  private keepAliveTimer: number | undefined;
  private lastPong = 0;
  private static readonly KEEP_ALIVE_INTERVAL_MS = 30000;
  private static readonly PONG_TIMEOUT_MS = 90000;

  constructor(private ngZone: NgZone) {}

  get connected(): boolean {
    return this.connectionStatusSubject.value === ConnectionStatus.Connected;
  }

  init(port: number): void {
    if(this.connectionStatusSubject.value == ConnectionStatus.Connected
        || this.connectionStatusSubject.value == ConnectionStatus.Connecting
    ) {
      return;
    }
    const wsUri = this.buildWebSocketUri(port);
    this.connect(wsUri);
  }

  setMediaActions(v: Record<string, string>) {
    this.mediaActions = v;
  }

  private buildWebSocketUri(port: number): string {
    const loc = window.location;
    const secure = loc.protocol === 'https:';
    // const scheme = secure ? 'wss' : 'ws';
    const scheme = 'ws';

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

  disconnect(): void {
    if (this.websocket) {
      this.stopKeepAlive();
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
      if(this.mediaActions && this.mediaActions[command]) {
        payload = command;
        command = 'mediaAction';
      }
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
    this.send('startMetadataProcess');
  }

  sendMetadataCleanProcess(): void {
    this.send('cleanupMetadata');
  }

  sendCleanupThumbsProcess(): void {
    this.send('cleanupThumbs');
  }

  sendClean1024Process(): void {
    this.send('clean1024');
  }

  sendTCode(tcode: string): void {
    this.send('tcode', tcode+"\n" );
  }

  sendTCodeRange(channelName: string, min: number, max: number): void {
    this.send('setChannelRange', { channelName, min, max });
  }

  // sendMediaAction(action: string): void {
  //   this.send('mediaAction', action );
  // }

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
    this.send('processMetadata', metadataKey);
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
    this.send(MediaActions.TCodeHomeAll);
  }

  togglePauseAllDeviceActions(): void {
    this.send(MediaActions.TogglePauseAllDeviceActions);
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

  private connect(uri: string): void {
    this.wsUri = uri;
    this.connectionStatusSubject.next(ConnectionStatus.Connecting);

    this.websocket = new WebSocket(uri);

    this.websocket.onopen = () => {
      this.ngZone.run(() => {
        this.connectionStatusSubject.next(ConnectionStatus.Connected);
      });
      this.lastPong = Date.now();
      this.startKeepAlive();
    };

    this.websocket.onmessage = (evt: MessageEvent) => {
      this.ngZone.run(() => {
        try {
          const message: WebSocketMessage = JSON.parse(evt.data as string);
          if (message.command === 'pong') {
            this.lastPong = Date.now();
            return;
          }
          this.messageSubject.next(message);
        } catch (e) {
          console.error('Failed to parse WebSocket message:', e);
        }
      });
    };

    this.websocket.onclose = () => {
      this.ngZone.run(() => {
        this.stopKeepAlive();
        this.connectionStatusSubject.next(ConnectionStatus.Disconnected);
      });
    };

    this.websocket.onerror = () => {
      this.ngZone.run(() => {
        this.connectionStatusSubject.next(ConnectionStatus.Error);
      });
    };
  }

  private startKeepAlive(): void {
    this.stopKeepAlive();
    this.keepAliveTimer = window.setInterval(() => {
      if (this.websocket && this.websocket.readyState === WebSocket.OPEN) {
        // Outgoing traffic also counts as activity for idle-timeout devices.
        this.websocket.send(JSON.stringify({ command: 'ping' }));
      }
      // If the server stopped responding, force a clean reconnect.
      if (Date.now() - this.lastPong > WebsocketService.PONG_TIMEOUT_MS) {
        console.warn('WebSocket pong timeout; forcing reconnect.');
        this.forceReconnect();
      }
    }, WebsocketService.KEEP_ALIVE_INTERVAL_MS);
  }

  private stopKeepAlive(): void {
    if (this.keepAliveTimer) {
      clearInterval(this.keepAliveTimer);
      this.keepAliveTimer = undefined;
    }
  }

  private forceReconnect(): void {
    this.stopKeepAlive();
    if (this.websocket) {
      // Closing fires onclose -> Disconnected, which drives the app's normal
      // ping-and-reconnect flow. Null it out so we don't touch a dead socket.
      this.websocket.close();
      this.websocket = null;
    }
  }
}