// WebXR VR media player overlay component.
//
// Entry point of the self-contained xr-player feature: the only coupling to
// the main app is the VR button in app-header and the <app-xr-player> line
// in app.component.html.
import {
    Component, OnInit, OnDestroy, AfterViewInit, Output, EventEmitter,
    ViewChild, ElementRef, ChangeDetectionStrategy
} from '@angular/core';
import { Subscription } from 'rxjs';
import { XrPlayerService } from './xr-player.service';

@Component({
    selector: 'app-xr-player',
    imports: [],
    templateUrl: './xr-player.component.html',
    // Eager (not OnPush): status/supported are updated from async callbacks
    // (WebXR support check, session events) that don't trigger OnPush
    // re-renders, which left the status line empty and the Enter VR button
    // missing.
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './xr-player.component.css'
})
export class XrPlayerComponent implements OnInit, OnDestroy, AfterViewInit {
    @ViewChild('canvas') canvasRef!: ElementRef<HTMLCanvasElement>;
    @Output() close = new EventEmitter<void>();

    supported = false;
    sessionActive = false;
    status = '';

    private sub?: Subscription;
    private wasInSession = false;

    constructor(private xrService: XrPlayerService) { }

    ngOnInit(): void {
        this.sub = new Subscription();
        this.xrService.supported$.subscribe(v => this.supported = v);
        this.xrService.status$.subscribe(v => this.status = v);
        this.xrService.sessionActive$.subscribe(v => {
            this.sessionActive = v;
            // When the session ends (headset button, Exit panel button or
            // browser), return to the home page - any playing video has
            // already been handed back to the in-page player by the service.
            if (this.wasInSession && !v) {
                this.close.emit();
            }
            this.wasInSession = v;
        });
    }

    ngAfterViewInit(): void {
        this.xrService.init(this.canvasRef.nativeElement);
    }

    enterVR(): void {
        this.xrService.enterVR();
    }

    exit(): void {
        if (this.sessionActive) {
            this.xrService.exitVR();
        } else {
            this.close.emit();
        }
    }

    ngOnDestroy(): void {
        this.sub?.unsubscribe();
        this.xrService.ngOnDestroy();
    }
}