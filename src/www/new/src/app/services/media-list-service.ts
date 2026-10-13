import { Injectable } from "@angular/core";
import { AppStateService } from "./app-state.service";
import { MediaItem } from "../models/interfaces";
import { VideoService } from "./video.service";
import { BehaviorSubject, Observable } from "rxjs";

@Injectable({
  providedIn: 'root'
})
export class MediaListService {
  constructor(
    private appState: AppStateService,
    private videoService: VideoService,
  ) {}

    setMediaList(list: MediaItem[]): void {
        this.mediaListSubject.next(list);
        this.applyFilterAndSort();
    }
    
    setSelectedMediaItem(item: MediaItem | null): void {
        this.selectedMediaItemSubject.next(item);
    }

    get mediaList(): MediaItem[] { return this.mediaListSubject.value; }
    get playingMediaItem(): MediaItem | null { return this.playingMediaItemSubject.value; }
    get selectedMediaItem(): MediaItem | null { return this.selectedMediaItemSubject.value; }
    get filterCriteria(): string { return this.filterCriteriaSubject.value; }
    get sortBy(): string { return this.sortBySubject.value; }
    get show(): string { return this.showSubject.value; }
    get thumbSize(): number { return this.thumbSizeSubject.value; }

    // Media list
    private mediaListSubject = new BehaviorSubject<MediaItem[]>([]);
    mediaList$: Observable<MediaItem[]> = this.mediaListSubject.asObservable();

    private mediaListDisplayedSubject = new BehaviorSubject<MediaItem[]>([]);
    mediaListDisplayed$: Observable<MediaItem[]> = this.mediaListDisplayedSubject.asObservable();

    // Playing media
    private playingMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
    playingMediaItem$: Observable<MediaItem | null> = this.playingMediaItemSubject.asObservable();

    // Selected media for metadata
    private selectedMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
    selectedMediaItem$: Observable<MediaItem | null> = this.selectedMediaItemSubject.asObservable();
    
    // Filter
    private filterCriteriaSubject = new BehaviorSubject<string>('');
    filterCriteria$: Observable<string> = this.filterCriteriaSubject.asObservable();

    private tagFilterCriteriaSubject = new BehaviorSubject<string[]>([]);
    tagFilterCriteria$: Observable<string[]> = this.tagFilterCriteriaSubject.asObservable();

    // Sort / Show
    private sortBySubject = new BehaviorSubject<string>(JSON.parse(window.localStorage.getItem('sortBy') || '"name"'));
    sortBy$: Observable<string> = this.sortBySubject.asObservable();

    private showSubject = new BehaviorSubject<string>(JSON.parse(window.localStorage.getItem('show') || '"All"'));
    show$: Observable<string> = this.showSubject.asObservable();

    // Thumb size
    private thumbSizeSubject = new BehaviorSubject<number>(JSON.parse(window.localStorage.getItem('thumbSize') || '200'));
    thumbSize$: Observable<number> = this.thumbSizeSubject.asObservable();

    playNextVideo(): void {
        const list = this.mediaList;
        const playing = this.playingMediaItem;
        if (!playing || list.length === 0) return;

        const index = list.findIndex(m => m.id === playing.id);
        if (index === -1) return;

        if (this.appState.shufflePlayMode) {
        const next = this.getNextShuffleMediaItem();
        if (next) this.videoService.playVideo(next);
        } else {
        const nextIndex = index + 1;
        if (nextIndex < list.length) {
            this.videoService.playVideo(list[nextIndex]);
        }
        }
    }

    playPreviousVideo(): void {
        const list = this.mediaList;
        const playing = this.playingMediaItem;
        if (!playing || list.length === 0) return;

        const index = list.findIndex(m => m.id === playing.id);
        if (index <= 0) return;

        this.videoService.playVideo(list[index - 1]);
    }
    
    setFilterCriteria(criteria: string): void {
        this.filterCriteriaSubject.next(criteria);
        this.applyFilterAndSort();
    }

    setTagFilterCriteria(tags: string[]): void {
        this.tagFilterCriteriaSubject.next(tags);
        this.applyFilterAndSort();
    }

    setSortBy(sortBy: string): void {
        this.sortBySubject.next(sortBy);
        window.localStorage.setItem('sortBy', JSON.stringify(sortBy));
        this.applyFilterAndSort();
    }

    setShow(show: string): void {
        this.showSubject.next(show);
        window.localStorage.setItem('show', JSON.stringify(show));
        this.applyFilterAndSort();
    }
    
    setThumbSize(size: number): void {
        this.thumbSizeSubject.next(size);
        window.localStorage.setItem('thumbSize', JSON.stringify(size));
    }

    onUpdateThumb(message: any): void {
        const item = this.mediaList.find(m => m.id === message.id);
        if (item) {
        item.relativeThumb = '/thumb/' + message.thumb;
        this.updateMediaItem(item);
        }
    }

    addItem(item: MediaItem): void {
        const list = [...this.mediaList];
        if (!list.find(m => m.id === item.id)) {
        list.push(item);
        this.setMediaList(list);
        }
    }

    deleteItem(itemID: number): void {
        const list = this.mediaList.filter(m => m.id !== itemID);
        this.setMediaList(list);
    }

    private getNextShuffleMediaItem(): MediaItem | null {
        const list = this.mediaList;
        if (list.length === 0) return null;
        const randomIndex = Math.floor(Math.random() * list.length);
        return list[randomIndex];
    }

    private applyFilterAndSort(): void {
        let list = [...this.mediaListSubject.value];

        // Filter by show
        const show = this.showSubject.value;
        if (show !== 'All') {
        switch (show) {
            case 'Unwatched':
            list = list.filter(m => !m.viewed || m.viewed === 0);
            break;
            case 'Watched':
            list = list.filter(m => m.viewed && m.viewed > 0);
            break;
        }
        }

        // Filter by text
        const filter = this.filterCriteriaSubject.value.toLowerCase();
        if (filter) {
        list = list.filter(m => m.name.toLowerCase().includes(filter));
        }

        // Filter by tags
        const tags = this.tagFilterCriteriaSubject.value;
        if (tags.length > 0) {
        list = list.filter(m => tags.some(t => m.tags?.includes(t)));
        }

        // Sort
        const sortBy = this.sortBySubject.value;
        list.sort((a, b) => {
        switch (sortBy) {
            case 'name':
            return a.name.localeCompare(b.name);
            case 'name-desc':
            return b.name.localeCompare(a.name);
            case 'date':
            return (b.metaData.dateAdded?.getTime() || 0) - (a.metaData.dateAdded?.getTime() || 0);
            case 'date-desc':
            return (a.metaData.dateAdded?.getTime() || 0) - (b.metaData.dateAdded?.getTime() || 0);
            default:
            return a.name.localeCompare(b.name);
        }
        });

        this.mediaListDisplayedSubject.next(list);
    }

    updateMediaItem(updated: MediaItem): void {
        const list = [...this.mediaListSubject.value];
        const index = list.findIndex(m => m.id === updated.id);
        if (index !== -1) {
        list[index] = updated;
        this.mediaListSubject.next(list);
        this.applyFilterAndSort();
        }
    }
}
