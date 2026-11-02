import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class AppStateService {
  // Connection
  private xtpConnectedSubject = new BehaviorSubject<boolean>(false);
  xtpConnected$: Observable<boolean> = this.xtpConnectedSubject.asObservable();

  // Misc
  private mediaLoadingSubject = new BehaviorSubject<boolean>(false);
  mediaLoading$: Observable<boolean> = this.mediaLoadingSubject.asObservable();

  private shufflePlayModeSubject = new BehaviorSubject<boolean>(false);
  shufflePlayMode$: Observable<boolean> = this.shufflePlayModeSubject.asObservable();

  // Browser info
  userAgent: string = navigator.userAgent;
  userAgentIsDeo: boolean = this.userAgent.indexOf('Deo VR') !== -1;
  userAgentIsHereSphere: boolean = this.userAgent.indexOf('HereSphere') !== -1;
  userAgentIsMobile: boolean = this.userAgent.indexOf('Mobile') !== -1;

  get xtpConnected(): boolean { return this.xtpConnectedSubject.value; }
  get mediaLoading(): boolean { return this.mediaLoadingSubject.value; }
  get shufflePlayMode(): boolean { return this.shufflePlayModeSubject.value; }

  setXtpConnected(connected: boolean): void {
    this.xtpConnectedSubject.next(connected);
  }

  setMediaLoading(loading: boolean): void {
    this.mediaLoadingSubject.next(loading);
  }

  setShufflePlayMode(enabled: boolean): void {
    this.shufflePlayModeSubject.next(enabled);
  }
}