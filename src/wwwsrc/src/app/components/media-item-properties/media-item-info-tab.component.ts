import { Component, Input, ChangeDetectionStrategy } from '@angular/core';

import { MediaItem } from '../../models/interfaces';

@Component({
    selector: 'app-media-item-info-tab',
    imports: [],
    templateUrl: './media-item-info-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-item-info-tab.component.css'
})
export class MediaItemInfoTabComponent {
    @Input() item!: MediaItem;
    @Input() isVisible = false;

    get name(): string {
        return this.item?.name || '';
    }

    get libraryItemPath(): string {
        return this.item?.metaData?.libraryItemPath || '';
    }

    get dateAdded(): string {
        const value = this.item?.metaData?.dateAdded;
        return value ? new Date(value).toLocaleString() : '';
    }

    get modifiedDate(): string {
        const value = this.item?.modifiedDate;
        return value ? new Date(value).toLocaleString() : '';
    }

    get hasAlternate(): boolean {
        return this.item?.metaData?.hasAlternate ?? false;
    }

    get scripts(): string[] {
        return (this.item?.metaData?.scripts || [])
            .map(s => s.filename + (s.track ? '.' + s.track : ''));
    }
}