import { Component, OnInit, ChangeDetectionStrategy, Output, EventEmitter, Input } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { ServerService } from '../../services/server.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { ChannelModel } from '../../models/interfaces';
import { ChannelType, Track } from '../../models/enums';
import { round } from '../../utils/utils';
import { RangeSliderComponent, RangeValue } from '../range-slider/range-slider.component';
import { SelectHsComponent } from '../select-hs/select-hs.component';
import { SelectDialogComponent } from '../select-dialog/select-dialog.component';
import { GetTextDialogComponent } from '../get-text-dialog/get-text-dialog.component';
import { AlertComponent } from '../alert-model/alert.component';
import { WebsocketService } from '../../services/websocket.service';

interface ChannelRange {
    channel: ChannelModel;
    userMin: number;
    userMax: number;
}

@Component({
    selector: 'app-settings-motion-tab',
    imports: [FormsModule, RangeSliderComponent, SelectHsComponent],
    templateUrl: './settings-motion-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-motion-tab.component.css'
})
export class SettingsMotionTabComponent implements OnInit {
    @Input()
    isVisible = false;
    @Output()
    onDirty = new EventEmitter();

    profiles: string[] = [];
    selectedProfile = '';
    channelRanges: ChannelRange[] = [];

    multiplierEnabled = false;
    channels: ChannelModel[] = [];

    private addProfileTextNameID = 'motionAddProfileText';
    private copyToProfileTextNameID = 'motionCopyToProfileText';
    private copyFromProfileSelectNameID = 'motionCopyFromProfileSelect';
    private profileAlertNameID = 'motionProfileAlert';
    private allChannels: ChannelModel[] = [];

    enabledHelpText = 'The will toggle whether this channel is included in the random motion generation or not.';
    linkToHelpText = 'This will match the values of the linked channel 1:1 with modifiers speed and offset applied.';
    linkInvertedHelpText = 'Inverts the selected linked channel if exists';
    speedHelpText = 'The percentage of the linked scripts time to modify the speed.\nIf the speed is 0.1, the speed will be 10% of the linked script or 90% slower.\nA value of 2 will be 200% faster.\nMinimum value is 0.01';
    offsetHelpText = 'The percentage of the linked scripts time to offset.\nIf the offset is 0.1 and the parent action interval is 300ms, the offset will be 30ms after the linked script event.\nCan be any value between -1.00 and 1.00';

    private rangeDebounceers: { [id: string]: any } = {};
    private rangeDebounceTimeout = 1000;

    constructor(
        private serverService: ServerService,
        private websocketService: WebsocketService,
        private overlayService: GenericOverlayService
    ) {
    }

    private relatedOptionsCache: Map<string, ChannelModel[]> = new Map();

    ngOnInit(): void {
        this.serverService.serverChannels$.subscribe(settings => {
            if (!settings) {
                return;
            }
            this.profiles = settings.allChannelProfileNames || [];
            this.selectedProfile = settings.selectedChannelProfile || '';
            this.serverService.setSelectedChannelProfile(this.selectedProfile);
            this.multiplierEnabled = settings.multiplierEnabled || false;
            const available = settings.availableChannelsArray || Object.values(settings.availableChannels || {});
            this.allChannels = available;
            this.relatedOptionsCache.clear();
            const wholeChannels = available.filter(c => c.type !== ChannelType.HalfOscillate);
            this.channels = this.sortChannels(wholeChannels);
            this.channelRanges = this.channels.map(c => ({ channel: c, userMin: c.userMin, userMax: c.userMax }));
        });
    }

    onProfileChange(profile: string): void {
        this.serverService.setSelectedChannelProfile(profile);
        this.websocketService.setSelectedProfile(profile);
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
        }, true);
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
        }, true);
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
        }, true);
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
        this.websocketService.sendDeviceHome();
    }

    onMultiplierEnabledChange(value: boolean): void {
        const channels = this.serverService.serverChannels;
        if (channels) {
            channels.multiplierEnabled = value;
            this.multiplierEnabled = value;
            this.onDirty.emit();
        }
    }

    relatedChannelOptions(channel: ChannelModel): ChannelModel[] {
        const key = channel.channel;
        if (!this.relatedOptionsCache.has(key)) {
            this.relatedOptionsCache.set(key, this.allChannels.filter(c => c.channel !== key));
        }
        return this.relatedOptionsCache.get(key)!;
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
            channel.offset = round(value, 2);
            this.onDirty.emit();
        }
    }

    private sortChannels(channelsIn: ChannelModel[]): ChannelModel[] {
        let channelsOut: ChannelModel[] = [];
        let index = channelsIn.findIndex(x => x.track == Track.Stroke);
        if(index > -1) {
            channelsOut.push(channelsIn.splice(index, 1)[0]);
        }
        index = channelsIn.findIndex(x => x.track == Track.Roll);
        if(index > -1) {
            channelsOut.push(channelsIn.splice(index, 1)[0]);
        }
        index = channelsIn.findIndex(x => x.track == Track.Pitch);
        if(index > -1) {
            channelsOut.push(channelsIn.splice(index, 1)[0]);
        }
        index = channelsIn.findIndex(x => x.track == Track.Twist);
        if(index > -1) {
            channelsOut.push(channelsIn.splice(index, 1)[0]);
        }
        index = channelsIn.findIndex(x => x.track == Track.Surge);
        if(index > -1) {
            channelsOut.push(channelsIn.splice(index, 1)[0]);
        }
        index = channelsIn.findIndex(x => x.track == Track.Sway);
        if(index > -1) {
            channelsOut.push(channelsIn.splice(index, 1)[0]);
        }
        channelsIn.forEach(x => channelsOut.push(x));
        return channelsOut;
    }
}