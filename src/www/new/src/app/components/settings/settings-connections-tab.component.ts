import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';
import { WebsocketService } from '../../services/websocket.service';
import { ConnectionStatus, DeviceType, NetworkProtocol } from '../../models/enums';

@Component({
    selector: 'app-settings-connections-tab',
    imports: [FormsModule],
    templateUrl: './settings-connections-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-connections-tab.component.css'
})
export class SettingsConnectionsTabComponent implements OnInit {
    DeviceType = DeviceType;

    inputDevices: { type: DeviceType; name: string }[] = [
        { type: DeviceType.None, name: 'None' },
        { type: DeviceType.Deo, name: 'DeoVR/HereSphere' },
        { type: DeviceType.Whirligig, name: 'Whirligig' },
        { type: DeviceType.XTPWeb, name: 'XTP Web (internal web player)' }
    ];

    outputDevices: { type: DeviceType; name: string }[] = [
        { type: DeviceType.Serial, name: 'Serial' },
        { type: DeviceType.Network, name: 'Network' }
    ];

    selectedInputDevice = DeviceType.None;
    selectedOutputDevice = DeviceType.None;
    gamePadEnabled = false;
    deoVRAddress = '';
    deoVRPort = '';
    networkAddress = '';
    networkPort = '';
    serialPort = '';
    networkProtocolWebsocket = true;
    inputStatus = ConnectionStatus.Disconnected;
    outputStatus = ConnectionStatus.Disconnected;

    constructor(
        private appState: AppStateService,
        private serverService: ServerService,
        private websocketService: WebsocketService
    ) {}

    ngOnInit(): void {
        this.appState.inputConnectionStatus$.subscribe(s => this.inputStatus = s);
        this.appState.outputConnectionStatus$.subscribe(s => this.outputStatus = s);

        const settings = this.appState.remoteUserSettings;
        if (settings?.connection) {
            const conn = settings.connection as any;
            this.selectedInputDevice = conn.input?.selectedDevice ?? DeviceType.None;
            this.selectedOutputDevice = conn.output?.selectedDevice ?? DeviceType.None;
            this.gamePadEnabled = conn.input?.gamePadEnabled ?? false;
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

    onGamePadChange(checked: boolean): void {
        this.gamePadEnabled = checked;
        this.websocketService.sendInputDeviceConnectionChange(DeviceType.Gamepad, checked);
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

    statusColor(status: ConnectionStatus): string {
        switch (status) {
            case ConnectionStatus.Connected:
                return 'green';
            case ConnectionStatus.Connecting:
                return 'yellow';
            case ConnectionStatus.Error:
                return 'orange';
            default:
                return 'crimson';
        }
    }

    statusText(status: ConnectionStatus): string {
        switch (status) {
            case ConnectionStatus.Connected:
                return 'Connected';
            case ConnectionStatus.Connecting:
                return 'Connecting';
            case ConnectionStatus.Error:
                return 'Error';
            default:
                return 'Disconnected';
        }
    }

    retryConnection(): void {
        this.serverService.tcodeDeviceConnectRetry();
    }
}