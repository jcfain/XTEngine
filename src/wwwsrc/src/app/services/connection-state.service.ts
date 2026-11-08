import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { ServerService } from './server.service';
import { WebsocketService } from './websocket.service';
import { AuthService } from './auth.service';
import { ConnectionStatus } from '../models/enums';
import { DeviceConnectionService } from './device-connection.service';

export enum ConnectionState {
  Disconnected = 'disconnected',
  Pinging = 'pinging',
  AwaitingAuth = 'awaiting-auth',
  Configured = 'configured',
  Connected = 'connected',
}

enum ConnectionEvent {
  Start,
  PongSuccess,
  PongFailure,
  SettingsLoaded,
  SettingsUnauthorized,
  Authenticated,
  WebSocketOpen,
  WebSocketClosed,
}

@Injectable({ providedIn: 'root' })
export class ConnectionStateService {
  private stateSubject = new BehaviorSubject<ConnectionState>(ConnectionState.Disconnected);
  state$: Observable<ConnectionState> = this.stateSubject.asObservable();

  private retryTimer: number | undefined;
  private started = false;
  private static readonly RETRY_DELAY_MS = 5000;

  get state(): ConnectionState {
    return this.stateSubject.value;
  }

  constructor(
    private serverService: ServerService,
    private websocketService: WebsocketService,
    private authService: AuthService
  ) {}

  start(): void {
    if (this.started) return;
    this.started = true;

    this.websocketService.connectionStatus$.subscribe(status => {
      if (status === ConnectionStatus.Connected) {
        this.handleEvent(ConnectionEvent.WebSocketOpen);
      } else if (status === ConnectionStatus.Disconnected || status === ConnectionStatus.Error) {
        this.handleEvent(ConnectionEvent.WebSocketClosed);
      }
    });

    this.authService.isAuthenticated$.subscribe(authed => {
      if (authed) {
        this.handleEvent(ConnectionEvent.Authenticated);
      }
    });

    this.handleEvent(ConnectionEvent.Start);
  }

  private handleEvent(event: ConnectionEvent): void {
    const state = this.state;

    switch (state) {
      case ConnectionState.Disconnected:
        if (event === ConnectionEvent.Start) {
          this.setState(ConnectionState.Pinging);
          this.doPing();
        }
        break;

      case ConnectionState.Pinging:
        switch (event) {
          case ConnectionEvent.PongSuccess:
            this.doFetchSettings();
            break;
          case ConnectionEvent.PongFailure:
            this.scheduleRetry();
            break;
          case ConnectionEvent.SettingsLoaded:
            this.setState(ConnectionState.Configured);
            this.doConnectWebSocket();
            break;
          case ConnectionEvent.SettingsUnauthorized:
            this.setState(ConnectionState.AwaitingAuth);
            // Safety: if auth already completed (e.g., cookie was set before
            // the 401 arrived), proceed immediately instead of waiting for
            // an Authenticated event that already fired.
            if (this.authService.isAuthenticated) {
              this.doFetchSettings();
            }
            break;
        }
        break;

      case ConnectionState.AwaitingAuth:
        if (event === ConnectionEvent.Authenticated) {
          this.doFetchSettings();
        } else if (event === ConnectionEvent.SettingsLoaded) {
          this.setState(ConnectionState.Configured);
          this.doConnectWebSocket();
        }
        break;

      case ConnectionState.Configured:
        if (event === ConnectionEvent.WebSocketOpen) {
          this.setState(ConnectionState.Connected);
        } else if (event === ConnectionEvent.WebSocketClosed) {
          this.setState(ConnectionState.Disconnected);
          this.handleEvent(ConnectionEvent.Start);
        }
        break;

      case ConnectionState.Connected:
        if (event === ConnectionEvent.WebSocketClosed) {
          this.setState(ConnectionState.Disconnected);
          this.handleEvent(ConnectionEvent.Start);
        }
        break;
    }
  }

  private doPing(): void {
    this.serverService.ping().subscribe({
      next: () => this.handleEvent(ConnectionEvent.PongSuccess),
      error: () => this.handleEvent(ConnectionEvent.PongFailure),
    });
  }

  private doFetchSettings(): void {
    this.serverService.initServerSettings().subscribe({
      next: () => this.handleEvent(ConnectionEvent.SettingsLoaded),
      error: (err: any) => {
        if (err?.status === 401) {
          this.handleEvent(ConnectionEvent.SettingsUnauthorized);
        } else {
          this.handleEvent(ConnectionEvent.PongFailure);
        }
      },
    });
  }

  private doConnectWebSocket(): void {
    const port = this.serverService.remoteUserSettings?.webSocketServerPort;
    if (port) {
      this.websocketService.init(port);
    }
  }

  private scheduleRetry(): void {
    if (this.retryTimer) clearTimeout(this.retryTimer);
    this.retryTimer = window.setTimeout(() => {
      this.retryTimer = undefined;
      this.doPing();
    }, ConnectionStateService.RETRY_DELAY_MS);
  }

  private setState(state: ConnectionState): void {
    this.stateSubject.next(state);
  }
}