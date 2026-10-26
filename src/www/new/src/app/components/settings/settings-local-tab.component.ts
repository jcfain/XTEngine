import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { AppStateService } from '../../services/app-state.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { ShowMode, SortMode } from '../../models/enums';
import { SelectDialogComponent } from '../select-dialog/select-dialog.component';

@Component({
    selector: 'app-settings-local-tab',
    imports: [FormsModule],
    templateUrl: './settings-local-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-local-tab.component.css'
})
export class SettingsLocalTabComponent implements OnInit {
    showModes = [
        { mode: ShowMode.All, label: 'All' },
        { mode: ShowMode.ThreeD, label: '3D only' },
        { mode: ShowMode.TwoD, label: '2D and audio only' }
    ];

    sortModes = [
        { mode: SortMode.Name, label: 'Name (asc)' },
        { mode: SortMode.NameDesc, label: 'Name (desc)' },
        { mode: SortMode.Date, label: 'Date (asc)' },
        { mode: SortMode.DateDesc, label: 'Date (desc)' },
        { mode: SortMode.DateAdded, label: 'Added (asc)' },
        { mode: SortMode.DateAddedDesc, label: 'Added (desc)' },
        { mode: SortMode.Random, label: 'Random' }
    ];

    thumbSizes: number[] = [];
    showMode: ShowMode = ShowMode.All;
    sortMode: SortMode = SortMode.Name;
    thumbSize = 200;
    externalStreaming = false;
    disableLazyLoad = false;

    constructor(
        private localSettings: LocalSettingsService,
        private appState: AppStateService,
        private overlayService: GenericOverlayService
    ) {}

    ngOnInit(): void {
        for (let size = 50; size <= 500; size += 25) {
            this.thumbSizes.push(size);
        }
        this.localSettings.show$.subscribe(v => this.showMode = v);
        this.localSettings.sort$.subscribe(v => this.sortMode = v);
        this.localSettings.thumbSize$.subscribe(v => this.thumbSize = v);
        this.externalStreaming = this.appState.externalStreaming;
        this.disableLazyLoad = this.appState.disableBrowserLazyLoad;
    }

    onShowChange(mode: ShowMode): void {
        this.localSettings.setShow(mode);
    }

    onSortChange(mode: SortMode): void {
        this.localSettings.setSort(mode);
    }

    onThumbSizeChange(size: number): void {
        this.localSettings.setThumbSize(size);
    }

    showModeLabel(): string {
        const mode = this.showModes.find(m => m.mode === this.showMode);
        return mode ? mode.label : '';
    }

    sortModeLabel(): string {
        const mode = this.sortModes.find(m => m.mode === this.sortMode);
        return mode ? mode.label : '';
    }

    openShowSelect(): void {
        this.overlayService.open('localShowSelect', SelectDialogComponent, {
            titleText: 'Show',
            message: 'Select what to show',
            options: this.showModes.map(m => ({ value: m.mode, label: m.label })),
            selectedValue: this.showMode,
            onAccept: (value: ShowMode) => this.onShowChange(value),
            onClose: () => this.overlayService.close('localShowSelect')
        }, undefined, true);
    }

    openSortSelect(): void {
        this.overlayService.open('localSortSelect', SelectDialogComponent, {
            titleText: 'Sort by',
            message: 'Select sort order',
            options: this.sortModes.map(m => ({ value: m.mode, label: m.label })),
            selectedValue: this.sortMode,
            onAccept: (value: SortMode) => this.onSortChange(value),
            onClose: () => this.overlayService.close('localSortSelect')
        }, undefined, true);
    }

    openThumbSizeSelect(): void {
        this.overlayService.open('localThumbSizeSelect', SelectDialogComponent, {
            titleText: 'Thumb size',
            message: 'Select thumb size',
            options: this.thumbSizes.map(size => ({ value: size, label: size + 'px' })),
            selectedValue: this.thumbSize,
            onAccept: (value: number) => this.onThumbSizeChange(value),
            onClose: () => this.overlayService.close('localThumbSizeSelect')
        }, undefined, true);
    }

    onExternalStreamingChange(checked: boolean): void {
        this.externalStreaming = checked;
        this.appState.externalStreaming = checked;
        window.localStorage.setItem('externalStreaming', JSON.stringify(checked));
    }

    onDisableLazyLoadChange(checked: boolean): void {
        this.disableLazyLoad = checked;
        this.appState.disableBrowserLazyLoad = checked;
        window.localStorage.setItem('disableLazyLoad', JSON.stringify(checked));
    }
}