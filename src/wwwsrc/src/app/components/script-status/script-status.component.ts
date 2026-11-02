import { Component, OnInit, OnDestroy, ChangeDetectionStrategy, ChangeDetectorRef } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Subscription } from 'rxjs';
import { AvailableChannels, ChannelInfo, ChannelModel } from '../../models/interfaces';
import { getChannelColor, scale, lerp, round } from '../../utils/utils';
import { VideoService } from '../../services/video.service';
import { AppStateService } from '../../services/app-state.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { ServerService } from '../../services/server.service';

interface ChannelAnimationState {
  channel: ChannelInfo;
  hidden: boolean;
  startValue: number;
  targetValue: number;
  targetTime: number;
  startTime: number;
  lastAnimation: number | null;
  progressValue: number;
}

@Component({
    selector: 'app-script-status',
    imports: [CommonModule],
    templateUrl: './script-status.component.html',
    changeDetection: ChangeDetectionStrategy.OnPush,
    styleUrl: './script-status.component.css'
})
export class ScriptStatusComponent implements OnInit, OnDestroy {
  // isShown = false;
  // isModalShown = false;
  isPlayerShown = false;
  isPaused = false;
  channels: ChannelAnimationState[] = [];

  private animationFrameId: number | null = null;
  private availableChannels: AvailableChannels | undefined = undefined;

  constructor(
    private serverService: ServerService,
    private messageHandlerService: MessageHandlerService,
    private videoService: VideoService,
    private cdr: ChangeDetectorRef
  ) {
    this.serverService.remoteUserSettings$.subscribe(v => {
      this.availableChannels = v?.availableChannels;
    });
    this.messageHandlerService.channelPositionChange$.subscribe(v => {
      if(!v)
        return;
      this.setChannelStatus(v?.channel, v?.position, v?.time, v?.timeType);
    });
    this.videoService.isPaused$.subscribe(v => {
      this.isPaused = v;
      if (!v && this.animationFrameId === null) {
        this.animate();
      }
    });
    this.videoService.isPlaying$.subscribe(v => this.isPlayerShown = v);
  }

  ngOnInit(): void {
    // Animation loop is started on demand in setChannelStatus
  }

  ngOnDestroy(): void {
    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
      this.animationFrameId = null;
    }
  }

  // toggleModal(): void {
  //   this.isModalShown = !this.isModalShown;
  //   this.updateShown();
  // }

  // togglePlayer(): void {
  //   this.isPlayerShown = !this.isPlayerShown;
  //   this.updateShown();
  // }

  // setPaused(isPaused: boolean): void {
  //   this.isPaused = isPaused;
  // }

  setChannelStatus(channel: string, value: number, time: number, timeType: number): void {
    // if (!this.isShown) return;

    let state = this.channels.find(c => c.channel.channel === channel);
    if (!state) {
      let friendlyName = "Unknown";
      if(this.availableChannels) {
        const channelModel: ChannelModel = this.availableChannels[channel];
        friendlyName = channelModel.friendlyName;
      }
      state = {
        channel: { channel, friendlyName },
        hidden: false,
        startValue: 0,
        targetValue: value,
        targetTime: time,
        startTime: 0,
        lastAnimation: null,
        progressValue: 0
      };
      this.channels.push(state);
    } else {
      state.startValue = state.progressValue;
      state.targetValue = value;
      state.targetTime = time;
      state.startTime = 0;
      state.hidden = false;
    }

    if (this.animationFrameId === null) {
      this.animate();
    }
  }

  // private updateShown(): void {
  //   this.isShown = this.isModalShown || this.isPlayerShown;
  // }

  private animate = (): void => {
    const now = performance.now();
    let anyActive = false;

    for (const state of this.channels) {
      if (state.hidden) continue;
      if (this.isPaused) continue;

      if (!state.startTime) {
        state.startTime = now;
      }

      const elapsed = now - state.startTime;
      const downStroke = state.startValue > state.targetValue;
      const timeScale = downStroke
        ? scale(elapsed, state.targetTime, 0, 0.0, 1.0)
        : scale(elapsed, 0, state.targetTime, 0.0, 1.0);

      if (!isNaN(timeScale) && timeScale >= 0 && timeScale <= 1) {
        state.progressValue = downStroke
          ? lerp(state.targetValue, state.startValue, timeScale)
          : lerp(state.startValue, state.targetValue, timeScale);
      }

      if (state.progressValue !== state.targetValue && elapsed < state.targetTime) {
        anyActive = true;
      } else {
        state.progressValue = state.targetValue;
      }
    }

    this.cdr.markForCheck();

    if (anyActive) {
      this.animationFrameId = requestAnimationFrame(this.animate);
    } else {
      this.animationFrameId = null;
    }
  };

  getChannelColor(channel: string): string {
    return getChannelColor(channel);
  }
}
