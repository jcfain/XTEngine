import { Component, OnInit, ChangeDetectionStrategy, Output, EventEmitter } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { ServerService } from '../../services/server.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { ChannelModel } from '../../models/interfaces';
import { ChannelType } from '../../models/enums';
import { round } from '../../utils/utils';
import { RangeSliderComponent, RangeValue } from '../range-slider/range-slider.component';
import { SelectDialogComponent } from '../select-dialog/select-dialog.component';
import { GetTextDialogComponent } from '../get-text-dialog/get-text-dialog.component';
import { AlertComponent } from '../alert-model/alert.component';
import { WebsocketService } from '../../services/websocket.service';
import { MessageHandlerService } from '../../services/message-handler.service';

interface ChannelRange {
    channel: ChannelModel;
    userMin: number;
    userMax: number;
}

@Component({
    selector: 'app-settings-motion-tab',
    imports: [FormsModule, RangeSliderComponent],
    templateUrl: './settings-motion-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-motion-tab.component.css'
})
export class SettingsMotionTabComponent implements OnInit {
    @Output()
    onDirty = new EventEmitter();

    profiles: string[] = [];
    selectedProfile = '';
    channels: ChannelRange[] = [];

    multiplierEnabled = false;
    modifierChannels: ChannelModel[] = [];
    userAgentIsHereSphere = false;

    private profileSelectNameID = 'motionProfileSelect';
    private relatedChannelSelectNameID = 'motionRelatedChannelSelect';
    private addProfileTextNameID = 'motionAddProfileText';
    private copyToProfileTextNameID = 'motionCopyToProfileText';
    private copyFromProfileSelectNameID = 'motionCopyFromProfileSelect';
    private profileAlertNameID = 'motionProfileAlert';

    enabledHelpText = 'The will toggle whether this channel is included in the random motion generation or not.';
    linkToHelpText = 'This will match the values of the linked channel 1:1 with modifiers speed and offset applied.';
    linkInvertedHelpText = 'Inverts the selected linked channel if exists';
    speedHelpText = 'The percentage of the linked scripts time to modify the speed.\nIf the speed is 0.1, the speed will be 10% of the linked script or 90% slower.\nA value of 2 will be 200% faster.\nMinimum value is 0.01';
    offsetHelpText = 'The percentage of the linked scripts time to offset.\nIf the offset is 0.1 and the parent action interval is 300ms, the offset will be 30ms after the linked script event.\nCan be any value between -1.00 and 1.00';

    private rangeDebounceers: { [id: string]: any } = {};
    private rangeDebounceTimeout = 1000;

    constructor(
        private appState: AppStateService,
        private serverService: ServerService,
        private websocketService: WebsocketService,
        private messageHandlerService: MessageHandlerService,
        private overlayService: GenericOverlayService
    ) {
        this.userAgentIsHereSphere = this.appState.userAgentIsHereSphere;
    }

    ngOnInit(): void {
        this.serverService.remoteUserSettings$.subscribe(settings => {
            if (!settings) {
                return;
            }
            this.profiles = settings.allChannelProfileNames || [];
            this.selectedProfile = settings.selectedChannelProfile || '';
            this.multiplierEnabled = settings.multiplierEnabled || false;
            const available = settings.availableChannelsArray || Object.values(settings.availableChannels || {});
            this.modifierChannels = available.filter(c => c.type !== ChannelType.HalfOscillate);
            this.channels = this.modifierChannels.map(c => ({ channel: c, userMin: c.userMin, userMax: c.userMax }));
        });
        this.messageHandlerService.channelProfileChanged$.subscribe(v => {
            this.serverService.getServerChannels();
        });
    }

    onProfileChange(profile: string): void {
        this.selectedProfile = profile;
        this.websocketService.setSelectedProfile(profile);
    }

    openProfileSelect(): void {
        this.overlayService.open(this.profileSelectNameID, SelectDialogComponent, {
            titleText: 'Channel profile',
            message: 'Select a channel profile',
            options: this.profiles.map(p => ({ value: p, label: p })),
            selectedValue: this.selectedProfile,
            onAccept: (value: string) => this.onProfileChange(value),
            onClose: () => this.overlayService.close(this.profileSelectNameID)
        }, undefined, true);
    }

    onAddProfile(): void {
        this.overlayService.open(this.addProfileTextNameID, GetTextDialogComponent, {
            titleText: 'Add new profile',
            message: 'Enter the name of the new profile',
            onAccept: (value: string) => {
                if (value) {
                    this.websocketService.addChannelProfile(value);
                }
            },
            onClose: () => this.overlayService.close(this.addProfileTextNameID)
        }, undefined, true);
    }

    onDeleteProfile(): void {
        if (this.profiles.length < 2) {
            this.showProfileAlert('There must be at least 1 profile');
            return;
        }
        this.overlayService.open(this.profileAlertNameID, AlertComponent, {
            content: `Are you sure you want to delete ${this.selectedProfile}?\nThis action cannot be undone!`,
            titleText: 'Warning',
            acceptText: 'Delete',
            rejectText: 'Cancel',
            onAccept: () => {
                this.websocketService.deleteChannelProfile(this.selectedProfile);
            },
            onClose: () => this.overlayService.close(this.profileAlertNameID)
        });
    }

    onCopyToProfile(): void {
        this.overlayService.open(this.copyToProfileTextNameID, GetTextDialogComponent, {
            titleText: 'Copy profile to new',
            message: 'Enter the name of the new profile',
            onAccept: (value: string) => {
                if (value) {
                    this.websocketService.cloneChannelProfile(this.selectedProfile, value);
                }
            },
            onClose: () => this.overlayService.close(this.copyToProfileTextNameID)
        }, undefined, true);
    }

    onCopyFromProfile(): void {
        if (this.profiles.length < 2) {
            this.showProfileAlert('There is only one profile');
            return;
        }
        const others = this.profiles.filter(p => p !== this.selectedProfile);
        this.overlayService.open(this.copyFromProfileSelectNameID, SelectDialogComponent, {
            titleText: 'Copy profile from',
            message: 'Select a profile to copy from',
            options: others.map(p => ({ value: p, label: p })),
            onAccept: (value: string) => {
                this.websocketService.cloneChannelProfile(value, this.selectedProfile);
            },
            onClose: () => this.overlayService.close(this.copyFromProfileSelectNameID)
        }, undefined, true);
    }

    showProfileAlert(message: string): void {
        this.overlayService.open(this.profileAlertNameID, AlertComponent, {
            content: message,
            titleText: 'Warning',
            onClose: () => this.overlayService.close(this.profileAlertNameID)
        });
    }

    onRangeInstantChange(range: ChannelRange, value: number): void {
        this.websocketService.sendTCode(range.channel.channel + value.toString().padStart(4, '0') + 'S2000');
    }

    onRangeChange(range: ChannelRange, value: RangeValue): void {
        range.userMin = value.min;
        range.userMax = value.max;
        const id = range.channel.channel;
        if (this.rangeDebounceers[id]) {
            clearTimeout(this.rangeDebounceers[id]);
        }
        this.rangeDebounceers[id] = setTimeout(() => {
            this.rangeDebounceers[id] = undefined;
        this.websocketService.sendTCodeRange(id, range.userMin, range.userMax);
        }, this.rangeDebounceTimeout);
    }

    homeDevice(): void {
        this.serverService.sendDeviceHome();
    }

    onMultiplierEnabledChange(value: boolean): void {
        const settings = this.serverService.remoteUserSettings;
        if (settings) {
            settings.multiplierEnabled = value;
            this.multiplierEnabled = value;
            this.onDirty.emit();
        }
    }

    relatedChannelOptions(channel: ChannelModel): ChannelModel[] {
        return this.modifierChannels.filter(c => c.channel !== channel.channel);
    }

    relatedChannelName(channel: ChannelModel): string {
        const related = this.modifierChannels.find(x => x.axisName === channel.relatedChannel);
        return related ? related.friendlyName : '';
    }

    openRelatedChannelSelect(channel: ChannelModel): void {
        this.overlayService.open(this.relatedChannelSelectNameID, SelectDialogComponent, {
            titleText: 'Link channel to',
            message: 'Select channel',
            options: this.relatedChannelOptions(channel).map(c => ({ value: c.axisName, label: c.friendlyName })),
            selectedValue: channel.relatedChannel,
            onAccept: (value: string) => {
                channel.relatedChannel = value;
                this.onDirty.emit();
            },
            onClose: () => this.overlayService.close(this.relatedChannelSelectNameID)
        }, undefined, true);
    }

    onChannelChange(channel: ChannelModel): void {
        this.onDirty.emit();
    }

    onSpeedValueChange(channel: ChannelModel, value: number): void {
        if (value !== undefined && value !== null && !isNaN(value)) {
            channel.speedValue = round(value, 2);
            this.onDirty.emit();
        }
    }

    onOffsetChange(channel: ChannelModel, value: number): void {
        if (value !== undefined && value !== null && !isNaN(value)) {
            channel.offset = value;
            this.onDirty.emit();
        }
    }
}