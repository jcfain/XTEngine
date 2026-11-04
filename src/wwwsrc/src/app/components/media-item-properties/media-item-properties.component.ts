import { Component, ChangeDetectionStrategy, EventEmitter, Inject, Input, Optional, Output } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { MediaItem } from '../../models/interfaces';
import { MediaService } from '../../services/media.service';
import { ServerService } from '../../services/server.service';
import { MediaItemSettingsTabComponent } from './media-item-settings-tab.component';
import { MediaItemTagsTabComponent } from './media-item-tags-tab.component';
import { MediaItemInfoTabComponent } from './media-item-info-tab.component';

export interface MediaItemPropertiesData {
  item: MediaItem;
}

@Component({
    selector: 'app-media-item-properties',
    imports: [
        FormsModule,
        MediaItemSettingsTabComponent,
        MediaItemTagsTabComponent,
        MediaItemInfoTabComponent
    ],
    templateUrl: './media-item-properties.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-item-properties.component.css'
})
export class MediaItemPropertiesComponent {
    @Input() item: MediaItem | null = null;
    @Output() close = new EventEmitter<void>();

    get allTags(): string[] {
        return this._allTags;
    }

    tabs = ['Settings', 'Tags', 'Info'];
    activeTab = 0;

    get isDirty() {
        return this._isDirty || this._isDirty && !this._isSaving;
    }
    private _isDirty = false;
    private _isSaving = false;
    saveState = '';

    private _allTags: string[] = [];

    constructor(
        private mediaService: MediaService,
        private serverService: ServerService,
        @Inject('overlayData') @Optional() data?: MediaItemPropertiesData
    ) {
        // When opened via the overlay service, the item is injected.
        // When rendered inline (e.g. inside the video player), it is
        // supplied through the @Input() binding instead.
        if (data) {
            this.item = data.item;
        }
        this.serverService.remoteUserSettings$.subscribe(v => {
            this._allTags = v?.allTags || [];
        });
    }

    tabClick(index: number): void {
        this.activeTab = index;
    }

    markDirty(): void {
        this._isDirty = true;
    }

    save(): void {
        if (!this.item) {
            return;
        }
        this.saveState = 'Saving...';
        this._isSaving = true;
        this.mediaService.updateItemMetadata(this.item.metaData).subscribe({
            next: () => {
                this.saveState = 'Saved';
                setTimeout(() => this.saveState = '', 2000);
                this._isSaving = false;
                this._isDirty = false;
            },
            error: (err) => {
                console.error('Error saving metadata:', err);
                this.saveState = 'Save failed';
                setTimeout(() => this.saveState = '', 2000);
                this._isSaving = false;
            }
        });
    }

    closeProperties(): void {
        this.close.emit();
    }
}