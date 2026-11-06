import { Injectable } from "@angular/core";
import { GenericOverlayService } from "./overlay.service";
import { ScriptStatusComponent } from "../components/script-status/script-status.component";
import { ConnectedPosition } from "@angular/cdk/overlay";


@Injectable({
  providedIn: 'root'
})
export class ScriptStatusService {

    constructor(
        private overlayService: GenericOverlayService
    ) {}


    toggleScriptStatusHeader($event?: MouseEvent): boolean {
        const nameID = "scriptStatusHeader";
        if(this.overlayService.isOpen(nameID)) {
            this.overlayService.close(nameID);
        } else {
            const positions: ConnectedPosition[] = [
            {
                originX: 'center',
                originY: 'bottom',
                overlayX: 'center',
                overlayY: 'bottom',
                offsetY: 75, // 20px below the clicked element
            },
            ]
            this.overlayService.open(nameID, ScriptStatusComponent, null, false, $event, positions);
        }
        return this.overlayService.isOpen(nameID);
    }

    toggleScriptStatusVideo(originElement?: HTMLElement): boolean {
        const nameID = "scriptStatusVideo";
        if(this.overlayService.isOpen(nameID)) {
            this.overlayService.close(nameID);
        } else {
            // Anchor to the video player's bottom controls so the overlay is always
            // aligned with them, regardless of which button triggered it.
            const positions: ConnectedPosition[] = [
            {
                originX: 'center',
                originY: 'top',
                overlayX: 'center',
                overlayY: 'bottom',
                offsetY: -8, // small gap above the bottom controls
            },
            ]
            this.overlayService.open(nameID, ScriptStatusComponent, null, false, originElement, positions);
        }
        return this.overlayService.isOpen(nameID);
    }
}