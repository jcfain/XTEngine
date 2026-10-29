import { Component, Input, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { ServerService } from '../../services/server.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { AlertComponent } from '../alert-model/alert.component';
import { SettingsLocalTabComponent } from './settings-local-tab.component';
import { SettingsConnectionsTabComponent } from './settings-connections-tab.component';
import { SettingsMotionTabComponent } from './settings-motion-tab.component';
import { SettingsSystemTabComponent } from './settings-system-tab.component';
import { OutputDeviceToolsComponent } from '../device-tools/output/output-device-tools.component';

@Component({
    selector: 'app-settings',
    imports: [
        FormsModule,
        SettingsLocalTabComponent,
        SettingsConnectionsTabComponent,
        SettingsMotionTabComponent,
        SettingsSystemTabComponent,
        OutputDeviceToolsComponent
    ],
    templateUrl: './settings.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings.component.css'
})
export class SettingsComponent {
    @Input() isOpen = false;
    @Output() toggle = new EventEmitter<void>();

    formDirty = false;
    saveState = '';
    tabs = ['Local', 'Connections', 'Motion', 'System'];
    activeTab = 0;
    restartRequired = false;

    constructor(private serverService: ServerService, private overlayService: GenericOverlayService) {}

    onToggle(): void {
        this.toggle.emit();
    }

    tabClick(tabNumber: number): void {
        this.activeTab = tabNumber;
    }

    onRestartRequiredChange(required: boolean): void {
        this.restartRequired = required;
    }

    defaultLocalSettings(): void {
        const nameID = 'defaultLocalSettingsAlert';
        this.overlayService.open(nameID, AlertComponent, {
            content: 'Are you sure you want to reset ALL local settings to default? Note: This only resets the settings in this browser. Your settings stored in XTP will remain.',
            titleText: 'Warning',
            acceptText: 'Reset',
            rejectText: 'Cancel',
            onAccept: () => {
                window.localStorage.clear();
                window.location.reload();
            },
            onClose: () => this.overlayService.close(nameID)
        });
    }

    restartXTP(): void {
        this.serverService.restartXTP();
    }

    refreshXTPLibrary(): void {
        this.serverService.refreshXTPLibrary();
    }

    markDirty(): void {
        this.formDirty = true;
    }

    onSaveToXTP(): void {
        this.saveState = 'Saving...';
        this.serverService.saveServerSettings().subscribe({
            next: () => {
                this.formDirty = false;
                this.saveState = 'Saved';
                setTimeout(() => this.saveState = '', 2000);
            },
            error: (err) => {
                console.error('Error saving settings:', err);
                this.saveState = 'Save failed';
                setTimeout(() => this.saveState = '', 2000);
            }
        });
    }
}