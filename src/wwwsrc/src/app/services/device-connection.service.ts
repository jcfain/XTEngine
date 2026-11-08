import { Injectable } from '@angular/core';
import { WebsocketService } from './websocket.service';
import { MessageHandlerService } from './message-handler.service';
import { ServerService } from './server.service';
import { LocalSettingsService } from './local-settings.service';
import { ConnectionInterface, ConnectionStatus } from '../models/enums';
import { RemoteUserSettings } from '../models/interfaces';

@Injectable({
  providedIn: 'root'
})
export class DeviceConnectionService {
  inputDeviceStatus: { [key: number]: ConnectionStatus } = {};
  outputDeviceStatus: { [key: number]: ConnectionStatus } = {};
  settings: RemoteUserSettings | undefined;

  selectedInputDevice = ConnectionInterface.None;
  selectedOutputDevice = ConnectionInterface.None;

  constructor(
    private websocketService: WebsocketService,
    private messageHandlerService: MessageHandlerService,
    private serverService: ServerService,
    private localSettingsService: LocalSettingsService
  ) {
    this.messageHandlerService.inputDeviceStatus$.subscribe(v => {
      if (v) {
        this.inputDeviceStatus[v.type] = v.status;
      }
    });
    this.messageHandlerService.outputDeviceStatus$.subscribe(v => {
      if (v) {
        this.outputDeviceStatus[v.type] = v.status;
      }
    });
    this.serverService.remoteUserSettings$.subscribe(v => {
      this.settings = v ?? undefined;
      if (v) {
        // On first launch local settings are None; seed them from remote settings.
        if (this.selectedInputDevice === ConnectionInterface.None && v.connection?.input?.selectedDevice) {
          this.localSettingsService.setSelectedInputDevice(v.connection.input.selectedDevice);
        }
        if (this.selectedOutputDevice === ConnectionInterface.None && v.connection?.output?.selectedDevice) {
          this.localSettingsService.setSelectedOutputDevice(v.connection.output.selectedDevice);
        }
        // this.checkAndUpdateInputConnection();
        // this.checkAndUpdateOutputConnection();
      }
    });
    this.localSettingsService.selectedInputDevice$.subscribe(s => {
      this.selectedInputDevice = s;
      if (this.initialized && s !== ConnectionInterface.None) {
        this.checkAndUpdateInputConnection(s);
      }
    });
    this.localSettingsService.selectedOuputDevice$.subscribe(s => {
      this.selectedOutputDevice = s;
      if (this.initialized && s !== ConnectionInterface.None) {
        this.checkAndUpdateOutputConnection(s);
      }
    });
  }
  private initialized = false;

  init() {
      if (this.selectedInputDevice !== ConnectionInterface.None) {
        this.checkAndUpdateInputConnection(this.selectedInputDevice);
      }
      if (this.selectedOutputDevice !== ConnectionInterface.None) {
        this.checkAndUpdateOutputConnection(this.selectedOutputDevice);
      }
      this.initialized = true;
  }

  checkAndUpdateInputConnection(value?: ConnectionInterface) {
    if (value !== undefined) {
      this.selectedInputDevice = value;
    } else {
      this.selectedInputDevice = this.settings?.connection.input.selectedDevice ?? ConnectionInterface.None;
    }
    const device = this.selectedInputDevice;
    if (device !== ConnectionInterface.None
      && this.getStatus(device) !== ConnectionStatus.Connected
      && this.getStatus(device) !== ConnectionStatus.Connecting) {
      this.websocketService.sendInputDeviceConnectionChange(device, true);
    }
  }

  checkAndUpdateOutputConnection(value?: ConnectionInterface) {
    if (value !== undefined) {
      this.selectedOutputDevice = value;
    } else {
      this.selectedOutputDevice = this.settings?.connection.output.selectedDevice ?? ConnectionInterface.None;
    }
    const device = this.selectedOutputDevice;
    if (device !== ConnectionInterface.None
      && this.getStatus(device) !== ConnectionStatus.Connected
      && this.getStatus(device) !== ConnectionStatus.Connecting) {
      this.websocketService.sendOutputDeviceConnectionChange(device, true);
    }
  }

  setGamepadEnabled(enabled: boolean) {
      this.websocketService.sendInputDeviceConnectionChange(ConnectionInterface.Gamepad, enabled);
  }

  getStatus(device: ConnectionInterface): ConnectionStatus | undefined {
    let status = this.inputDeviceStatus[device];
    if (status == undefined)
      status = this.outputDeviceStatus[device];
    return status;
  }
}