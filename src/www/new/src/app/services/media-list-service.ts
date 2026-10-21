import { Injectable } from "@angular/core";
import { AppStateService } from "./app-state.service";
import { MediaItem, MediaItemMetaData } from "../models/interfaces";
import { VideoService } from "./video.service";
import { BehaviorSubject, Observable } from "rxjs";
import { LocalSettingsService } from "./local-settings.service";
import { MediaType, ShowMode, SortMode } from "../models/enums";
import { HttpClient } from "@angular/common/http";

@Injectable({
  providedIn: 'root'
})
export class MediaListService {

    constructor(
        private localSettingsService: LocalSettingsService,
        private appState: AppStateService,
        private videoService: VideoService,
    ) {
        this.localSettingsService.show$.subscribe(v => {
            this.show = v;
            this.applyFilterAndSort();
        });
        this.localSettingsService.sort$.subscribe(v => {
            this.sortBy = v;
            this.applyFilterAndSort();
        });
        this.localSettingsService.filterCriteria$.subscribe(v => {
            this.filterCriteria = v;
            this.applyFilterAndSort();
        });
        this.localSettingsService.thumbSize$.subscribe(v => {
            this.thumbSize = v;
            // this.applyFilterAndSort();
        });
    }

    setMediaList(list: MediaItem[]): void {
        this.mediaListSubject.next(list);
        this.applyFilterAndSort();
    }
    
    setSelectedMediaItem(item: MediaItem | null): void {
        this.selectedMediaItemSubject.next(item);
    }

    get mediaList(): MediaItem[] { return this.mediaListSubject.value; }
    get mediaListDisplayed(): MediaItem[] { return this.mediaListDisplayedSubject.value; }
    get selectedMediaItem(): MediaItem | null { return this.selectedMediaItemSubject.value; }

    filterCriteria: string = this.localSettingsService.filterCriteriaDefault;
    sortBy: SortMode = this.localSettingsService.sortDefault;
    show: ShowMode = this.localSettingsService.showDefault;
    thumbSize: number = this.localSettingsService.thumbSizeDefault;

    // Media list
    private mediaListSubject = new BehaviorSubject<MediaItem[]>([]);
    mediaList$: Observable<MediaItem[]> = this.mediaListSubject.asObservable();

    private mediaListDisplayedSubject = new BehaviorSubject<MediaItem[]>([]);
    mediaListDisplayed$: Observable<MediaItem[]> = this.mediaListDisplayedSubject.asObservable();

    // Selected media for metadata
    private selectedMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
    selectedMediaItem$: Observable<MediaItem | null> = this.selectedMediaItemSubject.asObservable();

    private tagFilterCriteriaSubject = new BehaviorSubject<string[]>([]);
    tagFilterCriteria$: Observable<string[]> = this.tagFilterCriteriaSubject.asObservable();


    playNextVideo(): void {
        const list = this.mediaListDisplayed;
        const playing = this.videoService.playingMediaItem;
        if (!playing || list.length === 0) return;

        let index = list.findIndex(m => m.id === playing.id);

        if (this.appState.shufflePlayMode) {
            const next = this.getNextShuffleMediaItem();
            if (next) 
                this.videoService.playVideo(next);
        } else {
            let nextIndex = index + 1;
            if (nextIndex >= list.length) {
                nextIndex = 0;
            }
            this.videoService.playVideo(list[nextIndex]);
        }
    }

    playPreviousVideo(): void {
        const list = this.mediaListDisplayed;
        const playing = this.videoService.playingMediaItem;
        if (!playing || list.length === 0) return;

        let index = list.findIndex(m => m.id === playing.id);
        if (index === -1 && list.length > 0) index = list.length;
        let previousIndex = index - 1;
        if(previousIndex < 0)
            previousIndex = list.length - 1
        this.videoService.playVideo(list[previousIndex]);
    }

    setTagFilterCriteria(tags: string[]): void {
        this.tagFilterCriteriaSubject.next(tags);
        this.applyFilterAndSort();
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

    deleteItem(itemID: string): void {
        const list = this.mediaList.filter(m => m.id !== itemID);
        this.setMediaList(list);
    }

    private getNextShuffleMediaItem(): MediaItem | null {
        const list = this.mediaListDisplayed;
        if (list.length === 0) return null;
        const randomIndex = Math.floor(Math.random() * list.length);
        return list[randomIndex];
    }

    private applyFilterAndSort(): void {
        let list = [...this.mediaList];

        // Filter by show
        const show = this.show;
        if (show !== ShowMode.All) {
            switch (show) {
                // case 'Unwatched':
                // list = list.filter(m => !m.viewed || m.viewed === 0);
                // break;
                // case 'Watched':
                // list = list.filter(m => m.viewed && m.viewed > 0);
                // break;
                // case ShowMode.All:
                //     list = JSON.parse(JSON.stringify(this.mediaListSubject.value));
                //     break;
                case ShowMode.ThreeD:
                    list = this.mediaListSubject.value.filter(x => x.type === MediaType.VR || x.metaData.tags.includes("vr"));
                    break;
                case ShowMode.TwoD:
                    list = this.mediaListSubject.value.filter(x => x.type === MediaType.Audio || x.type === MediaType.Video || x.metaData.tags.includes("2d") || x.metaData.tags.includes("audio"));
                    break;
            }
        }

        // Filter by text
        const filter = this.filterCriteria.toLowerCase();
        if (filter) {
            list = list.filter(m => m.name.toLowerCase().includes(filter));
        }

        // Filter by tags
        const tags = this.tagFilterCriteriaSubject.value;
        if (tags.length > 0) {
            list = list.filter(m => tags.some(t => m.tags?.includes(t)));
        }

        // Sort
        const sortBy = this.sortBy;
        if(sortBy == SortMode.Random)
        {
            this.shuffle(list);
        }
        list.sort((a, b) => {
            switch (sortBy) {
                case SortMode.Name:
                return a.name.localeCompare(b.name);
                case SortMode.NameDesc:
                return b.name.localeCompare(a.name);
                case SortMode.Date:
                return (b.metaData.dateAdded?.getTime() || 0) - (a.metaData.dateAdded?.getTime() || 0);
                case SortMode.DateDesc:
                return (a.metaData.dateAdded?.getTime() || 0) - (b.metaData.dateAdded?.getTime() || 0);
                case SortMode.DateAdded:
                return (b.modifiedDate?.getTime() || 0) - (a.modifiedDate?.getTime() || 0);
                case SortMode.DateAddedDesc:
                return (a.modifiedDate?.getTime() || 0) - (b.modifiedDate?.getTime() || 0);
                default:
                return a.name.localeCompare(b.name);
            }
        });

        this.mediaListDisplayedSubject.next(list);
    }

    updateMediaItem(updated: MediaItem): void {
        const list = [...this.mediaList];
        const index = list.findIndex(m => m.id === updated.id);
        if (index !== -1) {
        list[index] = updated;
        this.mediaListSubject.next(list);
        this.applyFilterAndSort();
        }
    }

    // //https://stackoverflow.com/questions/2450954/how-to-randomize-shuffle-a-javascript-array
    // private  fy(a: any, b?: any, c?: any, d?: any) {//array,placeholder,placeholder,placeholder
	//     c=a.length;while(c)b=Math.random()*c--|0,d=a[c],a[c]=a[b],a[b]=d
    // }

    // https://stackoverflow.com/questions/2450954/how-to-randomize-shuffle-a-javascript-array
    // This method will modify the array passed in
    private shuffle(mediaItems: MediaItem[]) {
        let currentIndex = mediaItems.length;

        // While there remain elements to shuffle...
        while (currentIndex != 0) {

            // Pick a remaining element...
            let randomIndex = Math.floor(Math.random() * currentIndex);
            currentIndex--;

            // And swap it with the current element.
            [mediaItems[currentIndex], mediaItems[randomIndex]] = [
            mediaItems[randomIndex], mediaItems[currentIndex]];
        }
    }
}
