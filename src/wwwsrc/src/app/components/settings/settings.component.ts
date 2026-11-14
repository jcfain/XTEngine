import { Component, Input, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { finalize, forkJoin } from 'rxjs';
import { ServerService } from '../../services/server.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { AlertComponent } from '../alert-model/alert.component';
import { SettingsLocalTabComponent } from './settings-local-tab.component';
import { SettingsConnectionsTabComponent } from './settings-connections-tab.component';
import { SettingsMotionTabComponent } from './settings-motion-tab.component';
import { SettingsSystemTabComponent } from './settings-system-tab.component';
import { OutputDeviceToolsComponent } from '../device-tools/output/output-device-tools.component';
import { WebsocketService } from '../../services/websocket.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { SaveState } from '../../models/enums';
import { AlertService } from '../../services/alert.service';

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
    hasLaunchPass: boolean | undefined = undefined;
    showMoreActions = false;

    private saveStateEnum: SaveState = SaveState.Idle;

    constructor(
        private localSettingsService: LocalSettingsService,
        private websocketService: WebsocketService,
        private serverService: ServerService, 
        private alertService: AlertService,
        private overlayService: GenericOverlayService) {
            this.serverService.remoteUserSettings$.subscribe(v => {
                this.hasLaunchPass = v?.hasLaunchPass;
            });
        }

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
                this.localSettingsService.reset()
            },
            onClose: () => this.overlayService.close(nameID)
        });
    }

    saveSuccess(): boolean {
        return this.saveStateEnum === SaveState.Success;
    }
    saveFail(): boolean {
        return this.saveStateEnum === SaveState.Fail;
    }

    restartXTP(): void {
        const nameID = 'restartXTPAlert';
        this.overlayService.open(nameID, AlertComponent, {
            content: `Are you sure you want restart XTP? 
${this.hasLaunchPass ? `PLEASE NOTE: XTP is protected by a launch password.
You will need to go to the pc and enter the password manually for now.` : ''}`,
            titleText: 'Confirm',
            acceptText: 'Restart XTP',
            rejectText: 'Cancel',
            onAccept: () => {
                this.websocketService.restartXTP();
            },
            onClose: () => this.overlayService.close(nameID)
        });
    }

    refreshXTPLibrary(): void {
        this.websocketService.refreshXTPLibrary();
    }

    markDirty(): void {
        this.formDirty = true;
    }

    onSaveToXTP(): void {
        this.saveState = 'Saving...';
        this.saveStateEnum = SaveState.Saving;
        forkJoin([
            this.serverService.saveServerSettings(),
            this.serverService.saveServerChannels()
        ]).pipe(
            finalize(() => {
               // Left for reference
            })
        ).subscribe({
            next: () => {
                this.saveStateEnum = SaveState.Success;
                this.formDirty = false;
                this.saveState = 'Saved';
                setTimeout(() => {
                    this.saveState = ''; 
                    this.saveStateEnum = SaveState.Idle;
                }, 2000);
            },
            error: (err) => {
                this.saveStateEnum = SaveState.Fail;
                console.error('Error saving settings:', err);
                this.saveState = 'Save failed';
            }
        });
    }
}