import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';

export enum AlertLevel {
    Info = 0,
    Warning = 1,
    Debug = 2,
    Error = 3
}

export interface AlertMessage {
    id: number;
    level: AlertLevel;
    message: string;
}

export interface ConfirmDialogData {
    id: number;
    header: string;
    message: string;
    confirmText: string;
    cancelText: string;
    /** True while the caller is processing the confirm action (spinner shown, buttons disabled). */
    saving: boolean;
}

export interface ConfirmOptions {
    /** Dialog header. Defaults to "Confirm". */
    header?: string;
    /** Dialog message. Required. */
    message: string;
    /** Confirm button label. Defaults to "Confirm". */
    confirmText?: string;
    /** Cancel button label. Defaults to "Cancel". */
    cancelText?: string;
    /** Called when the user clicks confirm. The dialog stays open until the caller closes it via closeConfirm(). */
    onConfirm?: () => void;
    /** Called when the user clicks cancel. The dialog closes immediately. */
    onCancel?: () => void;
}

/**
 * Global alert/toast service. Queues messages of various levels that are
 * rendered as a stacked, full-width toast overlay (see AlertsComponent).
 * Debug messages are only shown when enabled at runtime via setDebugEnabled().
 * Also hosts a blocking confirm dialog (see ConfirmDialogComponent).
 */
@Injectable({ providedIn: 'root' })
export class AlertService {
    private nextId = 0;
    private debugEnabled = false;

    private alertsSubject = new BehaviorSubject<AlertMessage[]>([]);
    alerts$: Observable<AlertMessage[]> = this.alertsSubject.asObservable();

    private confirmDialogSubject = new BehaviorSubject<ConfirmDialogData | null>(null);
    confirmDialog$: Observable<ConfirmDialogData | null> = this.confirmDialogSubject.asObservable();

    private confirmCallbacks: { onConfirm?: () => void; onCancel?: () => void } = {};

    /** Enables/disables the display of debug level alerts at runtime. */
    setDebugEnabled(enabled: boolean): void {
        this.debugEnabled = enabled;
    }

    get isDebugEnabled(): boolean {
        return this.debugEnabled;
    }

    show(level: AlertLevel, message: string): void {
        if (level === AlertLevel.Debug && !this.debugEnabled) {
            return;
        }
        const alert: AlertMessage = { id: this.nextId++, level, message };
        this.alertsSubject.next([...this.alertsSubject.value, alert]);
    }

    info(message: string): void {
        this.show(AlertLevel.Info, message);
    }

    warning(message: string): void {
        this.show(AlertLevel.Warning, message);
    }

    debug(message: string): void {
        this.show(AlertLevel.Debug, message);
    }

    error(message: string): void {
        this.show(AlertLevel.Error, message);
    }

    dismiss(id: number): void {
        this.alertsSubject.next(this.alertsSubject.value.filter(a => a.id !== id));
    }

    dismissAll(): void {
        this.alertsSubject.next([]);
        this.cancelConfirm();
    }

    /**
     * Shows a blocking confirm dialog (see ConfirmDialogComponent).
     * @param options Dialog options. `message` is required; header defaults to
     *   "Confirm" and the buttons default to "Confirm"/"Cancel".
     * @returns The id of the confirm dialog, which can be passed to the close methods.
     */
    confirm(options: ConfirmOptions): number {
        if (!options.message) {
            throw new Error('AlertService.confirm() requires a message');
        }
        const id = this.nextId++;
        this.confirmCallbacks = { onConfirm: options.onConfirm, onCancel: options.onCancel };
        this.confirmDialogSubject.next({
            id,
            header: options.header ?? 'Confirm',
            message: options.message,
            confirmText: options.confirmText ?? 'Confirm',
            cancelText: options.cancelText ?? 'Cancel',
            saving: false
        });
        return id;
    }

    /**
     * Shows the saving spinner on the confirm button and disables both buttons
     * while the caller processes the confirm action.
     */
    savingConfirm(id?: number): void {
        this.setSaving(id, true);
    }

    /**
     * Stops the saving spinner and re-enables both buttons (e.g. when the
     * confirm action failed and the user should try again or cancel).
     */
    saveConfirmFail(id?: number): void {
        this.setSaving(id, false);
    }

    private setSaving(id: number | undefined, saving: boolean): void {
        const dialog = this.confirmDialogSubject.value;
        if (!dialog || (id !== undefined && dialog.id !== id) || (id !== undefined && id < 0)) {
            return;
        }
        if (dialog.saving === saving) {
            return;
        }
        this.confirmDialogSubject.next({ ...dialog, saving });
    }

    /**
     * Called by the confirm button. Invokes the onConfirm callback and leaves the
     * dialog open; the caller is expected to close it via closeConfirm() once done.
     */
    confirmConfirm(id?: number): void {
        const dialog = this.confirmDialogSubject.value;
        if (!dialog || (id !== undefined && dialog.id !== id) || (id !== undefined && id < 0)) {
            return;
        }
        const callbacks = this.confirmCallbacks;
        this.confirmCallbacks = {};
        if (callbacks.onConfirm) {
            callbacks.onConfirm();
        }
    }

    /**
     * Called by the cancel button. Closes the dialog immediately and invokes
     * the onCancel callback.
     */
    cancelConfirm(id?: number): void {
        const dialog = this.confirmDialogSubject.value;
        if (!dialog || (id !== undefined && dialog.id !== id) || (id !== undefined && id < 0)) {
            return;
        }
        const callbacks = this.confirmCallbacks;
        this.confirmCallbacks = {};
        this.confirmDialogSubject.next(null);
        if (callbacks.onCancel) {
            callbacks.onCancel();
        }
    }

    /**
     * Closes a confirm dialog after the user has confirmed (no callback is invoked).
     * Closing the dialog also stops the saving spinner and re-enables the buttons.
     */
    closeConfirm(id?: number): void {
        const dialog = this.confirmDialogSubject.value;
        if (!dialog || (id !== undefined && dialog.id !== id) || (id !== undefined && id < 0)) {
            return;
        }
        this.confirmCallbacks = {};
        this.confirmDialogSubject.next(null);
    }
}
