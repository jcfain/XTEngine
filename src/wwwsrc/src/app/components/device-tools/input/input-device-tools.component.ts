import { Component, OnInit, ChangeDetectionStrategy } from "@angular/core";
import { MessageHandlerService } from "../../../services/message-handler.service";
import { WebsocketService } from "../../../services/websocket.service";
import { ConnectionStatus, ConnectionInterface } from "../../../models/enums";
import { LocalSettingsService } from "../../../services/local-settings.service";

@Component({
    selector: 'input-device-tools',
    templateUrl: './input-device-tools.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './input-device-tools.component.css'
})
export class InputDeviceToolsComponent implements OnInit {

    // Exposed to the template so the @if can compare against the enum value.
    ConnectionInterface = ConnectionInterface;

    private selectedDevice: ConnectionInterface | undefined = undefined;
    // Status of every input device the server has reported, keyed by device
    // type. Reading the selected device's status from this map keeps the
    // button correct even when the selection changes between status pushes.
    private inputDeviceStatus: { [key: number]: ConnectionStatus } = {};

    constructor(
        private localSettingsService: LocalSettingsService,
        private messageHandlerService: MessageHandlerService,
        private websocketService: WebsocketService
    ) {}

    ngOnInit(): void {
        this.messageHandlerService.inputDeviceStatus$.subscribe(status => {
            if (status) {
                this.inputDeviceStatus[status.type] = status.status;
            }
        });
        this.localSettingsService.selectedInputDevice$.subscribe(v => {
            this.selectedDevice = v;
        });
    }

    get selectedInputDevice(): ConnectionInterface | undefined {
        return this.selectedDevice;
    }

    getDeviceName(): string {
        switch (this.selectedDevice) {
            case ConnectionInterface.HereSphere:
                return 'HereSphere';
            case ConnectionInterface.Whirligig:
                return 'Whirligig';
            case ConnectionInterface.XTPWeb:
                return 'XTPWeb';
            case ConnectionInterface.Gamepad:
                return 'Gamepad';
            default:
                return '';
        }
    }

    getDeviceStatus(): ConnectionStatus | undefined {
        if (this.selectedDevice === undefined || this.selectedDevice === ConnectionInterface.None) {
            return undefined;
        }
        return this.inputDeviceStatus[this.selectedDevice];
    }

    getGamepadStatus(): ConnectionStatus | undefined {
        return this.inputDeviceStatus[ConnectionInterface.Gamepad];
    }

    isDeviceConnected(): boolean {
        return this.getDeviceStatus() === ConnectionStatus.Connected;
    }
    isDeviceConnecting(): boolean {
        return this.getDeviceStatus() === ConnectionStatus.Connecting;
    }
    isDeviceError(): boolean {
        return this.getDeviceStatus() === ConnectionStatus.Error;
    }
    isDeviceDisconnected(): boolean {
        return this.getDeviceStatus() === ConnectionStatus.Disconnected
            || this.getDeviceStatus() === undefined;
    }

    // The gamepad button is only shown while the gamepad is active. The server
    // reports the gamepad as Connecting/Connected while it is enabled and
    // Disconnected when it is disabled, so the connection status is the reliable
    // (and reactive) "enabled" signal - the settings object is mutated in place
    // by the settings tab and does not re-emit.
    isGamepadActive(): boolean {
        const status = this.getGamepadStatus();
        return status === ConnectionStatus.Connected
            || status === ConnectionStatus.Connecting;
    }
    isGamepadConnected(): boolean {
        return this.getGamepadStatus() === ConnectionStatus.Connected;
    }
    isGamepadConnecting(): boolean {
        return this.getGamepadStatus() === ConnectionStatus.Connecting;
    }
    isGamepadError(): boolean {
        return this.getGamepadStatus() === ConnectionStatus.Error;
    }
    isGamepadDisconnected(): boolean {
        return this.getGamepadStatus() === ConnectionStatus.Disconnected
            || this.getGamepadStatus() === undefined;
    }

    // The device name is shown in the button tooltip (it used to be a separate
    // label, which forced the footer buttons to wrap on narrow screens).
    getStatusTitle(): string {
        const name = this.getDeviceName();
        switch (this.getDeviceStatus()) {
            case ConnectionStatus.Connected:
                return `${name} connected`;
            case ConnectionStatus.Connecting:
                return `${name} connecting`;
            case ConnectionStatus.Disconnected:
                return `${name} disconnected (click to connect)`;
            case ConnectionStatus.Error:
                return `${name} error`;
            default:
                return 'No input device selected';
        }
    }

    getGamepadStatusTitle(): string {
        switch (this.getGamepadStatus()) {
            case ConnectionStatus.Connected:
                return 'Gamepad connected';
            case ConnectionStatus.Connecting:
                return 'Gamepad connecting';
            case ConnectionStatus.Disconnected:
                return 'Gamepad disconnected';
            case ConnectionStatus.Error:
                return 'Gamepad error';
            default:
                return 'Gamepad';
        }
    }

    // Only a disconnected device triggers a connect; connecting, connected and
    // error states all ignore the click.
    inputDeviceConnectRetry(): void {
        if (this.selectedDevice === undefined || this.selectedDevice === ConnectionInterface.None) {
            return;
        }
        if (this.getDeviceStatus() !== ConnectionStatus.Disconnected) {
            return;
        }
        this.websocketService.sendInputDeviceConnectionChange(this.selectedDevice, true);
    }
}