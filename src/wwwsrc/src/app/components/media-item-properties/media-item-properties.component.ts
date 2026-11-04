import { Component, ChangeDetectionStrategy, Inject } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { Observable } from 'rxjs';
import { MediaItem, MediaItemMetaData } from '../../models/interfaces';
import { MediaItemSettingsTabComponent } from './media-item-settings-tab.component';
import { MediaItemTagsTabComponent } from './media-item-tags-tab.component';
import { MediaItemInfoTabComponent } from './media-item-info-tab.component';

export interface MediaItemPropertiesData {
  item: MediaItem;
  allTags: string[];
  onSave: (metaData: MediaItemMetaData) => Observable<any>;
  onClose: () => void;
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
    item: MediaItem;
    allTags: string[];

    tabs = ['Settings', 'Tags', 'Info'];
    activeTab = 0;

    isDirty = false;
    saveState = '';

    constructor(@Inject('overlayData') private data: MediaItemPropertiesData) {
        this.item = this.data.item;
        this.allTags = this.data.allTags || [];
    }

    tabClick(index: number): void {
        this.activeTab = index;
    }

    markDirty(): void {
        this.isDirty = true;
    }

    save(): void {
        if (!this.item) {
            return;
        }
        this.saveState = 'Saving...';
        this.data.onSave(this.item.metaData).subscribe({
            next: () => {
                this.isDirty = false;
                this.saveState = 'Saved';
                setTimeout(() => this.saveState = '', 2000);
            },
            error: (err) => {
                console.error('Error saving metadata:', err);
                this.saveState = 'Save failed';
                setTimeout(() => this.saveState = '', 2000);
            }
        });
    }

    close(): void {
        if (this.data.onClose) {
            this.data.onClose();
        }
    }
}