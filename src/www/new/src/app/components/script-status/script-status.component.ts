import { Component, OnInit, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Subscription } from 'rxjs';
import { ChannelInfo } from '../../models/interfaces';
import { getChannelColor, scale, lerp, round } from '../../utils/utils';

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
  standalone: true,
  imports: [CommonModule],
  templateUrl: './script-status.component.html',
  styleUrl: './script-status.component.css'
})
export class ScriptStatusComponent implements OnInit, OnDestroy {
  isShown = false;
  isModalShown = false;
  isPlayerShown = false;
  isPaused = false;
  channels: ChannelAnimationState[] = [];

  private animationFrameId: number | null = null;

  ngOnInit(): void {
    // Start animation loop
    this.animate();
  }

  ngOnDestroy(): void {
    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
    }
  }

  toggleModal(): void {
    this.isModalShown = !this.isModalShown;
    this.updateShown();
  }

  togglePlayer(): void {
    this.isPlayerShown = !this.isPlayerShown;
    this.updateShown();
  }

  setPaused(isPaused: boolean): void {
    this.isPaused = isPaused;
  }

  setChannelStatus(channel: string, value: number, time: number, timeType: number, friendlyName: string): void {
    if (!this.isShown) return;

    let state = this.channels.find(c => c.channel.channel === channel);
    if (!state) {
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
  }

  private updateShown(): void {
    this.isShown = this.isModalShown || this.isPlayerShown;
  }

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

    this.animationFrameId = requestAnimationFrame(this.animate);
  };

  getChannelColor(channel: string): string {
    return getChannelColor(channel);
}
}