import { Injectable } from "@angular/core";
import { AppStateService } from "./app-state.service";
import { MediaItem, MediaItemMetaData } from "../models/interfaces";
import { VideoService } from "./video.service";
import { BehaviorSubject, Observable, Subject } from "rxjs";
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
        this.localSettingsService.selectedTags$.subscribe(v => {
            this.selectedTags = v;
            this.applyFilterAndSort();
        });
        this.localSettingsService.thumbSize$.subscribe(v => {
            this.thumbSize = v;
            // this.applyFilterAndSort();
        });
        this.appState.shufflePlayMode$.subscribe(v => {
            this.shufflePlayMode = v;
            if(v) {
                this.playNextVideo();
            }
        });
    }

    setMediaList(list: MediaItem[]): void {
        this.mediaListSubject.next(list);
        this.applyFilterAndSort();
    }
    
    setSelectedMediaItem(item: MediaItem | null): void {
        this.selectedMediaItemSubject.next(item);
    }

    // private get selectedMediaItem(): MediaItem | null { return this.selectedMediaItemSubject.value; }

    private get mediaList(): MediaItem[] | undefined { return this.mediaListSubject.value; }
    private get mediaListDisplayed(): MediaItem[] | undefined { return this.mediaListDisplayedSubject.value; }

    filterCriteria: string = this.localSettingsService.filterCriteriaDefault;
    sortBy: SortMode = this.localSettingsService.sortDefault;
    show: ShowMode = this.localSettingsService.showDefault;
    thumbSize: number = this.localSettingsService.thumbSizeDefault;

    private selectedTags: string[] = [];
    private shufflePlayMode: boolean = false;

    // Media list
    private mediaListSubject = new BehaviorSubject<MediaItem[] | undefined>([]);
    mediaList$: Observable<MediaItem[] | undefined> = this.mediaListSubject.asObservable();

    private mediaListDisplayedSubject = new BehaviorSubject<MediaItem[] | undefined>(undefined);
    mediaListDisplayed$: Observable<MediaItem[] | undefined> = this.mediaListDisplayedSubject.asObservable();

    // Selected media for metadata
    private selectedMediaItemSubject = new BehaviorSubject<MediaItem | null>(null);
    selectedMediaItem$: Observable<MediaItem | null> = this.selectedMediaItemSubject.asObservable();

    // Scroll-to-item request (emitted when a new item starts playing, e.g. shuffle)
    private scrollToItemSubject = new Subject<MediaItem>();
    scrollToItem$: Observable<MediaItem> = this.scrollToItemSubject.asObservable();

    playNextVideo(): void {
        const list = this.mediaListDisplayed;
        if (!list || list.length === 0) return;
        const playing = this.videoService.playingMediaItem;
        let index = 0;
        if (playing !== null) {
            index = list.findIndex(m => m.id === playing.id);
            if (index === -1) {
                index = 0;
            }
        }

        if (this.shufflePlayMode) {
            const next = this.getNextShuffleMediaItem();
            if (next) {
                this.scrollToItemSubject.next(next);
                this.videoService.playVideo(next);
            }
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
        if(!list) return;
        const playing = this.videoService.playingMediaItem;
        if (!playing || list.length === 0) return;

        let index = list.findIndex(m => m.id === playing.id);
        if (index === -1 && list.length > 0) index = list.length;
        let previousIndex = index - 1;
        if(previousIndex < 0)
            previousIndex = list.length - 1
        this.videoService.playVideo(list[previousIndex]);
    }

    onUpdateThumb(message: any): void {
        if(!this.mediaList) return;
        const item = this.mediaList.find(m => m.id === message.id);
        if (item) {
            item.relativeThumb = '/thumb/' + message.thumb;
            this.updateMediaItem(item);
        }
    }

    addItem(item: MediaItem): void {
        if(!this.mediaList) return;
        const list = [...this.mediaList];
        if (!list.find(m => m.id === item.id)) {
        list.push(item);
        this.setMediaList(list);
        }
    }

    deleteItem(itemID: string): void {
        if(!this.mediaList) return;
        const list = this.mediaList.filter(m => m.id !== itemID);
        this.setMediaList(list);
    }

    updateMediaItem(updated: MediaItem): void {
        if(!this.mediaList) return;
        const list = [...this.mediaList];
        const index = list.findIndex(m => m.id === updated.id);
        if (index !== -1) {
        list[index] = updated;
        this.mediaListSubject.next(list);
        this.applyFilterAndSort();
        }
    }

    private getNextShuffleMediaItem(): MediaItem | null {
        const list = this.mediaListDisplayed;
        if (!list || list.length === 0) return null;
        const randomIndex = Math.floor(Math.random() * list.length);
        return list[randomIndex];
    }

    private applyFilterAndSort(): void {
        if(!this.mediaList) return;
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
                    list = this.mediaList.filter(x => x.type === MediaType.VR || x.metaData.tags.includes("vr"));
                    break;
                case ShowMode.TwoD:
                    list = this.mediaList.filter(x => x.type === MediaType.Audio || x.type === MediaType.Video || x.metaData.tags.includes("2d") || x.metaData.tags.includes("audio"));
                    break;
            }
        }

        // Filter by text
        const filter = this.filterCriteria.toLowerCase();
        if (filter) {
            list = list.filter(m => m.name.toLowerCase().includes(filter));
        }

        // Filter by tags
        let tags: string[] = JSON.parse(JSON.stringify(this.selectedTags));
        const orIndex = tags.findIndex(x => x === 'OR');
        const orMode = orIndex > -1;
        if(orMode) {
            tags.splice(orIndex, 1);
        }
        if (tags.length > 0) {
            if(orMode)
                list = list.filter(m => tags.some(t => m.metaData.tags?.includes(t)));
            else
                list = list.filter(m => tags.every(t => m.metaData.tags?.includes(t)));
        }

        // Sort
        const sortBy = this.sortBy;
        if(sortBy == SortMode.Random) {
            this.shuffle(list);
        } else {
            list.sort((a, b) => {
                switch (sortBy) {
                    case SortMode.Name:
                    return a.displayName.localeCompare(b.displayName);
                    case SortMode.NameDesc:
                    return b.displayName.localeCompare(a.displayName);
                    case SortMode.Date:
                    return (a.metaData.dateAdded?.getTime() || 0) - (b.metaData.dateAdded?.getTime() || 0);
                    case SortMode.DateDesc:
                    return (b.metaData.dateAdded?.getTime() || 0) - (a.metaData.dateAdded?.getTime() || 0);
                    case SortMode.DateAdded:
                    return (a.modifiedDate?.getTime() || 0) - (b.modifiedDate?.getTime() || 0);
                    case SortMode.DateAddedDesc:
                    return (b.modifiedDate?.getTime() || 0) - (a.modifiedDate?.getTime() || 0);
                    default:
                    return a.displayName.localeCompare(b.displayName);
                }
            });
        }

        this.mediaListDisplayedSubject.next(list);
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
