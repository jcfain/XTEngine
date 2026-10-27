import { Injectable } from "@angular/core";
import { ConnectionInterface, ShowMode, SortMode } from "../models/enums";
import { BehaviorSubject, Observable } from "rxjs";

@Injectable({
  providedIn: 'root'
})
export class LocalSettingsService {
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

    constructor( ) {
        this.sortSubject.next(this.getFromLocalStorage(this.sortKey, this.sortDefault));
        this.showSubject.next(this.getFromLocalStorage(this.showKey, this.showDefault));
        this.thumbSizeSubject.next(this.getFromLocalStorage(this.thumbSizeKey, this.thumbSizeDefault));
        this.filterCriteriaSubject.next(this.getFromLocalStorage(this.filterCriteriaKey, this.filterCriteriaDefault));
        this.volumeSubject.next(this.getFromLocalStorage(this.volumeKey, this.volumeDefault));
        this.muteSubject.next(this.getFromLocalStorage(this.muteKey, this.muteDefault));
        this.selectedTagsSubject.next(this.getFromLocalStorage(this.selectedTagsKey, this.selectedTagsDefault));
        this.selectedInputDeviceSubject.next(this.getFromLocalStorage(this.selectedInputDeviceKey, this.selectedInputDeviceDefault));
        this.selectedOuputDeviceSubject.next(this.getFromLocalStorage(this.selectedOuputDeviceKey, this.selectedOuputDeviceDefault));
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

    private getFromLocalStorage<T>(key: string, defaultValue: T): T {
        const valueInStorage = window.localStorage.getItem(key);
        if(valueInStorage == null) {
            this.setToLocalStorage(key, defaultValue);
            return defaultValue;
        }
        return JSON.parse(valueInStorage) as T;
    }

    private setToLocalStorage<T>(key: string, value: T) {
        window.localStorage.setItem(key, JSON.stringify(value));
    }
}
