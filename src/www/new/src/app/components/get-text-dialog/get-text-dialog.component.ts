import { Component, Inject, ChangeDetectionStrategy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { ClickStopPropagation } from '../../directives/stop-propegation';

export interface GetTextDialogData {
    titleText?: string;
    message?: string;
    initialValue?: string;
    onAccept?: (value: string) => void;
    onClose?: () => void;
}

@Component({
    selector: 'app-get-text-dialog',
    imports: [FormsModule, ClickStopPropagation],
    templateUrl: './get-text-dialog.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './get-text-dialog.component.css'
})
export class GetTextDialogComponent {
    titleText = 'Enter value';
    message = '';
    value = '';

    constructor(@Inject('overlayData') private data: GetTextDialogData) {
        if (this.data.titleText) {
            this.titleText = this.data.titleText;
        }
        if (this.data.message) {
            this.message = this.data.message;
        }
        if (this.data.initialValue) {
            this.value = this.data.initialValue;
        }
    }

    acceptEvent(): void {
        if (this.data.onAccept) {
            this.data.onAccept(this.value);
        }
        this.closeEvent();
    }

    closeEvent(): void {
        if (this.data.onClose) {
            this.data.onClose();
        }
    }
}