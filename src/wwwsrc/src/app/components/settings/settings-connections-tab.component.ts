import { Component, ChangeDetectionStrategy, Output, EventEmitter, Input } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { ServerService } from '../../services/server.service';
import { WebsocketService } from '../../services/websocket.service';
import { ConnectionInterface, ConnectionStatus, NetworkProtocol } from '../../models/enums';
import { LocalSettingsService } from '../../services/local-settings.service';
import { DeviceConnectionService } from '../../services/device-connection.service';

@Component({
    selector: 'app-settings-connections-tab',
    imports: [FormsModule],
    templateUrl: './settings-connections-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-connections-tab.component.css'
})
export class SettingsConnectionsTabComponent {
    ConnectionInterface = ConnectionInterface;
    
    @Input()
    isVisible = false;
    @Output()
    onDirty = new EventEmitter();

    inputDevices: { type: ConnectionInterface; name: string }[] = [
        { type: ConnectionInterface.HereSphere, name: 'DeoVR/HereSphere' },
        { type: ConnectionInterface.Whirligig, name: 'Whirligig' },
        { type: ConnectionInterface.XTPWeb, name: 'XTP Web (internal web player)' }
    ];

    outputDevices: { type: ConnectionInterface; name: string }[] = [
        { type: ConnectionInterface.Serial, name: 'Serial' },
        { type: ConnectionInterface.Network, name: 'Network' }
    ];

    get selectedInputDevice(): ConnectionInterface {
        return this.deviceConnectionService.selectedInputDevice;
    }
    set selectedInputDevice(value: ConnectionInterface) {
        this.deviceConnectionService.selectedInputDevice = value;
    }
    get selectedOutputDevice(): ConnectionInterface {
        return this.deviceConnectionService.selectedOutputDevice;
    }
    set selectedOutputDevice(value: ConnectionInterface) {
        this.deviceConnectionService.selectedOutputDevice = value;
    }

    get inputDeviceStatus(): { [key: number]: ConnectionStatus } {
        return this.deviceConnectionService.inputDeviceStatus;
    }
    get outputDeviceStatus(): { [key: number]: ConnectionStatus } {
        return this.deviceConnectionService.outputDeviceStatus;
    }

    get settings() {
        return this.deviceConnectionService.settings;
    }

    get networkProtocolWebsocket(): boolean {
        return this.settings?.connection?.output?.networkProtocol === NetworkProtocol.WEBSOCKET;
    }
    set networkProtocolWebsocket(value: boolean) {
        let obj = this.settings?.connection?.output;
        if (obj)
            obj.networkProtocol = value ? NetworkProtocol.WEBSOCKET : NetworkProtocol.UDP;
    }

    constructor(
        private localSettingsService: LocalSettingsService,
        private websocketService: WebsocketService,
        private serverService: ServerService,
        private deviceConnectionService: DeviceConnectionService
    ) {}

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

    onInputDeviceChange(value: ConnectionInterface): void {
        this.localSettingsService.setSelectedInputDevice(value);
    }

    onOutputDeviceChange(value: ConnectionInterface): void {
        this.localSettingsService.setSelectedOutputDevice(value);
    }

    onGamePadChange(checked: boolean): void {
        this.deviceConnectionService.setGamepadEnabled(checked);
    }

    onDeoVRAddressChange(value: string): void {
        this.onDirty.emit();
    }

    onDeoVRPortChange(value: string): void {
        this.onDirty.emit();
    }

    onNetworkAddressChange(value: string): void {
        this.onDirty.emit();
    }

    onNetworkPortChange(value: string): void {
        this.onDirty.emit();
    }

    onSerialPortChange(value: string): void {
        this.onDirty.emit();
    }

    onNetworkProtocolChange(checked: boolean): void {
        this.onDirty.emit();
    }

    statusColor(deviceType: ConnectionInterface): string {
        const status = this.deviceConnectionService.getStatus(deviceType);
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
        const status = this.deviceConnectionService.getStatus(deviceType);
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
        const status = this.deviceConnectionService.getStatus(deviceType);
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

    get selectedOutputConnected(): boolean {
        if (this.selectedOutputDevice == ConnectionInterface.None)
            return false;
        const status = this.deviceConnectionService.getStatus(this.selectedOutputDevice);
        return status === ConnectionStatus.Connected || status === ConnectionStatus.Connecting;
    }

    retryOutputConnection(): void {
        this.websocketService.sendOutputDeviceConnectionChange(this.selectedOutputDevice, true);
    }

    retryInputConnection() {
        this.websocketService.sendInputDeviceConnectionChange(this.selectedInputDevice, true);
    }
}
