import { Component, Input, OnInit, OnDestroy, ChangeDetectionStrategy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Subscription } from 'rxjs';
import { ServerService } from '../../services/server.service';
import { WebsocketService } from '../../services/websocket.service';

// Shortcut to the channel profile selection on the Settings > Motion tab.
// Lists the available motion profiles as toggle buttons (TCode-action style)
// - only the currently selected profile is toggled, and selecting one sends
// the same changeChannelProfile message the Motion tab's profile select does.
@Component({
    selector: 'app-motion-profiles',
    imports: [CommonModule],
    templateUrl: './motion-profiles.component.html',
    changeDetection: ChangeDetectionStrategy.OnPush,
    styleUrl: './motion-profiles.component.css'
})
export class MotionProfilesComponent implements OnInit, OnDestroy {
    profiles: string[] = [];
    selectedProfile = '';
    // Width of the inner container, set by each usage site (the video player
    // panel is 50vw like the script status, the external player passes 100%
    // to fill its own panel).
    @Input() width: string = '50vw';

    private subscription: Subscription | undefined;

    constructor(
        private serverService: ServerService,
        private websocketService: WebsocketService
    ) {}

    ngOnInit(): void {
        this.subscription = this.serverService.serverChannels$.subscribe(settings => {
            if (!settings) {
                return;
            }
            this.profiles = settings.allChannelProfileNames || [];
            this.selectedProfile = settings.selectedChannelProfile || '';
        });
    }

    ngOnDestroy(): void {
        this.subscription?.unsubscribe();
    }

    selectProfile(profile: string): void {
        if (profile === this.selectedProfile) {
            return;
        }
        this.selectedProfile = profile;
        this.websocketService.setSelectedProfile(profile);
    }
}