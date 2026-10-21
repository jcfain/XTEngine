import { Component, EventEmitter, input, Input, OnInit, Output } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { ClickStopPropagation } from '../../../directives/stop-propegation';

@Component({
  selector: 'app-alert-modal',
  standalone: true,
  imports: [CommonModule, FormsModule, NgOptimizedImage, ClickStopPropagation],
  templateUrl: './alert.component.html',
  styleUrl: './alert.component.css'
})
export class AlertComponent implements OnInit {
    @Input() 
    content!: string;
    @Input() 
    titleText: string = "Confirm";
    @Input() 
    acceptText: string = "Yes";
    @Input() 
    rejectText: string = "No";

    @Output()
    accept = new EventEmitter();

    @Output()
    close = new EventEmitter();

    constructor( ) { }

    ngOnInit(): void { }

    acceptEvent() {
        this.accept.emit();
    }

    closeEvent() {
        this.close.emit();
    }
}