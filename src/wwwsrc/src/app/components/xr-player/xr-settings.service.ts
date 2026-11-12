// XR-specific local settings service.
//
// Mimics the web app's LocalSettingsService (a BehaviorSubject per setting,
// localStorage persistence, defaults, and a reset) but is scoped to the
// self-contained xr-player feature: every key is namespaced under "xtpXr" so
// it never collides with the web app's local settings, and resetXrSettings()
// only ever touches the XR keys. Removing the feature = delete this file.
import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';

// Persisted position/rotation/visibility of a single 3D panel.
export interface XrPanelLayoutEntry {
    pos: [number, number, number];
    rotY: number;
    rotX: number;
    visible: boolean;
}
// Panel id -> layout entry.
export type XrPanelLayout = Record<string, XrPanelLayoutEntry>;

@Injectable({
    providedIn: 'root'
})
export class XrSettingsService {

    readonly passthroughDefault = false;
    private passthroughKey = 'xtpXrPassthrough';
    private passthroughSubject = new BehaviorSubject<boolean>(this.passthroughDefault);
    passthrough$: Observable<boolean> = this.passthroughSubject.asObservable();

    readonly stereoStrengthDefault = 1.0;
    private stereoStrengthKey = 'xtpXrStereoStrength';
    private stereoStrengthSubject = new BehaviorSubject<number>(this.stereoStrengthDefault);
    stereoStrength$: Observable<number> = this.stereoStrengthSubject.asObservable();

    readonly layoutDefault: XrPanelLayout = {};
    private layoutKey = 'xtpXrLayout.v2';
    private layoutSubject = new BehaviorSubject<XrPanelLayout>(this.layoutDefault);
    layout$: Observable<XrPanelLayout> = this.layoutSubject.asObservable();

    constructor() {
        this.passthroughSubject.next(this.getFromLocalStorage(this.passthroughKey, this.passthroughDefault));
        this.stereoStrengthSubject.next(this.getFromLocalStorage(this.stereoStrengthKey, this.stereoStrengthDefault));
        this.layoutSubject.next(this.getFromLocalStorage(this.layoutKey, this.layoutDefault));
    }

    setPassthrough(value: boolean): void {
        this.setToLocalStorage(this.passthroughKey, value);
        this.passthroughSubject.next(value);
    }

    setStereoStrength(value: number): void {
        this.setToLocalStorage(this.stereoStrengthKey, value);
        this.stereoStrengthSubject.next(value);
    }

    setLayout(value: XrPanelLayout): void {
        this.setToLocalStorage(this.layoutKey, value);
        this.layoutSubject.next(value);
    }

    // Reset ONLY the XR settings: remove every XR key from localStorage and
    // restore the in-memory subjects to their defaults. The web app's local
    // settings (LocalSettingsService) are completely untouched.
    resetXrSettings(): void {
        window.localStorage.removeItem(this.passthroughKey);
        window.localStorage.removeItem(this.stereoStrengthKey);
        window.localStorage.removeItem(this.layoutKey);
        this.passthroughSubject.next(this.passthroughDefault);
        this.stereoStrengthSubject.next(this.stereoStrengthDefault);
        this.layoutSubject.next(this.layoutDefault);
    }

    private getFromLocalStorage<T>(key: string, defaultValue: T): T {
        const valueInStorage = window.localStorage.getItem(key);
        if (valueInStorage == null) {
            return defaultValue;
        }
        try {
            return JSON.parse(valueInStorage) as T;
        } catch {
            return defaultValue;
        }
    }

    private setToLocalStorage<T>(key: string, value: T): void {
        window.localStorage.setItem(key, JSON.stringify(value));
    }
}