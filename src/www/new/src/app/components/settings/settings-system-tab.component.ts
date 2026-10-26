import { Component, OnInit, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { SettingsService } from '../../services/settings.service';
import { AppStateService } from '../../services/app-state.service';
import { Setting } from '../../models/interfaces';
import { FormControlType } from '../../models/enums';
import { capitalizeFirstLetter } from '../../utils/utils';

interface SettingGroup {
    name: string;
    settings: Setting[];
}

@Component({
    selector: 'app-settings-system-tab',
    imports: [FormsModule],
    templateUrl: './settings-system-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-system-tab.component.css'
})
export class SettingsSystemTabComponent implements OnInit {
    @Output() restartRequiredChange = new EventEmitter<boolean>();

    groups: SettingGroup[] = [];
    xteVersion = '';
    userAgent = navigator.userAgent;
    userAgentIsDeo = false;
    userAgentIsHereSphere = false;
    userAgentIsMobile = false;

    private debounceers: { [id: string]: any } = {};
    private debounceerTimeout = 1000;

    constructor(
        private settingsService: SettingsService,
        private appState: AppStateService
    ) {}

    ngOnInit(): void {
        this.appState.remoteUserSettings$.subscribe(settings => {
            if (settings) {
                this.xteVersion = settings.xteVersion || '';
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
}