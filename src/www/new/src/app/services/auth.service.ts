import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';

@Injectable({
  providedIn: 'root'
})
export class AuthService {
  private isAuthenticatedSubject = new BehaviorSubject<boolean>(false);
  isAuthenticated$: Observable<boolean> = this.isAuthenticatedSubject.asObservable();

  private storedHash: string | null = window.localStorage.getItem('storedHash');
  private rememberPassword = false;

  get isAuthenticated(): boolean {
    return this.isAuthenticatedSubject.value;
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
    this.isAuthenticatedSubject.next(false);
    window.location.replace(window.location.origin);
    
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

  private checkPass(hashedPass: string): Promise<boolean> {
    return new Promise((resolve, reject) => {
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/auth', true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.onreadystatechange = () => {
        if (xhr.readyState === 4) {
          if (xhr.status !== 200) {
            if (xhr.status === 401 && this.storedHash) {
              window.localStorage.removeItem('storedHash');
              this.storedHash = null;
            }
            resolve(false);
          } else {
            if (this.rememberPassword) {
              window.localStorage.setItem('storedHash', hashedPass);
              this.storedHash = hashedPass;
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
      // keccak512 would be called here
      // For now, placeholder - integrate js-sha3
      return password; // TODO: replace with keccak512(password)
    }
    return undefined;
  }
}