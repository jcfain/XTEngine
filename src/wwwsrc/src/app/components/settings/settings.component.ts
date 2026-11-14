import { Component, Input, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { finalize, forkJoin, Observable } from 'rxjs';
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

export enum Tabs {
    Local,
    Connections,
    Motion,
    System
}

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

    // formDirty = false;
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
        private alertService: AlertService) {
            this.serverService.remoteUserSettings$.subscribe(v => {
                this.hasLaunchPass = v?.hasLaunchPass;
            });
        }

    onClose(): void {
        if(!this.dirtyCheck(() => { this.toggle.emit(); })) {
            this.toggle.emit();
        }
    }

    tabClick(tabNumber: number): void {
        if(!this.dirtyCheck(() => { this.activeTab = tabNumber; })) {
            this.activeTab = tabNumber;
        }
//         if(this.isDirty()) {
//             const id = this.alertService.confirm(
//             {
//                 message: `You have unsaved changes, 
// Save them now?`, 
//                 header: "Warning!", 
//                 confirmText: "Save", 
//                 cancelText: "Discard",
//                 onConfirm: () => {
//                     this.onSaveToXTP(() => {
//                         this.activeTab = tabNumber;
//                         this.alertService.closeConfirm(id);
//                     });
//                 },
//                 onCancel: () => {
//                     if(this.activeTab == Tabs.Motion) {
//                         this.serverService.getServerChannels().add(() => {
//                             this.activeTab = tabNumber;
//                             this.markClean();
//                         });
//                     } else {
//                         this.serverService.initServerSettings().subscribe(v => {
//                             this.activeTab = tabNumber;
//                             this.markClean();
//                         });
//                     }
//                 }
//             }
//             //, $event 
//             );
//         } else {
//             this.activeTab = tabNumber;
//         }
    }

    onRestartRequiredChange(required: boolean): void {
        this.restartRequired = required;
    }

    defaultLocalSettings(): void {
        const id = this.alertService.confirm({
            message: 'Are you sure you want to reset ALL local settings to default? Note: This only resets the settings in this browser. Your settings stored in XTP will remain.',
            header: 'Warning',
            confirmText: 'Reset',
            cancelText: 'Cancel',
            onConfirm: () => {
                this.localSettingsService.reset()
            },
            onCancel: () => this.alertService.closeConfirm(id)
        });
    }

    saveSuccess(): boolean {
        return this.saveStateEnum === SaveState.Success;
    }
    saveFail(): boolean {
        return this.saveStateEnum === SaveState.Fail;
    }

    restartXTP(): void {
            const id = this.alertService.confirm({
            message: `Are you sure you want restart XTP? 
${this.hasLaunchPass ? `PLEASE NOTE: XTP is protected by a launch password.
You will need to go to the pc and enter the password manually for now.` : ''}`,
            header: 'Confirm',
            confirmText: 'Restart XTP',
            cancelText: 'Cancel',
            onConfirm: () => {
                this.websocketService.restartXTP();
            },
            onCancel: () => this.alertService.closeConfirm(id)
        });
    }

    refreshXTPLibrary(): void {
        this.websocketService.refreshXTPLibrary();
    }

    isDirty(): boolean {
        return this.serverService.remoteUserSettingsDirty;
        // this.formDirty = true;
    }
    markDirty(): void {
        this.serverService.remoteUserSettingsDirty = true;
        // this.formDirty = true;
    }
    markClean(): void {
        this.serverService.remoteUserSettingsDirty = false;
        // this.formDirty = false;
    }


    onSaveToXTP(onSucess?: Function, onFail?: Function): void {
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
                this.markClean();
                this.saveState = 'Saved';
                setTimeout(() => {
                    this.saveState = ''; 
                    this.saveStateEnum = SaveState.Idle;
                    if(onSucess)
                        onSucess();
                }, 2000);
            },
            error: (err) => {
                this.saveStateEnum = SaveState.Fail;
                console.error('Error saving settings:', err);
                this.saveState = 'Save failed';
                if(onFail)
                    onFail();
            }
        });
    }

    private dirtyCheck(onDone: Function): boolean {
        if(this.isDirty()) {
            const id = this.alertService.confirm(
            {
                message: `You have unsaved changes, 
Save them now?`, 
                header: "Warning!", 
                confirmText: "Save", 
                cancelText: "Discard",
                onConfirm: () => {
                    this.alertService.savingConfirm(id);
                    this.onSaveToXTP(() => {
                        onDone();
                        this.alertService.closeConfirm(id);
                    });
                },
                onCancel: () => {
                    this.alertService.savingConfirm(id);
                    if(this.activeTab == Tabs.Motion) {
                        this.serverService.getServerChannels().add(() => {
                            onDone();
                            this.markClean();
                        });
                    } else {
                        this.serverService.initServerSettings().subscribe(v => {
                            onDone();
                            this.markClean();
                        });
                    }
                }
            }
            //, $event 
            );
            return true;
        } else {
            return false;
        }
    }
}