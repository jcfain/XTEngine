import { Injectable } from "@angular/core";
import { ShowMode, SortMode } from "../models/enums";
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

    constructor( ) {
        this.sortSubject.next(this.getFromLocalStorage(this.sortKey, this.sortDefault));
        this.showSubject.next(this.getFromLocalStorage(this.showKey, this.showDefault));
        this.thumbSizeSubject.next(this.getFromLocalStorage(this.thumbSizeKey, this.thumbSizeDefault));
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
