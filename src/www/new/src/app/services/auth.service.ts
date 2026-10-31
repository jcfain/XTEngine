import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { keccak512 } from 'js-sha3';
import { ActiveSession } from '../models/interfaces';
import { ServerService } from './server.service';
import { GenericOverlayService } from './overlay.service';
import { deleteCookie, parseCookie } from '../utils/utils';
import { LocalSettingsService } from './local-settings.service';
import { AlertComponent } from '../components/alert-model/alert.component';

@Injectable({
  providedIn: 'root'
})
export class AuthService {
  private isAuthenticatedSubject = new BehaviorSubject<boolean>(false);
  isAuthenticated$: Observable<boolean> = this.isAuthenticatedSubject.asObservable();

  private sessionIDSubject = new BehaviorSubject<string | undefined>(undefined);
  sessionID$: Observable<string | undefined> = this.sessionIDSubject.asObservable();

  private sessionsSubject = new BehaviorSubject<ActiveSession[]>([]);
  sessions$: Observable<ActiveSession[]> = this.sessionsSubject.asObservable();
  
  private rememberPassword = false;
  private storedHash: string | null = null;
  private sessionAlertNameID = 'systemSessionAlert';

  get isAuthenticated(): boolean {
    return this.isAuthenticatedSubject.value;
  }

  constructor(
    private localSettingsService: LocalSettingsService,
    private serverService: ServerService,
    private overlayService: GenericOverlayService
  ) {
    this.localSettingsService.storedHash$.subscribe(v => {
      this.storedHash = v;
    });

    this.serverService.remoteUserSettings$.subscribe(v => {
      this.loadSessions();
    })
    const cookie = parseCookie(document.cookie)['sessionID'];
    if(cookie)
      this.isAuthenticatedSubject.next(true);
    this.sessionIDSubject.next(cookie);
  }

  login(password: string, remember: boolean): Promise<boolean> {
    this.rememberPassword = remember;
    const hashedPass = this.hashPassword(password);
    if (!hashedPass) {
      return Promise.resolve(false);
    }
    return this.checkPass(hashedPass);
  }

  autoLogin(): Promise<boolean> {
    if (this.storedHash && this.storedHash.trim().length) {
      return this.checkPass(this.storedHash);
    }
    return Promise.resolve(false);
  }

  logout(): void {
    this.serverService.logout().subscribe({
          next: (sessions) => {
            this.isAuthenticatedSubject.next(false);
            deleteCookie("sessionID");
            this.localSettingsService.setStoredHash(null);
            window.location.replace(window.location.origin);
          },
          error: (err) => {
              console.error('Error getting server sessions:', err);
          }
      });
    
    // var xhr = new XMLHttpRequest();
    // xhr.open('GET', "/logout", true);
    // xhr.responseType = 'json';
    // xhr.onload = function () {
    //   var status = xhr.status;
    //   if (status === 200) {
    //     window.localStorage.removeItem("storedHash");
    //     window.sessionStorage.clear();
    //     delete_cookie("sessionID");
    //     cookies.sessionID = undefined;
    //     window.location.replace(window.location.origin);
    //   } else {
    //     parseHttpError("Error logging out", xhr);
    //   }
    // };
    // xhr.onerror = function () {
    //   parseHttpError("Error logging out", xhr);
    // };
    // xhr.send();
  }

  // Authorized browser sessions
  loadSessions(): void {
      if (!this.sessionIDSubject.value) return;
      this.serverService.getActiveSessions().subscribe({
          next: (sessions) => {
              this.sessionsSubject.next(sessions);
          },
          error: (err) => {
              console.error('Error getting server sessions:', err);
          }
      });
  }

  deauthorizeSession(sessionID: string): void {
      if (sessionID === this.sessionIDSubject.value) {
          this.overlayService.open(this.sessionAlertNameID, AlertComponent, {
              content: 'Are you sure you want to delete the current session? This will log you out.',
              titleText: 'Logout?',
              acceptText: 'Logout',
              rejectText: 'Cancel',
              onAccept: () => this.expireSession(sessionID),
              onClose: () => this.overlayService.close(this.sessionAlertNameID)
          });
      } else {
          this.expireSession(sessionID);
      }
  }

  private expireSession(sessionID: string): void {
      this.serverService.expireSession(sessionID).subscribe({
          next: () => {
            let sessions = this.sessionsSubject.value;
              const index = sessions.findIndex(s => s.id === sessionID);
              if (index > -1) {
                  sessions.splice(index, 1);
                  this.sessionsSubject.next(sessions);
              }
              if (sessionID === this.sessionIDSubject.value) {
                  deleteCookie('sessionID');
                  this.sessionIDSubject.next(undefined);
                  this.logout();
              }
          },
          error: (err) => {
              console.error('Error deleting session:', err);
          }
      });
  }

  private checkPass(hashedPass: string): Promise<boolean> {
    return new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/auth', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = () => {
        if (xhr.readyState === 4) {
          if (xhr.status !== 200) {
            if (xhr.status === 401 && this.storedHash) {
              // window.localStorage.removeItem('storedHash');
              this.localSettingsService.setStoredHash(null);
            }
            resolve(false);
          } else {
            if (this.rememberPassword) {
              this.localSettingsService.setStoredHash(hashedPass);
            }
            this.isAuthenticatedSubject.next(true);
            resolve(true);
          }
        }
      };
      xhr.onerror = () => resolve(false);
      xhr.send(JSON.stringify({ hashedPass, remember: this.rememberPassword }));
    });
  }

  private hashPassword(password: string): string | undefined {
    if (password.trim().length) {
      return keccak512(password);
    }
    return undefined;
  }
}