
import { Component, OnInit, ChangeDetectionStrategy } from "@angular/core";
import { FormsModule } from "@angular/forms";
import { MessageHandlerService } from "../../../services/message-handler.service";
import { AppStateService } from "../../../services/app-state.service";
import { WebsocketService } from "../../../services/websocket.service";
import { ConnectionStatus, ConnectionInterface } from "../../../models/enums";
import { LocalSettingsService } from "../../../services/local-settings.service";

@Component({
    selector: 'output-device-tools',
    imports: [FormsModule],
    templateUrl: './output-device-tools.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './output-device-tools.component.css'
})
export class OutputDeviceToolsComponent implements OnInit {

  private selectedDevice: ConnectionInterface | undefined = undefined;
  private selectedDeviceAddress: string | undefined = undefined;
  private deviceConnectionStatus: ConnectionStatus | undefined = undefined;
  private deviceIsPaused: boolean = false;

  constructor(
    private localSettingsService: LocalSettingsService,
    private messageHandlerService: MessageHandlerService,
    private websocketService: WebsocketService,
    private appState: AppStateService
  ) {}

  ngOnInit(): void {
    this.messageHandlerService.outputDeviceStatus$.subscribe(status => { 
      this.deviceConnectionStatus = status.status;
    });
    this.messageHandlerService.scriptTogglePaused$.subscribe(v => {
      this.deviceIsPaused = v;
    });
    // this.messageHandlerService.inputDeviceStatus$.subscribe(status => { 
    //   this.appState.setInputConnectionStatus(status);
    // });
    this.appState.remoteUserSettings$.subscribe(v => {
      this.selectedDeviceAddress = v?.connection.output.networkAddress;
    });
    this.localSettingsService.selectedOuputDevice$.subscribe(v => {
      this.selectedDevice = v;
    })
  }

  getDeviceLinkHidden() : boolean {
    return this.selectedDevice !== ConnectionInterface.Network || !this.isDeviceConnected();
  }

  getDeviceLink() : string {
    if(!this.getDeviceLinkHidden() && this.selectedDeviceAddress) {
      return "http://"+this.selectedDeviceAddress;
    }
    return "";
  }

  isDeviceConnected() {
    return this.deviceConnectionStatus === ConnectionStatus.Connected;
  }
  isDeviceConnecting() {
    return this.deviceConnectionStatus === ConnectionStatus.Connecting;
  }
  isDeviceError() {
    return this.deviceConnectionStatus === ConnectionStatus.Error;
  }

  getStatusImage(): string {
    switch(this.deviceConnectionStatus) {
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

  getStatusTitle(): string {
    switch(this.deviceConnectionStatus) {
      case ConnectionStatus.Connected:
        return "Connected";
      case ConnectionStatus.Connecting:
        return "Connecting";
      case ConnectionStatus.Disconnected:
        return "TCode device status (Click to connect)";
      default:
        return "Error: Check your devices connection and click this to retry";
    }
  }

  getScriptPausedStatusImage() {
    if(this.deviceIsPaused)
      return "://images/icons/play.svg";
    else
    return "://images/icons/pause.svg";
  }

  sendDeviceHome() {
    this.websocketService.sendDeviceHome();
  }

  togglePauseAllDeviceActions() {
    this.websocketService.togglePauseAllDeviceActions();
  }

  tcodeDeviceConnectRetry() {
    if(this.selectedDevice === ConnectionInterface.None || this.selectedDevice === undefined)
      return;
    this.websocketService.tcodeDeviceConnectRetry(this.selectedDevice);
  }
}