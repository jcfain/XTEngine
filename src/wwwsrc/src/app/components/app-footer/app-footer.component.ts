import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { OutputDeviceToolsComponent } from '../device-tools/output/output-device-tools.component';

@Component({
    selector: 'app-footer',
    imports: [FormsModule, OutputDeviceToolsComponent],
    templateUrl: './app-footer.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './app-footer.component.css'
})
export class AppFooterComponent implements OnInit {

  constructor() {}

  ngOnInit(): void { }
}
