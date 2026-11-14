import { Injectable } from '@angular/core';
import { Observable, Subject } from 'rxjs';
import { AuthService } from './auth.service';

/**
 * Bridges the hamburger menu (which lives deep inside the media list toolbar)
 * to the app-level dialogs and actions that are owned by the root component
 * (settings, VR player) plus the auth logout. Using a service avoids threading
 * @Output events up through the media list and toolbar to the root.
 */
@Injectable({ providedIn: 'root' })
export class AppMenuService {
  private settingsToggleSubject = new Subject<void>();
  settingsToggle$: Observable<void> = this.settingsToggleSubject.asObservable();

  private vrOpenSubject = new Subject<void>();
  vrOpen$: Observable<void> = this.vrOpenSubject.asObservable();

  constructor(private authService: AuthService) {}

  openSettings(): void {
    this.settingsToggleSubject.next();
  }

  openVr(): void {
    this.vrOpenSubject.next();
  }

  logout(): void {
    this.authService.logout();
  }
}