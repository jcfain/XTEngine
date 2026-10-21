
import { Component, OnInit, ChangeDetectionStrategy } from "@angular/core";
import { FormsModule } from "@angular/forms";
import { MessageHandlerService } from "../../../services/message-handler.service";
import { AppStateService } from "../../../services/app-state.service";
import { WebsocketService } from "../../../services/websocket.service";

@Component({
    selector: 'output-device-tools',
    imports: [FormsModule],
    templateUrl: './output-device-tools.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './output-device-tools.component.css'
})
export class OutputDeviceToolsComponent implements OnInit {

  constructor(
    private messageHandlerService: MessageHandlerService,
    private websocketService: WebsocketService,
    private appState: AppStateService
  ) {}

  ngOnInit(): void {
    this.messageHandlerService.outputDeviceStatus$.subscribe(status => { 
      this.appState.setOutputConnectionStatus(status)
    });
    this.messageHandlerService.inputDeviceStatus$.subscribe(status => { 
      this.appState.setInputConnectionStatus(status)
    });
  }

  sendDeviceHome() {
    this.websocketService.sendDeviceHome();
  }

  togglePauseAllDeviceActions() {
    this.websocketService.togglePauseAllDeviceActions();
  }

  tcodeDeviceConnectRetry() {
    // this.websocketService.();

  }
}