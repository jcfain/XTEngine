import { Component, ChangeDetectionStrategy, EventEmitter, Inject, Input, Optional, Output } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { MediaItem, ScriptInfo } from '../../models/interfaces';
import { ScriptContainerType, ScriptType } from '../../models/enums';
import { WebsocketService } from '../../services/websocket.service';

export interface MediaItemAltScriptsData {
    item: MediaItem;
}

@Component({
    selector: 'app-media-item-alt-scripts',
    imports: [
        FormsModule
    ],
    templateUrl: './media-item-alt-scripts.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-item-alt-scripts.component.css'
})
export class MediaItemAltScriptsComponent {
    @Output() close = new EventEmitter<void>();

    altScripts: ScriptInfo[] = [];
    selectedScript: ScriptInfo | null = null;

    private _item: MediaItem | null = null;

    @Input()
    set item(value: MediaItem | null) {
        this._item = value;
        this.setupAltScripts();
    }
    get item(): MediaItem | null {
        return this._item;
    }

    constructor(
        private websocketService: WebsocketService,
        @Inject('overlayData') @Optional() data?: MediaItemAltScriptsData
    ) {
        // When opened via the overlay service, the item is injected.
        // When rendered inline (e.g. inside the video player), it is
        // supplied through the @Input() binding instead.
        if (data) {
            this.item = data.item;
        }
    }

    private setupAltScripts(): void {
        this.altScripts = this.filterScripts();
    }

    private filterScripts(): ScriptInfo[] {
        const scripts = this._item?.metaData?.scripts;
        if (!scripts || !scripts.length) {
            return [];
        }
        const result: ScriptInfo[] = [];
        scripts.forEach(x => {
            // Skip MFS/ZIP main scripts
            if ((x.containerType === ScriptContainerType.MFS || x.containerType === ScriptContainerType.ZIP) && x.type === ScriptType.MAIN) {
                return;
            }
            result.push(x);
        });
        return result;
    }

    getScriptInfoName(scriptInfo: ScriptInfo): string {
        return scriptInfo.name + (scriptInfo.containerTypeName.length ? " " + scriptInfo.containerTypeName : "");
    }

    isSelected(scriptInfo: ScriptInfo): boolean {
        // The "Default" main script is the active one when nothing else has been chosen.
        if (!this.selectedScript) {
            return scriptInfo.name === "Default";
        }
        return this.selectedScript.name === scriptInfo.name;
    }

    onScriptSelected(scriptInfo: ScriptInfo): void {
        this.websocketService.sendSwapScript(scriptInfo);
        this.selectedScript = scriptInfo.name === "Default" ? null : scriptInfo;
        this.close.emit();
    }

    closeOverlay(): void {
        this.close.emit();
    }
}