import { Component, ChangeDetectionStrategy } from '@angular/core';
import { AlertService, AlertLevel, AlertMessage } from '../../services/alert.service';

@Component({
    selector: 'app-alerts',
    imports: [],
    templateUrl: './alerts.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './alerts.component.css'
})
export class AlertsComponent {
    readonly AlertLevel = AlertLevel;
    alerts: AlertMessage[] = [];

    private readonly levelNames: Record<AlertLevel, string> = {
        [AlertLevel.Info]: 'Info',
        [AlertLevel.Warning]: 'Warning',
        [AlertLevel.Debug]: 'Debug',
        [AlertLevel.Error]: 'Error'
    };

    constructor(private alertService: AlertService) {
        this.alertService.alerts$.subscribe(alerts => this.alerts = alerts);
    }

    levelName(level: AlertLevel): string {
        return this.levelNames[level];
    }

    levelClass(level: AlertLevel): string {
        return `alert-toast--${this.levelName(level).toLowerCase()}`;
    }

    dismiss(id: number): void {
        this.alertService.dismiss(id);
    }

    dismissAll(): void {
        this.alertService.dismissAll();
    }
}