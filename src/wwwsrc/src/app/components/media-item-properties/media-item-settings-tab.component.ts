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

    private _offset = 0;
    public get offset() {
        return this._offset;
    }
    public set offset(value) {
        this._offset = value;
        if (this.item?.metaData) {
            this.item.metaData.offset = this._offset;
            this.onDirty.emit();
        }
    }
    private _funscriptModifier = 0;
    public get funscriptModifier() {
        return this._funscriptModifier;
    }
    public set funscriptModifier(value) {
        this._funscriptModifier = value;
        if (this.item?.metaData) {
            this.item.metaData.funscriptModifier = this.funscriptModifier;
            this.onDirty.emit();
        }
    }
    private _moneyShotMillis = -1;
    public get moneyShotMillis() {
        return this._moneyShotMillis;
    }
    public set moneyShotMillis(value) {
        this._moneyShotMillis = value;
        if (this.item?.metaData) {
            this.item.metaData.moneyShotMillis = String(this.moneyShotMillis);
            this.onDirty.emit();
        }
    }

    ngOnInit(): void {
        const meta = this.item?.metaData;
        if (!meta) {
            return;
        }
        this._offset = meta.offset ?? 0;
        this._funscriptModifier = meta.funscriptModifier ?? 0;
        this._moneyShotMillis = (meta.moneyShotMillis === undefined || meta.moneyShotMillis === null)
            ? -1
            : Number(meta.moneyShotMillis);
    }

    // onOffsetChange(): void {
    //     // if(v != undefined)
    //     //     this.offset = v;
    //     // if (this.item?.metaData) {
    //     //     this.item.metaData.offset = this.offset;
    //     // }
    //     this.onDirty.emit();
    // }

    // onModifierChange(v?: number): void {
    //     if(v != undefined)
    //         this.funscriptModifier = v;
    //     if (this.item?.metaData) {
    //         this.item.metaData.funscriptModifier = this.funscriptModifier;
    //     }
    //     this.onDirty.emit();
    // }

    // onMoneyShotChange(v?: number): void {
    //     if(v != undefined)
    //         this.moneyShotMillis = v;
    //     if (this.item?.metaData) {
    //         this.item.metaData.moneyShotMillis = String(this.moneyShotMillis);
    //     }
    //     this.onDirty.emit();
    // }

    addToOffset(amount: number): void {
        this.offset += amount;
        // this.onOffsetChange();
    }

    addToModifier(amount: number): void {
        this.funscriptModifier += amount;
        // this.onModifierChange();
    }

    resetMoneyShot(): void {
        this.moneyShotMillis = -1;
        // this.onMoneyShotChange();
    }
}