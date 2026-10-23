import { Component, EventEmitter, input, Input, OnInit, Output, ChangeDetectionStrategy, TemplateRef, Inject } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { ClickStopPropagation } from '../../directives/stop-propegation';


@Component({
    selector: 'app-alert-modal',
    imports: [FormsModule, ClickStopPropagation],
    templateUrl: './alert.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './alert.component.css'
})
export class AlertComponent implements OnInit {
    content!: string;
    titleText: string = "Alert";
    acceptText: string = "Ok";
    rejectText: string | null = null;


    constructor(@Inject('overlayData') private data: any) { 
        if(data.content)
            this.content = data.content;
        if(data.titleText)
            this.titleText = data.titleText;
        if(data.acceptText)
            this.acceptText = data.acceptText;
        if(data.rejectText)
            this.rejectText = data.rejectText;
    }
    acceptEvent() {
        if(this.data.onAccept)
            this.data.onAccept();
        this.closeEvent();
    }
    rejectEvent() {
        if(this.data.onReject)
            this.data.onReject();
        this.closeEvent();
    }
    closeEvent() {
        if(this.data.onClose)
            this.data.onClose();
    }
    ngOnInit(): void { }
}
