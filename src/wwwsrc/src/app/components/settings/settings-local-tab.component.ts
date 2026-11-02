import { Component, OnInit, ChangeDetectionStrategy, Input } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { ShowMode, SortMode } from '../../models/enums';
import { SelectDialogComponent } from '../select-dialog/select-dialog.component';
import { AppStateService } from '../../services/app-state.service';

@Component({
    selector: 'app-settings-local-tab',
    imports: [FormsModule],
    templateUrl: './settings-local-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-local-tab.component.css'
})
export class SettingsLocalTabComponent implements OnInit {
    @Input()
    isVisible = false;
    
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
    private _showMode: ShowMode = ShowMode.All;
    public get showMode(): ShowMode {
        return this._showMode;
    }
    public set showMode(value: ShowMode) {
        const valueNum: number =  Number(value);
        this.localSettings.setShow(valueNum);
        this._showMode = valueNum;
    }
    private _sortMode: SortMode = SortMode.Name;
    public get sortMode(): SortMode {
        return this._sortMode;
    }
    public set sortMode(value: SortMode) {
        const valueNum: number =  Number(value);
        this.localSettings.setSort(valueNum);
        this._sortMode = valueNum;
    }
    private _thumbSize = 200;
    public get thumbSize(): number {
        return this._thumbSize;
    }
    public set thumbSize(value: number) {
        // This is done because the select element 
        // Always pukes out a string for some reason.
        // This messes up the thumb size calculations done
        // on the media list.
        const valueNum: number =  Number(value);
        this.localSettings.setThumbSize(valueNum);
        this._thumbSize = valueNum;
    }
    externalStreaming = false;
    disableLazyLoad = false;
    userAgentIsHereSphere = false;

    constructor(
        private appState: AppStateService,
        private localSettings: LocalSettingsService,
        private overlayService: GenericOverlayService
    ) {}

    ngOnInit(): void {
        for (let size = 50; size <= 500; size += 25) {
            this.thumbSizes.push(size);
        }
        this.localSettings.show$.subscribe(v => this._showMode = v);
        this.localSettings.sort$.subscribe(v => this._sortMode = v);
        this.localSettings.thumbSize$.subscribe(v => this._thumbSize = v);
        this.localSettings.externalStreaming$.subscribe(v => this.externalStreaming = v);
        this.localSettings.disableBrowserLazyLoad$.subscribe(v => this.disableLazyLoad = v);
        this.userAgentIsHereSphere = this.appState.userAgentIsHereSphere;
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
            onAccept: (value: ShowMode) => this.showMode = value,
            onClose: () => this.overlayService.close('localShowSelect')
        }, undefined, true);
    }

    openSortSelect(): void {
        this.overlayService.open('localSortSelect', SelectDialogComponent, {
            titleText: 'Sort by',
            message: 'Select sort order',
            options: this.sortModes.map(m => ({ value: m.mode, label: m.label })),
            selectedValue: this.sortMode,
            onAccept: (value: SortMode) => this.sortMode = value,
            onClose: () => this.overlayService.close('localSortSelect')
        }, undefined, true);
    }

    openThumbSizeSelect(): void {
        this.overlayService.open('localThumbSizeSelect', SelectDialogComponent, {
            titleText: 'Thumb size',
            message: 'Select thumb size',
            options: this.thumbSizes.map(size => ({ value: size, label: size + 'px' })),
            selectedValue: this.thumbSize,
            onAccept: (value: number) => this.thumbSize = value,
            onClose: () => this.overlayService.close('localThumbSizeSelect')
        }, undefined, true);
    }

    onExternalStreamingChange(checked: boolean): void {
        this.localSettings.setExternalStreamingDefault(checked);
    }

    onDisableLazyLoadChange(checked: boolean): void {
        this.localSettings.setDisableBrowserLazyLoad(checked);
    }
}