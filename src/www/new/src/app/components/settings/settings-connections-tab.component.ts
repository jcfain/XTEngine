import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';
import { WebsocketService } from '../../services/websocket.service';
import { ConnectionInterface, ConnectionStatus, NetworkProtocol } from '../../models/enums';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';

@Component({
    selector: 'app-settings-connections-tab',
    imports: [FormsModule],
    templateUrl: './settings-connections-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-connections-tab.component.css'
})
export class SettingsConnectionsTabComponent implements OnInit {
    ConnectionInterface = ConnectionInterface;

    inputDevices: { type: ConnectionInterface; name: string }[] = [
        { type: ConnectionInterface.None, name: 'None' },
        { type: ConnectionInterface.HereSphere, name: 'DeoVR/HereSphere' },
        { type: ConnectionInterface.Whirligig, name: 'Whirligig' },
        { type: ConnectionInterface.XTPWeb, name: 'XTP Web (internal web player)' }
    ];

    outputDevices: { type: ConnectionInterface; name: string }[] = [
        { type: ConnectionInterface.Serial, name: 'Serial' },
        { type: ConnectionInterface.Network, name: 'Network' }
    ];

    selectedInputDevice = ConnectionInterface.None;
    selectedOutputDevice = ConnectionInterface.None;
    // inputStatus = ConnectionStatus.Disconnected;
    // outputStatus = ConnectionStatus.Disconnected;
    gamePadEnabled = false;
    deoVRAddress = '';
    deoVRPort = '';
    networkAddress = '';
    networkPort = '';
    serialPort = '';
    networkProtocolWebsocket = true;

    private inputDeviceStatus: {[key: number]: ConnectionStatus} = {};
    private outputDeviceStatus: {[key: number]: ConnectionStatus} = {};

    constructor(
        private appState: AppStateService,
        private localSettingsService: LocalSettingsService,
        private messageHandlerService: MessageHandlerService,
        private websocketService: WebsocketService
    ) {
    }

    ngOnInit(): void {
        this.inputDevices.forEach(x => {
            this.inputDeviceStatus[x.type] = ConnectionStatus.Disconnected;
        });
        this.outputDevices.forEach(x => {
            this.outputDeviceStatus[x.type] = ConnectionStatus.Disconnected;
        });
        this.messageHandlerService.inputDeviceStatus$.subscribe(s => this.inputDeviceStatus[s.type] = s.status);
        this.messageHandlerService.outputDeviceStatus$.subscribe(s => this.outputDeviceStatus[s.type] = s.status);
        this.localSettingsService.selectedInputDevice$.subscribe(s => this.selectedInputDevice = s);
        this.localSettingsService.selectedOuputDevice$.subscribe(s => this.selectedOutputDevice = s);

        this.appState.remoteUserSettings$.subscribe(v => {
            if (v?.connection) {
                const conn = v.connection;
                this.gamePadEnabled = conn.input?.gamePadEnabled ?? false;
                this.deoVRAddress = conn.input?.deoAddress || '';
                this.deoVRPort = conn.input?.deoPort || '';
                this.networkAddress = conn.output?.networkAddress || '';
                this.networkPort = conn.output?.networkPort || '';
                this.serialPort = conn.output?.serialPort || '';
                this.networkProtocolWebsocket = conn.output?.networkProtocol === NetworkProtocol.WEBSOCKET;
            }
        });
    }

    onInputDeviceChange(device: ConnectionInterface): void {
        this.selectedInputDevice = device;
        this.localSettingsService.setSelectedInputDevice(device);
        this.websocketService.sendInputDeviceConnectionChange(device, true);
    }

    onOutputDeviceChange(device: ConnectionInterface): void {
        this.selectedOutputDevice = device;
        this.localSettingsService.setSelectedOutputDevice(device);
        this.websocketService.sendOutputDeviceConnectionChange(device, true);
    }

    onGamePadChange(checked: boolean): void {
        this.gamePadEnabled = checked;
        this.websocketService.sendInputDeviceConnectionChange(ConnectionInterface.Gamepad, checked);
    }

    onDeoVRAddressChange(value: string): void {
        this.deoVRAddress = value;
        this.websocketService.settingChange('deoVRAddress', value);
    }

    onDeoVRPortChange(value: string): void {
        this.deoVRPort = value;
        this.websocketService.settingChange('deoVRPort', value);
    }

    onNetworkAddressChange(value: string): void {
        this.networkAddress = value;
        this.websocketService.settingChange('networkAddress', value);
    }

    onNetworkPortChange(value: string): void {
        this.networkPort = value;
        this.websocketService.settingChange('networkPort', value);
    }

    onSerialPortChange(value: string): void {
        this.serialPort = value;
        this.websocketService.settingChange('serialPort', value);
    }

    onNetworkProtocolChange(checked: boolean): void {
        this.networkProtocolWebsocket = checked;
        this.websocketService.settingChange('networkProtocol', checked ? NetworkProtocol.WEBSOCKET : NetworkProtocol.UDP);
    }

    statusColor(deviceType: ConnectionInterface): string {
        const status = this.getStatus(deviceType);
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

    statusText(deviceType: ConnectionInterface): string {
        const status = this.getStatus(deviceType);
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

    retryOutputConnection(): void {
        this.websocketService.tcodeDeviceConnectRetry(this.selectedOutputDevice);
    }

    retryInputConnection() {
        this.websocketService.sendInputDeviceConnectionChange(this.selectedInputDevice, true);
    }

    private getStatus(device: ConnectionInterface): ConnectionStatus {
        return this.inputDeviceStatus[device];
    }
}