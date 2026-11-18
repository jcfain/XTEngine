import { Component, Input, ChangeDetectionStrategy } from '@angular/core';

import { MediaItem, ScriptInfo } from '../../models/interfaces';

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

    private expandedScripts = new Set<number>();

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

    get scripts(): ScriptInfo[] {
        return (this.item?.metaData?.scripts || []);
        // return (this.item?.metaData?.scripts || [])
        //     .map(s => s.filename + (s.track ? '.' + s.track : ''));
    }
    get tracks(): string[] {
        return (this.item?.metaData?.MFSTracks || []);
        // return (this.item?.metaData?.scripts || [])
        //     .map(s => s.filename + (s.track ? '.' + s.track : ''));
    }

    isExpanded(index: number): boolean {
        return this.expandedScripts.has(index);
    }

    toggleScript(index: number): void {
        if (this.expandedScripts.has(index)) {
            this.expandedScripts.delete(index);
        } else {
            this.expandedScripts.add(index);
        }
    }

    formatTime(ms: string): string {
        const value = Number(ms);
        if (!Number.isFinite(value) || value < 0) {
            return '';
        }
        const totalSeconds = Math.floor(value / 1000);
        const hours = Math.floor(totalSeconds / 3600);
        const minutes = Math.floor((totalSeconds % 3600) / 60);
        const seconds = totalSeconds % 60;
        return hours > 0
            ? `${hours}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`
            : `${minutes}:${String(seconds).padStart(2, '0')}`;
    }

    formatOriginalDuration(script: ScriptInfo): string {
        const raw = script.metadata?.original_total_duration_ms;
        const value = Number(raw);
        if (raw === undefined || raw === null || raw === '' || !Number.isFinite(value) || value < 0) {
            return 'Unknown';
        }
        return `${raw} (${this.formatTime(raw)})`;
    }
}