import { Component, Inject, ChangeDetectionStrategy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { ClickStopPropagation } from '../../directives/stop-propegation';

export interface SelectDialogOption {
    value: any;
    label: string;
}

export interface SelectDialogData {
    titleText?: string;
    message?: string;
    options: (string | SelectDialogOption)[];
    selectedValue?: any;
    onAccept?: (value: any) => void;
    onClose?: () => void;
}

@Component({
    selector: 'app-select-dialog',
    imports: [FormsModule, ClickStopPropagation],
    templateUrl: './select-dialog.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './select-dialog.component.css'
})
export class SelectDialogComponent {
    titleText = 'Select';
    message = 'Select value';
    options: SelectDialogOption[] = [];
    selected: any = null;

    constructor(@Inject('overlayData') private data: SelectDialogData) {
        if (this.data.titleText) {
            this.titleText = this.data.titleText;
        }
        if (this.data.message) {
            this.message = this.data.message;
        }
        this.options = (this.data.options || []).map(x =>
            typeof x === 'object' ? { value: x.value, label: x.label } : { value: x, label: x });
        if (this.options.length) {
            const match = this.data.selectedValue !== undefined
                ? this.options.find(o => o.value === this.data.selectedValue)
                : undefined;
            this.selected = (match || this.options[0]).value;
        }
    }

    selectOption(value: any): void {
        this.selected = value;
        if (this.data.onAccept) {
            this.data.onAccept(value);
        }
        this.closeEvent();
    }

    closeEvent(): void {
        if (this.data.onClose) {
            this.data.onClose();
        }
    }
}