import { Component, Input, Output, EventEmitter, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { MediaItem } from '../../models/interfaces';

@Component({
    selector: 'app-media-item-settings-tab',
    imports: [FormsModule],
    templateUrl: './media-item-settings-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-item-settings-tab.component.css'
})
export class MediaItemSettingsTabComponent implements OnInit {
    @Input() item!: MediaItem;
    @Input() isVisible = false;
    @Output() onDirty = new EventEmitter<void>();

    offset = 0;
    funscriptModifier = 0;
    moneyShotMillis = -1;

    ngOnInit(): void {
        const meta = this.item?.metaData;
        if (!meta) {
            return;
        }
        this.offset = meta.offset ?? 0;
        this.funscriptModifier = meta.funscriptModifier ?? 0;
        this.moneyShotMillis = (meta.moneyShotMillis === undefined || meta.moneyShotMillis === null)
            ? -1
            : Number(meta.moneyShotMillis);
    }

    onOffsetChange(): void {
        if (this.item?.metaData) {
            this.item.metaData.offset = this.offset;
        }
        this.onDirty.emit();
    }

    onModifierChange(): void {
        if (this.item?.metaData) {
            this.item.metaData.funscriptModifier = this.funscriptModifier;
        }
        this.onDirty.emit();
    }

    onMoneyShotChange(): void {
        if (this.item?.metaData) {
            this.item.metaData.moneyShotMillis = String(this.moneyShotMillis);
        }
        this.onDirty.emit();
    }

    addToOffset(amount: number): void {
        this.offset += amount;
        this.onOffsetChange();
    }

    addToModifier(amount: number): void {
        this.funscriptModifier += amount;
        this.onModifierChange();
    }

    resetMoneyShot(): void {
        this.moneyShotMillis = -1;
        this.onMoneyShotChange();
    }
}