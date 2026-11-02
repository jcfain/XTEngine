import { Injectable } from "@angular/core";
import { GenericOverlayService } from "./overlay.service";
import { ScriptStatusComponent } from "../components/script-status/script-status.component";


@Injectable({
  providedIn: 'root'
})
export class ScriptStatusService {

    constructor(
        private overlayService: GenericOverlayService
    ) {}

    toggleScriptStatus($event?: MouseEvent) {
        this.overlayService.open("scriptStatus", ScriptStatusComponent, null, $event);
    }
}