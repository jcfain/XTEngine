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

/**
 * Global alert/toast service. Queues messages of various levels that are
 * rendered as a stacked, full-width toast overlay (see AlertsComponent).
 * Debug messages are only shown when enabled at runtime via setDebugEnabled().
 */
@Injectable({ providedIn: 'root' })
export class AlertService {
    private nextId = 0;
    private debugEnabled = false;

    private alertsSubject = new BehaviorSubject<AlertMessage[]>([]);
    alerts$: Observable<AlertMessage[]> = this.alertsSubject.asObservable();

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
    }
}