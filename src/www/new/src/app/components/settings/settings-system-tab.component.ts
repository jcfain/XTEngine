import { Component, OnInit, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { SettingsService } from '../../services/settings.service';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { GetTextDialogComponent } from '../get-text-dialog/get-text-dialog.component';
import { AlertComponent } from '../alert-model/alert.component';
import { Setting, TCodeCommand } from '../../models/interfaces';
import { FormControlType } from '../../models/enums';
import { capitalizeFirstLetter } from '../../utils/utils';

interface SettingGroup {
    name: string;
    settings: Setting[];
}

interface ExportedFile {
    name: string;
    date: string;
}

@Component({
    selector: 'app-settings-system-tab',
    imports: [FormsModule],
    templateUrl: './settings-system-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-system-tab.component.css'
})
export class SettingsSystemTabComponent implements OnInit {
    @Output()
    onDirty = new EventEmitter();
    @Output() 
    restartRequiredChange = new EventEmitter<boolean>();

    groups: SettingGroup[] = [];
    xteVersion = '';
    webVersion = 'v0.6.6b';
    userAgent = navigator.userAgent;
    userAgentIsDeo = false;
    userAgentIsHereSphere = false;
    userAgentIsMobile = false;
    browserName = '';
    browserFullVersion = '';
    browserMajorVersion = 0;

    userTags: string[] = [];
    smartTags: string[] = [];
    selectedUserTags: string[] = [];
    selectedSmartTags: string[] = [];
    tagsDirty = false;

    exportedFiles: ExportedFile[] = [];

    tcodeCommands: TCodeCommand[] = [];
    tcodeDirty = false;

    private addTagTextNameID = 'systemAddTagText';
    private tagAlertNameID = 'systemTagAlert';
    private confirmAlertNameID = 'systemConfirmAlert';

    private debounceers: { [id: string]: any } = {};
    private debounceerTimeout = 1000;

    constructor(
        private appState: AppStateService,
        private settingsService: SettingsService,
        private serverService: ServerService,
        private overlayService: GenericOverlayService
    ) {}

    ngOnInit(): void {
        this.getBrowserInformation();

        this.serverService.remoteUserSettings$.subscribe(settings => {
            if (settings) {
                this.xteVersion = settings.xteVersion || '';
                this.userTags = settings.userTags || [];
                this.smartTags = settings.smartTags || [];
            }
        });
        this.userAgentIsDeo = this.appState.userAgentIsDeo;
        this.userAgentIsHereSphere = this.appState.userAgentIsHereSphere;
        this.userAgentIsMobile = this.appState.userAgentIsMobile;

        this.settingsService.settings$.subscribe(settings => {
            const groupMap = new Map<string, Setting[]>();
            settings.forEach(s => {
                if (!groupMap.has(s.group)) {
                    groupMap.set(s.group, []);
                }
                groupMap.get(s.group)!.push(s);
            });
            this.groups = Array.from(groupMap.entries()).map(([name, settings]) => ({
                name,
                settings
            }));
        });

        this.serverService.tcodeCommands$.subscribe(commands => {
            this.tcodeCommands = commands as TCodeCommand[];
        });

        this.loadExportedFiles();
    }

    getBrowserInformation(): void {
        const nAgt = navigator.userAgent;
        let browserName = '';
        let fullVersion = '';
        let majorVersion = 0;
        let nameOffset, verOffset, ix;

        // In Opera, the true version is after "Opera" or after "Version"
        if ((verOffset = nAgt.indexOf("Opera")) != -1) {
            browserName = "Opera";
            fullVersion = nAgt.substring(verOffset + 6);
            if ((verOffset = nAgt.indexOf("Version")) != -1)
                fullVersion = nAgt.substring(verOffset + 8);
        }
        // In MSIE, the true version is after "MSIE" in userAgent
        else if ((verOffset = nAgt.indexOf("MSIE")) != -1) {
            browserName = "Microsoft Internet Explorer";
            fullVersion = nAgt.substring(verOffset + 5);
        }
        // In Chrome, the true version is after "Chrome"
        else if ((verOffset = nAgt.indexOf("Chrome")) != -1) {
            browserName = "Chrome";
            fullVersion = nAgt.substring(verOffset + 7);
        }
        // In Safari, the true version is after "Safari" or after "Version"
        else if ((verOffset = nAgt.indexOf("Safari")) != -1) {
            browserName = "Safari";
            fullVersion = nAgt.substring(verOffset + 7);
            if ((verOffset = nAgt.indexOf("Version")) != -1)
                fullVersion = nAgt.substring(verOffset + 8);
        }
        // In Firefox, the true version is after "Firefox"
        else if ((verOffset = nAgt.indexOf("Firefox")) != -1) {
            browserName = "Firefox";
            fullVersion = nAgt.substring(verOffset + 8);
        }
        // In most other browsers, "name/version" is at the end of userAgent
        else if ((nameOffset = nAgt.lastIndexOf(' ') + 1) < (verOffset = nAgt.lastIndexOf('/'))) {
            browserName = nAgt.substring(nameOffset, verOffset);
            fullVersion = nAgt.substring(verOffset + 1);
            if (browserName.toLowerCase() == browserName.toUpperCase()) {
                browserName = navigator.appName;
            }
        }
        // trim the fullVersion string at semicolon/space if present
        if ((ix = fullVersion.indexOf(";")) != -1)
            fullVersion = fullVersion.substring(0, ix);
        if ((ix = fullVersion.indexOf(" ")) != -1)
            fullVersion = fullVersion.substring(0, ix);

        majorVersion = parseInt('' + fullVersion, 10);
        if (isNaN(majorVersion)) {
            fullVersion = '' + parseFloat(navigator.appVersion);
            majorVersion = parseInt(navigator.appVersion, 10);
        }

        this.browserName = browserName;
        this.browserFullVersion = fullVersion;
        this.browserMajorVersion = majorVersion;
    }

    getControlType(type: FormControlType): string {
        switch (type) {
            case FormControlType.Double:
            case FormControlType.Int:
                return 'number';
            case FormControlType.Text:
                return 'text';
            case FormControlType.Radio:
                return 'radio';
            case FormControlType.Combo:
                return 'select';
            case FormControlType.Checkbox:
                return 'checkbox';
            case FormControlType.DateTime:
                return 'datetime-local';
            case FormControlType.Date:
                return 'date';
            case FormControlType.Time:
                return 'time';
            default:
                return 'text';
        }
    }

    getControlProps(setting: Setting): { min?: string; max?: string; step?: string } {
        switch (setting.key) {
            case 'playbackRateStep':
                return { min: '0.0', step: '0.01' };
            case 'httpChunkSizeMB':
                return { min: '0.5' };
            case 'scheduleLibraryLoadTime':
                return { min: '00:00', max: '23:59' };
            case 'viewedThreshold':
                return { min: '1', max: '99', step: '1' };
            default:
                return {};
        }
    }

    onValueChange(setting: Setting, value: any): void {
        // Keep the local remoteUserSettings in sync so a full save stays consistent
        setting.value = value;
        if (this.debounceers[setting.key]) {
            clearTimeout(this.debounceers[setting.key]);
        }
        this.debounceers[setting.key] = setTimeout(() => {
            this.debounceers[setting.key] = undefined;
            this.settingsService.changeSetting(setting.key, value);
            if (setting.requiresRestart) {
                this.restartRequiredChange.emit(true);
            }
        }, this.debounceerTimeout);
    }

    capitalizeFirstLetter(val: string): string {
        return capitalizeFirstLetter(val);
    }

    // Backup/Restore
    loadExportedFiles(): void {
        this.serverService.getExported().subscribe({
            next: (files) => {
                const fileList: ExportedFile[] = files || [];
                this.exportedFiles = fileList.sort((a, b) => (a.date > b.date) ? -1 : ((a.date < b.date) ? 1 : 0));
            },
            error: () => {
                this.exportedFiles = [];
            }
        });
    }

    quickExport(): void {
        this.serverService.sendQuickExport();
        setTimeout(() => this.loadExportedFiles(), 1000);
    }

    restoreExported(filename: string): void {
        this.overlayService.open(this.confirmAlertNameID, AlertComponent, {
            content: `Are you sure you want to restore the settings from: ${filename}?`,
            titleText: 'Restore',
            acceptText: 'Restore',
            rejectText: 'Cancel',
            onAccept: () => this.serverService.sendQuickImport(filename),
            onClose: () => this.overlayService.close(this.confirmAlertNameID)
        });
    }

    deleteExported(filename: string): void {
        this.overlayService.open(this.confirmAlertNameID, AlertComponent, {
            content: `Are you sure you want to delete: ${filename}?`,
            titleText: 'Delete',
            acceptText: 'Delete',
            rejectText: 'Cancel',
            onAccept: () => {
                this.serverService.deleteExported(filename).subscribe({
                    next: () => this.loadExportedFiles(),
                    error: () => { }
                });
            },
            onClose: () => this.overlayService.close(this.confirmAlertNameID)
        });
    }

    // Media Library tags
    onUserTagsSelectChange(tags: string[]): void {
        this.selectedUserTags = tags;
    }

    onSmartTagsSelectChange(tags: string[]): void {
        this.selectedSmartTags = tags;
    }

    addTag(smartMode: boolean): void {
        this.overlayService.open(this.addTagTextNameID, GetTextDialogComponent, {
            titleText: smartMode ? 'Add smart tag' : 'Add tag',
            message: 'Name',
            onAccept: (value: string) => this.systemTagAdded(value, smartMode),
            onClose: () => this.overlayService.close(this.addTagTextNameID)
        }, undefined, true);
    }

    systemTagAdded(name: string, smartMode: boolean): void {
        if (!name || !name.trim().length) {
            this.showTagAlert('Tag cant be empty!');
            return;
        }
        const tags = smartMode ? this.smartTags : this.userTags;
        if (tags.findIndex(x => x == name) > -1) {
            this.showTagAlert(`Tag '${name}' already exists!`);
            return;
        }
        const other = smartMode ? this.userTags : this.smartTags;
        const otherTagName = smartMode ? 'userTags' : 'smartTags';
        if (other.findIndex(x => x == name) > -1) {
            this.showTagAlert(`Tag '${name}' already exists in ${otherTagName}!`);
            return;
        }
        const settings = this.serverService.remoteUserSettings;
        if (!settings) return;
        const settingMemberName = smartMode ? 'smartTagsToAdd' : 'tagsToAdd';
        const list = settings[settingMemberName] ?? (settings[settingMemberName] = []);
        list.push(name);
        tags.push(name);
        this.tagsDirty = true;
    }

    removeTag(smartMode: boolean): void {
        const settings = this.serverService.remoteUserSettings;
        if (!settings) return;
        const selected = smartMode ? this.selectedSmartTags : this.selectedUserTags;
        const tags = smartMode ? this.smartTags : this.userTags;
        const settingMemberName = smartMode ? 'smartTagsToRemove' : 'tagsToRemove';
        const list = settings[settingMemberName] ?? (settings[settingMemberName] = []);
        selected.forEach(tag => {
            list.push(tag);
            const index = tags.findIndex(x => x == tag);
            if (index > -1) tags.splice(index, 1);
        });
        this.selectedUserTags = [];
        this.selectedSmartTags = [];
        this.tagsDirty = true;
    }

    saveTags(): void {
        const settings = this.serverService.remoteUserSettings;
        if (!settings) return;
        this.serverService.saveServerSettings().subscribe({
            next: () => {
                settings.tagsToAdd = [];
                settings.tagsToRemove = [];
                settings.smartTagsToAdd = [];
                settings.smartTagsToRemove = [];
                this.tagsDirty = false;
            },
            error: () => { }
        });
    }

    showTagAlert(message: string): void {
        this.overlayService.open(this.tagAlertNameID, AlertComponent, {
            content: message,
            titleText: 'Warning',
            onClose: () => this.overlayService.close(this.tagAlertNameID)
        });
    }

    // Media Library footer actions
    processMetadata(): void {
        this.overlayService.open(this.confirmAlertNameID, AlertComponent, {
            content: 'This will process the metadata for all media items. Continue?',
            titleText: 'Process metadata',
            acceptText: 'Process',
            rejectText: 'Cancel',
            onAccept: () => this.serverService.startMetadataProcess(),
            onClose: () => this.overlayService.close(this.confirmAlertNameID)
        });
    }

    cleanMetadata(): void {
        this.overlayService.open(this.confirmAlertNameID, AlertComponent, {
            content: 'This will compare the metadata with the currently loaded libraries and delete any that do not exist any more.\nDO NOT RUN unless you have all your media currently loaded!',
            titleText: 'Clean metadata',
            acceptText: 'Clean',
            rejectText: 'Cancel',
            onAccept: () => this.serverService.startMetadataCleanProcess(),
            onClose: () => this.overlayService.close(this.confirmAlertNameID)
        });
    }

    cleanThumbs(): void {
        this.overlayService.open(this.confirmAlertNameID, AlertComponent, {
            content: 'This will go through the thumbs and remove any thumbs where the media do not exist or they not have an image in the media directory.',
            titleText: 'Clean thumbs',
            acceptText: 'Clean',
            rejectText: 'Cancel',
            onAccept: () => this.serverService.startThumbCleanupProcess(),
            onClose: () => this.overlayService.close(this.confirmAlertNameID)
        });
    }

    // TCode commands
    addTCodeCommand(): void {
        this.tcodeCommands.push({ name: '', command: '', hidden: false });
        this.tcodeDirty = true;
    }

    deleteTCodeCommand(index: number): void {
        this.tcodeCommands.splice(index, 1);
        this.tcodeDirty = true;
    }

    onTCodeCommandChange(): void {
        this.tcodeDirty = true;
    }

    saveTCodeCommands(): void {
        this.serverService.saveTCodeCommands(this.tcodeCommands).subscribe({
            next: () => {
                this.tcodeDirty = false;
                this.serverService.getTCodeCommands();
            },
            error: () => { }
        });
    }

    cancelTCodeCommands(): void {
        this.serverService.getTCodeCommands();
        this.tcodeDirty = false;
    }
}