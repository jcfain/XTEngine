import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';
import { WebsocketService } from '../../services/websocket.service';
import { DeviceType, NetworkProtocol } from '../../models/enums';

@Component({
    selector: 'app-connections',
    imports: [FormsModule],
    templateUrl: './connections.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './connections.component.css'
})
export class ConnectionsComponent implements OnInit {
  inputDevices: { type: DeviceType; name: string }[] = [
    { type: DeviceType.Deo, name: 'Deo VR' },
    { type: DeviceType.Whirligig, name: 'Whirligig' },
    { type: DeviceType.XTPWeb, name: 'XTP Web' },
    { type: DeviceType.None, name: 'None' }
  ];

  outputDevices: { type: DeviceType; name: string }[] = [
    { type: DeviceType.None, name: 'None' },
    { type: DeviceType.Network, name: 'Network' },
    { type: DeviceType.Serial, name: 'Serial' }
  ];

  selectedInputDevice = DeviceType.None;
  selectedOutputDevice = DeviceType.None;
  deoVRAddress = '';
  deoVRPort = '';
  networkAddress = '';
  networkPort = '';
  serialPort = '';
  networkProtocolWebsocket = true;

  constructor(
    private appState: AppStateService,
    private serverService: ServerService,
    private websocketService: WebsocketService
  ) {}

  ngOnInit(): void {
    const settings = this.appState.remoteUserSettings;
    if (settings?.connection) {
      const conn = settings.connection as any;
      this.selectedInputDevice = conn.input?.selectedDevice ?? DeviceType.None;
      this.selectedOutputDevice = conn.output?.selectedDevice ?? DeviceType.None;
      this.deoVRAddress = conn.input?.deoAddress || '';
      this.deoVRPort = conn.input?.deoPort || '';
      this.networkAddress = conn.output?.networkAddress || '';
      this.networkPort = conn.output?.networkPort || '';
      this.serialPort = conn.output?.serialPort || '';
      this.networkProtocolWebsocket = conn.output?.networkProtocol === NetworkProtocol.WEBSOCKET;
    }
  }

  onInputDeviceChange(device: DeviceType): void {
    this.selectedInputDevice = device;
    this.appState.setSelectedInputDevice(device);
    this.websocketService.sendInputDeviceConnectionChange(device, true);
  }

  onOutputDeviceChange(device: DeviceType): void {
    this.selectedOutputDevice = device;
    this.appState.setSelectedOutputDevice(device);
    this.websocketService.sendOutputDeviceConnectionChange(device, true);
  }

  onDeoVRAddressChange(value: string): void {
    this.deoVRAddress = value;
    this.serverService.sendSettingChange('deoVRAddress', value);
  }

  onDeoVRPortChange(value: string): void {
    this.deoVRPort = value;
    this.serverService.sendSettingChange('deoVRPort', value);
  }

  onNetworkAddressChange(value: string): void {
    this.networkAddress = value;
    this.serverService.sendSettingChange('networkAddress', value);
  }

  onNetworkPortChange(value: string): void {
    this.networkPort = value;
    this.serverService.sendSettingChange('networkPort', value);
  }

  onSerialPortChange(value: string): void {
    this.serialPort = value;
    this.serverService.sendSettingChange('serialPort', value);
  }

  onNetworkProtocolChange(checked: boolean): void {
    this.networkProtocolWebsocket = checked;
    this.serverService.sendSettingChange('networkProtocol', checked ? NetworkProtocol.WEBSOCKET : NetworkProtocol.UDP);
  }

  retryConnection(): void {
    this.serverService.tcodeDeviceConnectRetry();
  }
}