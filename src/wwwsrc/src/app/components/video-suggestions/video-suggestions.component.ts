import { Component, ElementRef, AfterViewInit, ChangeDetectionStrategy, ChangeDetectorRef, OnDestroy, OnInit, NgZone } from '@angular/core';
import { CdkFixedSizeVirtualScroll, CdkVirtualScrollViewport } from '@angular/cdk/scrolling';
import { MediaItem } from '../../models/interfaces';
import { VideoService } from '../../services/video.service';
import { VideoSuggestionsService } from '../../services/video-suggestions.service';
import { MediaListService } from '../../services/media-list-service';
import { SuggestionsOverlayService } from '../../services/suggestions-overlay.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { LazySrcDirective } from '../../directives/lazy-src';
import { getMediaThumbPath } from '../../utils/utils';

/**
 * The "Similar" suggestions panel. It renders in three shapes:
 *  - side: a vertical, virtualized list to the right of the video (wide screens)
 *  - drawer: a horizontal strip below the video (narrow screens / external player)
 *  - full screen: the drawer is re-parented into the video container and pinned
 *    to the top of the media list sheet (see SuggestionsOverlayService).
 * All three render the same content; only the container layout changes.
 */
@Component({
    selector: 'app-video-suggestions',
    imports: [CdkVirtualScrollViewport, CdkFixedSizeVirtualScroll, LazySrcDirective],
    templateUrl: './video-suggestions.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './video-suggestions.component.css',
    host: {
        // In the full screen media sheet: render as a flex item between the
        // toolbar and the list (see the .suggestions-in-sheet CSS).
        '[class.suggestions-in-sheet]': 'overlayActive',
        // Hidden entirely when nothing is playing (no suggestions to show).
        '[class.suggestions-hidden]': '!hasPlaying',
    }
})
export class VideoSuggestionsComponent implements OnInit, AfterViewInit, OnDestroy {
    // Vertical list row height - must match .suggestion-row in the CSS.
    readonly rowHeight = 105;
    // The horizontal drawer renders a bounded number of cards (lazy thumbs keep
    // the rest cheap).
    private static readonly DRAWER_ITEM_COUNT = 40;
    private static readonly WIDE_QUERY = '(min-width: 1100px)';

    suggestions: MediaItem[] = [];
    playingItemId: string | null = null;
    // side = vertical panel next to the video; drawer = horizontal strip.
    isSide = false;
    // True while re-parented into the full screen media sheet.
    overlayActive = false;

    private wideQuery: MediaQueryList;
    private external = false;
    private isFullScreen = false;
    private onWideChange = (): void => {
        // matchMedia change events fire outside the Angular zone.
        this.ngZone.run(() => this.updateMode());
    };

    constructor(
        private videoService: VideoService,
        private videoSuggestionsService: VideoSuggestionsService,
        private mediaListService: MediaListService,
        private suggestionsOverlayService: SuggestionsOverlayService,
        private localSettingsService: LocalSettingsService,
        private elementRef: ElementRef<HTMLElement>,
        private changeDetectorRef: ChangeDetectorRef,
        private ngZone: NgZone
    ) {
        this.wideQuery = window.matchMedia(VideoSuggestionsComponent.WIDE_QUERY);
    }

    ngOnInit(): void {
        this.videoSuggestionsService.suggestions$.subscribe(list => {
            this.suggestions = list;
        });
        this.videoService.playingMediaItem$.subscribe(item => {
            this.playingItemId = item?.id ?? null;
        });
        this.videoService.isFullScreen$.subscribe(v => {
            this.isFullScreen = v;
            this.updateMode();
        });
        this.localSettingsService.externalStreaming$.subscribe(v => {
            this.external = v;
            this.updateMode();
        });
        this.suggestionsOverlayService.overlayActive$.subscribe(v => {
            this.overlayActive = v;
        });
        this.wideQuery.addEventListener('change', this.onWideChange);
        this.updateMode();
    }

    ngAfterViewInit(): void {
        // Register this element with the overlay service: while full screen it
        // is re-parented into the video container and back again on exit.
        this.suggestionsOverlayService.registerPanelElement(this.elementRef.nativeElement);
    }

    ngOnDestroy(): void {
        this.wideQuery.removeEventListener('change', this.onWideChange);
    }

    get hasPlaying(): boolean {
        return this.playingItemId !== null;
    }

    get drawerItems(): MediaItem[] {
        return this.suggestions.slice(0, VideoSuggestionsComponent.DRAWER_ITEM_COUNT);
    }

    private updateMode(): void {
        // side only on wide, in-page (not full screen) non-external layouts.
        this.isSide = this.wideQuery.matches && !this.external && !this.isFullScreen;
        this.changeDetectorRef.markForCheck();
    }

    thumbPath(item: MediaItem): string {
        return getMediaThumbPath(item);
    }

    visibleTags(item: MediaItem): string[] {
        return (item.metaData?.tags ?? []).slice(0, 3);
    }

    isPlaying(item: MediaItem): boolean {
        return item.id === this.playingItemId;
    }

    play(item: MediaItem): void {
        this.videoService.playVideo(item);
        // Scroll the media list to the item so the user can see where it is.
        this.mediaListService.scrollToItem(item);
    }

    reshuffle(): void {
        this.videoSuggestionsService.reshuffle();
    }
}