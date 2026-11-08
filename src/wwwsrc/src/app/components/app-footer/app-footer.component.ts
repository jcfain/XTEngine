import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { OutputDeviceToolsComponent } from '../device-tools/output/output-device-tools.component';
import { MessageHandlerService } from '../../services/message-handler.service';
import { DecimalPipe } from '@angular/common';

@Component({
    selector: 'app-footer',
    imports: [FormsModule, OutputDeviceToolsComponent, DecimalPipe],
    templateUrl: './app-footer.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './app-footer.component.css'
})
export class AppFooterComponent implements OnInit {

  progressValue: number | undefined = undefined;
  progressMessage: string | undefined = undefined;

  constructor(private messageHandlerService: MessageHandlerService) {}

  ngOnInit(): void {
    this.messageHandlerService.statusOutput$.subscribe(v => {
      this.progressValue = v?.percentage;
      this.progressMessage = v?.message;
      // console.log(`${Date.now()}: ${this.progressValue}% ${this.progressMessage} `);
    })
   }
}
