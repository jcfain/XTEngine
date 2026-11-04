import { Component, Input, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { MediaItem } from '../../models/interfaces';

@Component({
    selector: 'app-media-item-tags-tab',
    imports: [FormsModule],
    templateUrl: './media-item-tags-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-item-tags-tab.component.css'
})
export class MediaItemTagsTabComponent {
    @Input() item!: MediaItem;
    @Input() allTags: string[] = [];
    @Input() isVisible = false;
    @Output() onDirty = new EventEmitter<void>();

    filterText = '';

    get filteredTags(): string[] {
        const filter = this.filterText.trim().toLowerCase();
        if (!filter) {
            return this.allTags;
        }
        return this.allTags.filter(tag => tag.toLowerCase().includes(filter));
    }

    hasTag(tag: string): boolean {
        return this.item?.metaData?.tags?.includes(tag) ?? false;
    }

    toggleTag(tag: string): void {
        const tags = this.item?.metaData?.tags;
        if (!tags) {
            return;
        }
        const index = tags.findIndex(t => t === tag);
        if (index === -1) {
            tags.push(tag);
        } else {
            tags.splice(index, 1);
        }
        this.onDirty.emit();
    }
}