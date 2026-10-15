import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { OutputDeviceToolsComponent } from '../device-tools/output/output-device-tools.component';

@Component({
  selector: 'app-footer',
  standalone: true,
  imports: [CommonModule, FormsModule, OutputDeviceToolsComponent],
  templateUrl: './app-footer.component.html',
  styleUrl: './app-footer.component.css'
})
export class AppFooterComponent implements OnInit {

  constructor() {}

  ngOnInit(): void { }
}
