import { Injectable } from "@angular/core";
import { ConnectionInterface, ShowMode, SortMode } from "../models/enums";
import { BehaviorSubject, Observable } from "rxjs";

@Injectable({
  providedIn: 'root'
})
export class LocalSettingsService {

    readonly storedHashDefault = null;
    private storedHashKey = "storedHash";
    private storedHashSubject = new BehaviorSubject<string | null>(this.storedHashDefault);
    storedHash$: Observable<string | null> = this.storedHashSubject.asObservable();

    readonly sortDefault: SortMode = SortMode.Name;
    private sortKey = "sortBy";
    private sortSubject = new BehaviorSubject<SortMode>(this.sortDefault);
    sort$: Observable<SortMode> = this.sortSubject.asObservable();

    readonly showDefault = ShowMode.All;
    private showKey = "show";
    private showSubject = new BehaviorSubject<ShowMode>(this.showDefault);
    show$: Observable<ShowMode> = this.showSubject.asObservable();

    readonly thumbSizeDefault = 200;
    private thumbSizeKey = "thumbSize";
    private thumbSizeSubject = new BehaviorSubject<number>(this.thumbSizeDefault);
    thumbSize$: Observable<number> = this.thumbSizeSubject.asObservable();

    readonly filterCriteriaDefault = "";
    private filterCriteriaKey = "filter";
    private filterCriteriaSubject = new BehaviorSubject<string>(this.filterCriteriaDefault);
    filterCriteria$: Observable<string> = this.filterCriteriaSubject.asObservable();

    readonly volumeDefault = 0.5;
    private volumeKey = "volume";
    private volumeSubject = new BehaviorSubject<number>(this.volumeDefault);
    volume$: Observable<number> = this.volumeSubject.asObservable();

    readonly muteDefault = false;
    private muteKey = "mute";
    private muteSubject = new BehaviorSubject<boolean>(this.muteDefault);
    mute$: Observable<boolean> = this.muteSubject.asObservable();

    readonly selectedTagsDefault = [];
    private selectedTagsKey = "selectedTags";
    private selectedTagsSubject = new BehaviorSubject<string[]>(this.selectedTagsDefault);
    selectedTags$: Observable<string[]> = this.selectedTagsSubject.asObservable();

    readonly selectedInputDeviceDefault = ConnectionInterface.None;
    private selectedInputDeviceKey = "selectedInputDevice";
    private selectedInputDeviceSubject = new BehaviorSubject<ConnectionInterface>(this.selectedInputDeviceDefault);
    selectedInputDevice$: Observable<ConnectionInterface> = this.selectedInputDeviceSubject.asObservable();

    readonly selectedOuputDeviceDefault = ConnectionInterface.None;
    private selectedOuputDeviceKey = "selectedOuputDevice";
    private selectedOuputDeviceSubject = new BehaviorSubject<ConnectionInterface>(this.selectedOuputDeviceDefault);
    selectedOuputDevice$: Observable<ConnectionInterface> = this.selectedOuputDeviceSubject.asObservable();

    readonly externalStreamingDefault = false;
    private externalStreamingKey = "externalStreaming";
    private externalStreamingSubject = new BehaviorSubject<boolean>(this.externalStreamingDefault);
    externalStreaming$: Observable<boolean> = this.externalStreamingSubject.asObservable();

    readonly disableBrowserLazyLoadDefault = false;
    private disableBrowserLazyLoadKey = "disableBrowserLazyLoad";
    private disableBrowserLazyLoadSubject = new BehaviorSubject<boolean>(this.disableBrowserLazyLoadDefault);
    disableBrowserLazyLoad$: Observable<boolean> = this.disableBrowserLazyLoadSubject.asObservable();

    readonly disableFullScreenMediaListDefault = false;
    private disableFullScreenMediaListKey = "disableFullScreenMediaList";
    private disableFullScreenMediaListSubject = new BehaviorSubject<boolean>(this.disableFullScreenMediaListDefault);
    disableFullScreenMediaList$: Observable<boolean> = this.disableFullScreenMediaListSubject.asObservable();

    readonly disableHeatmapDefault = false;
    private disableHeatmapKey = "disableHeatmap";
    private disableHeatmapSubject = new BehaviorSubject<boolean>(this.disableHeatmapDefault);
    disableHeatmap$: Observable<boolean> = this.disableHeatmapSubject.asObservable();

    readonly enableTextToSpeechDefault = true;
    private enableTextToSpeechKey = "enableTextToSpeech";
    private enableTextToSpeechSubject = new BehaviorSubject<boolean>(this.enableTextToSpeechDefault);
    enableTextToSpeech$: Observable<boolean> = this.enableTextToSpeechSubject.asObservable();

    readonly selectedVoiceIndexDefault = 0;
    private selectedVoiceIndexKey = "selectedVoiceIndex";
    private selectedVoiceIndexSubject = new BehaviorSubject<number>(this.selectedVoiceIndexDefault);
    selectedVoiceIndex$: Observable<number> = this.selectedVoiceIndexSubject.asObservable();

    readonly speechPitchDefault = 1;
    private speechPitchKey = "speechPitch";
    private speechPitchSubject = new BehaviorSubject<number>(this.speechPitchDefault);
    speechPitch$: Observable<number> = this.speechPitchSubject.asObservable();

    readonly speechRateDefault = 1;
    private speechRateKey = "speechRate";
    private speechRateSubject = new BehaviorSubject<number>(this.speechRateDefault);
    speechRate$: Observable<number> = this.speechRateSubject.asObservable();

    readonly speechVolumeDefault = 0.5;
    private speechVolumeKey = "speechVolume";
    private speechVolumeSubject = new BehaviorSubject<number>(this.speechVolumeDefault);
    speechVolume$: Observable<number> = this.speechVolumeSubject.asObservable();

    readonly lastSelectedPropertiesTabDefault = 0;
    private lastSelectedPropertiesTabKey = "lastSelectedPropertiesTab";
    private lastSelectedPropertiesTabSubject = new BehaviorSubject<number>(this.lastSelectedPropertiesTabDefault);
    lastSelectedPropertiesTab$: Observable<number> = this.lastSelectedPropertiesTabSubject.asObservable();

    constructor( ) {
        this.storedHashSubject.next(this.getStoredHash());
        this.sortSubject.next(this.getFromLocalStorage(this.sortKey, this.sortDefault));
        this.showSubject.next(this.getFromLocalStorage(this.showKey, this.showDefault));
        this.thumbSizeSubject.next(this.getFromLocalStorage(this.thumbSizeKey, this.thumbSizeDefault));
        this.filterCriteriaSubject.next(this.getFromLocalStorage(this.filterCriteriaKey, this.filterCriteriaDefault));
        this.volumeSubject.next(this.getFromLocalStorage(this.volumeKey, this.volumeDefault));
        this.muteSubject.next(this.getFromLocalStorage(this.muteKey, this.muteDefault));
        this.selectedTagsSubject.next(this.getFromLocalStorage(this.selectedTagsKey, this.selectedTagsDefault));
        this.selectedInputDeviceSubject.next(this.getFromLocalStorage(this.selectedInputDeviceKey, this.selectedInputDeviceDefault));
        this.selectedOuputDeviceSubject.next(this.getFromLocalStorage(this.selectedOuputDeviceKey, this.selectedOuputDeviceDefault));
        this.externalStreamingSubject.next(this.getFromLocalStorage(this.externalStreamingKey, this.externalStreamingDefault));
        this.disableBrowserLazyLoadSubject.next(this.getFromLocalStorage(this.disableBrowserLazyLoadKey, this.externalStreamingDefault));
        this.disableHeatmapSubject.next(this.getFromLocalStorage(this.disableHeatmapKey, this.disableHeatmapDefault));
        this.enableTextToSpeechSubject.next(this.getFromLocalStorage(this.enableTextToSpeechKey, this.enableTextToSpeechDefault));
        this.selectedVoiceIndexSubject.next(this.getFromLocalStorage(this.selectedVoiceIndexKey, this.selectedVoiceIndexDefault));
        this.speechPitchSubject.next(this.getFromLocalStorage(this.speechPitchKey, this.speechPitchDefault));
        this.speechRateSubject.next(this.getFromLocalStorage(this.speechRateKey, this.speechRateDefault));
        this.speechVolumeSubject.next(this.getFromLocalStorage(this.speechVolumeKey, this.speechVolumeDefault));
        this.lastSelectedPropertiesTabSubject.next(this.getFromLocalStorage(this.lastSelectedPropertiesTabKey, this.lastSelectedPropertiesTabDefault));
    }

    setStoredHash(value: string | null): void {
        if (value === null) {
            window.localStorage.removeItem(this.storedHashKey);
        } else {
            // "storedHash" is shared with the legacy (non-Angular) site, which
            // stores and reads it as a raw string (see auth.js). Keep it raw so
            // both front-ends stay interoperable.
            window.localStorage.setItem(this.storedHashKey, value);
        }
        this.storedHashSubject.next(value);
    }

    setSort(value: SortMode): void {
        this.setToLocalStorage(this.sortKey, value);
        this.sortSubject.next(value);
    }

    setShow(value: ShowMode): void {
        this.setToLocalStorage(this.showKey, value);
        this.showSubject.next(value);
    }

    setThumbSize(value: number): void {
        this.setToLocalStorage(this.thumbSizeKey, value);
        this.thumbSizeSubject.next(value);
    }

    setFilter(value: string): void {
        this.setToLocalStorage(this.filterCriteriaKey, value);
        this.filterCriteriaSubject.next(value);
    }

    setVolume(value: number): void {
        this.setToLocalStorage(this.volumeKey, value);
        this.volumeSubject.next(value);
    }

    setMute(value: boolean): void {
        this.setToLocalStorage(this.muteKey, value);
        this.muteSubject.next(value);
    }

    setSelectedTags(value: string[]): void {
        this.setToLocalStorage(this.selectedTagsKey, value);
        this.selectedTagsSubject.next(value);
    }

    addToSelectedTags(value: string): void {
        let currentTags = this.selectedTagsSubject.value;
        if(currentTags.findIndex(x => x === value) > -1) {
            return;
        }
        currentTags.push(value);
        this.setSelectedTags(currentTags);
    }

    removeFromSelectedTags(value: string): void {
        let currentTags: string[] = JSON.parse(JSON.stringify(this.selectedTagsSubject.value));
        const index = currentTags.findIndex(x => x === value);
        if(index == -1) {
            return;
        }
        currentTags.splice(index, 1);
        this.setSelectedTags(currentTags);
    }

    setSelectedInputDevice(value: ConnectionInterface): void {
        this.setToLocalStorage(this.selectedInputDeviceKey, value);
        this.selectedInputDeviceSubject.next(value);
    }

    setSelectedOutputDevice(value: ConnectionInterface): void {
        this.setToLocalStorage(this.selectedOuputDeviceKey, value);
        this.selectedOuputDeviceSubject.next(value);
    }

    setExternalStreamingDefault(value: boolean): void {
        this.setToLocalStorage(this.externalStreamingKey, value);
        this.externalStreamingSubject.next(value);
    }

    setDisableBrowserLazyLoad(value: boolean): void {
        this.setToLocalStorage(this.disableBrowserLazyLoadKey, value);
        this.disableBrowserLazyLoadSubject.next(value);
    }

    setDisableFullScreenMediaList(value: boolean): void {
        this.setToLocalStorage(this.disableFullScreenMediaListKey, value);
        this.disableFullScreenMediaListSubject.next(value);
    }

    setDisableHeatmap(value: boolean): void {
        this.setToLocalStorage(this.disableHeatmapKey, value);
        this.disableHeatmapSubject.next(value);
    }

    setEnableTextToSpeech(value: boolean): void {
        this.setToLocalStorage(this.enableTextToSpeechKey, value);
        this.enableTextToSpeechSubject.next(value);
    }

    setSelectedVoiceIndex(value: number): void {
        this.setToLocalStorage(this.selectedVoiceIndexKey, value);
        this.selectedVoiceIndexSubject.next(value);
    }

    setSpeechPitch(value: number): void {
        this.setToLocalStorage(this.speechPitchKey, value);
        this.speechPitchSubject.next(value);
    }

    setSpeechRate(value: number): void {
        this.setToLocalStorage(this.speechRateKey, value);
        this.speechRateSubject.next(value);
    }

    setSpeechVolume(value: number): void {
        this.setToLocalStorage(this.speechVolumeKey, value);
        this.speechVolumeSubject.next(value);
    }

    setLastSelectedPropertiesTab(value: number): void {
        this.setToLocalStorage(this.lastSelectedPropertiesTabKey, value);
        this.lastSelectedPropertiesTabSubject.next(value);
    }

    reset() {
        window.localStorage.clear();
        window.location.reload();
    }

    private getStoredHash(): string | null {
        const valueInStorage = window.localStorage.getItem(this.storedHashKey);
        if (valueInStorage == null) {
            return this.storedHashDefault;
        }
        // Accept both the legacy raw string and a JSON-quoted string (written by
        // an earlier version of this app) so existing stored hashes keep working.
        try {
            return JSON.parse(valueInStorage) as string;
        } catch {
            return valueInStorage;
        }
    }

    private getFromLocalStorage<T>(key: string, defaultValue: T): T {
        const valueInStorage = window.localStorage.getItem(key);
        if(valueInStorage == null) {
            this.setToLocalStorage(key, defaultValue);
            return defaultValue;
        }
        try {
            return JSON.parse(valueInStorage) as T;
        } catch {
            // Tolerate values written by the legacy (non-Angular) site, which
            // stored some values (e.g. storedHash) as raw strings.
            return valueInStorage as unknown as T;
        }
    }

    private setToLocalStorage<T>(key: string, value: T) {
        window.localStorage.setItem(key, JSON.stringify(value));
    }
}
