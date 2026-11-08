import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { ConnectionStatus } from '../models/enums';

@Injectable({
  providedIn: 'root'
})
export class AppStateService {

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

  // get mediaLoading(): boolean { return this.mediaLoadingSubject.value; }
  get shufflePlayMode(): boolean { return this.shufflePlayModeSubject.value; }

  setMediaLoading(loading: boolean): void {
    this.mediaLoadingSubject.next(loading);
  }

  setShufflePlayMode(enabled: boolean): void {
    this.shufflePlayModeSubject.next(enabled);
  }
}