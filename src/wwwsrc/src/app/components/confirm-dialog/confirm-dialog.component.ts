import { Component, ChangeDetectionStrategy } from '@angular/core';
import { AlertService, ConfirmDialogData } from '../../services/alert.service';
import { ClickStopPropagation } from '../../directives/stop-propegation';

/**
 * Renders the blocking confirm dialog driven by AlertService.confirm().
 * Cancel closes the dialog immediately and invokes the cancel callback;
 * Confirm invokes the confirm callback and leaves the dialog open until the
 * caller closes it via AlertService.closeConfirm().
 */
@Component({
    selector: 'app-confirm-dialog',
    imports: [ClickStopPropagation],
    templateUrl: './confirm-dialog.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './confirm-dialog.component.css'
})
export class ConfirmDialogComponent {
    dialog: ConfirmDialogData | null = null;

    constructor(private alertService: AlertService) {
        this.alertService.confirmDialog$.subscribe(dialog => this.dialog = dialog);
    }

    confirm(): void {
        this.alertService.confirmConfirm();
    }

    cancel(): void {
        this.alertService.cancelConfirm();
    }
}