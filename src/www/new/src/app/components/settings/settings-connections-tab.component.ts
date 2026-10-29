import { Component, OnInit, ChangeDetectionStrategy, Output, EventEmitter } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';
import { WebsocketService } from '../../services/websocket.service';
import { ConnectionInterface, ConnectionStatus, NetworkProtocol } from '../../models/enums';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { RemoteUserSettings } from '../../models/interfaces';

@Component({
    selector: 'app-settings-connections-tab',
    imports: [FormsModule],
    templateUrl: './settings-connections-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-connections-tab.component.css'
})
export class SettingsConnectionsTabComponent implements OnInit {
    ConnectionInterface = ConnectionInterface;
    
    @Output()
    onDirty = new EventEmitter();

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

    public get gamePadEnabled(): boolean | undefined {
        return this.settings?.connection?.input?.gamePadEnabled;
    }
    public set gamePadEnabled(value: boolean) {
        let obj = this.settings?.connection?.input;
        if(obj)
            obj.gamePadEnabled = value;
    }
    public get deoVRAddress(): string | undefined {
        return this.settings?.connection?.input?.deoAddress;
    }
    public set deoVRAddress(value: string) {
        let obj = this.settings?.connection?.input;
        if(obj)
            obj.deoAddress = value;
    }
    public get deoVRPort(): string | undefined {
        return this.settings?.connection?.input?.deoPort;
    }
    public set deoVRPort(value: string) {
        let obj = this.settings?.connection?.input;
        if(obj)
            obj.deoPort = value;
    }
    public get networkAddress(): string | undefined {
        return this.settings?.connection?.output?.networkAddress;
    }
    public set networkAddress(value: string) {
        let obj = this.settings?.connection?.output;
        if(obj)
            obj.networkAddress = value;
    }
    public get networkPort(): string | undefined {
        return this.settings?.connection?.output?.networkPort;
    }
    public set networkPort(value: string) {
        let obj = this.settings?.connection?.output;
        if(obj)
            obj.networkPort = value;
    }
    public get serialPort(): string | undefined {
        return this.settings?.connection?.output?.serialPort;
    }
    public set serialPort(value: string) {
        let obj = this.settings?.connection?.output;
        if(obj)
            obj.serialPort = value;
    }
    public get networkProtocolWebsocket(): boolean | undefined {
        return this.settings?.connection?.output?.networkProtocol == NetworkProtocol.WEBSOCKET;
    }
    public set networkProtocolWebsocket(value: boolean) {
        let obj = this.settings?.connection?.output;
        if(obj)
            obj.networkProtocol = value ? NetworkProtocol.WEBSOCKET : NetworkProtocol.UDP;
    }

    private inputDeviceStatus: {[key: number]: ConnectionStatus} = {};
    private outputDeviceStatus: {[key: number]: ConnectionStatus} = {};
    private settings: RemoteUserSettings | null = null;

    constructor(
        private localSettingsService: LocalSettingsService,
        private serverService: ServerService,
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
        this.localSettingsService.selectedInputDevice$.subscribe(s => {
            this.selectedInputDevice = s
            this.websocketService.sendInputDeviceConnectionChange(s, true);
        });
        this.localSettingsService.selectedOuputDevice$.subscribe(s => {
            this.selectedOutputDevice = s;
            this.websocketService.sendOutputDeviceConnectionChange(s, true);
        });

        this.serverService.remoteUserSettings$.subscribe(v => {
            this.settings = v;
            // if (v?.connection) {
            //     const conn = v.connection;
            //     this.gamePadEnabled = conn.input?.gamePadEnabled ?? false;
            //     this.deoVRAddress = conn.input?.deoAddress || '';
            //     this.deoVRPort = conn.input?.deoPort || '';
            //     this.networkAddress = conn.output?.networkAddress || '';
            //     this.networkPort = conn.output?.networkPort || '';
            //     this.serialPort = conn.output?.serialPort || '';
            //     this.networkProtocolWebsocket = conn.output?.networkProtocol === NetworkProtocol.WEBSOCKET;
            // }
        });
    }

    onInputDeviceChange(device: ConnectionInterface): void {
        this.localSettingsService.setSelectedInputDevice(device);
    }

    onOutputDeviceChange(device: ConnectionInterface): void {
        this.localSettingsService.setSelectedOutputDevice(device);
    }

    onGamePadChange(checked: boolean): void {
        this.websocketService.sendInputDeviceConnectionChange(ConnectionInterface.Gamepad, checked);
    }

    onDeoVRAddressChange(value: string): void {
        this.onDirty.emit();
        // this.websocketService.settingChange('deoVRAddress', value);
    }

    onDeoVRPortChange(value: string): void {
        this.onDirty.emit();
        // this.websocketService.settingChange('deoVRPort', value);
    }

    onNetworkAddressChange(value: string): void {
        this.onDirty.emit();
        // this.websocketService.settingChange('networkAddress', value);
    }

    onNetworkPortChange(value: string): void {
        this.onDirty.emit();
        // this.websocketService.settingChange('networkPort', value);
    }

    onSerialPortChange(value: string): void {
        this.onDirty.emit();
        // this.websocketService.settingChange('serialPort', value);
    }

    onNetworkProtocolChange(checked: boolean): void {
        this.onDirty.emit();
        // this.websocketService.settingChange('networkProtocol', checked ? NetworkProtocol.WEBSOCKET : NetworkProtocol.UDP);
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

    statusImage(deviceType: ConnectionInterface): string {
        const status = this.getStatus(deviceType);
        switch(status) {
        case ConnectionStatus.Connected:
            return "://images/icons/check-mark-black.png";
        case ConnectionStatus.Connecting:
            return "://images/icons/reload.svg";
        case ConnectionStatus.Disconnected:
            return "://images/icons/x.svg";
        default:
            return "://images/icons/error-black.png";
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
        let status = this.inputDeviceStatus[device];
        if(status == undefined)
            status = this.outputDeviceStatus[device];
        return status;
    }
}