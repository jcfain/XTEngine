import { AfterViewChecked, AfterViewInit, Component, ElementRef, EventEmitter, OnDestroy, OnInit, Output, ViewChild, ChangeDetectionStrategy, ViewChildren, QueryList } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { CdkFixedSizeVirtualScroll, CdkVirtualForOf, CdkVirtualScrollViewport } from '@angular/cdk/scrolling';
import { MediaItem, MediaItemMetaData } from '../../models/interfaces';
import { MediaListItemComponent } from '../media-list-item/media-list-item.component';
import { MediaToolbarComponent } from '../media-toolbar/media-toolbar.component';
import { MediaListService } from '../../services/media-list-service';
import { VideoService } from '../../services/video.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { MediaService } from '../../services/media.service';
import { MediaListOverlayService } from '../../services/media-list-overlay.service';
import { Roles } from '../../models/enums';

@Component({
    selector: 'app-media-list',
    imports: [FormsModule, CdkVirtualScrollViewport, CdkFixedSizeVirtualScroll, CdkVirtualForOf, MediaListItemComponent, MediaToolbarComponent],
    templateUrl: './media-list.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list.component.css',
    host: {
      // Full screen: the view is re-parented into the video container by
      // MediaListOverlayService and styled as a right-hand overlay that shows
      // and hides with the player controls. Can be turned off in the local
      // settings (see overlayActive).
      '[class.media-list-overlay]': 'overlayActive',
      '[class.media-list-overlay-visible]': 'overlayActive && overlayVisible',
      // Keep-alive for the full screen overlay: it stays visible while any of
      // these is active - mouse over the list, a finger on it, or focus in
      // one of its filter inputs (typing). Touch devices don't fire
      // mouseenter/mouseleave, so the touch events cover them.
      '(mouseenter)': 'onMediaListMouseEnter()',
      '(mouseleave)': 'onMediaListMouseLeave()',
      '(touchstart)': 'onMediaListTouchStart()',
      '(touchend)': 'onMediaListTouchEnd()',
      '(touchcancel)': 'onMediaListTouchEnd()',
      '(focusin)': 'onMediaListFocusIn()',
      '(focusout)': 'onMediaListFocusOut($event)',
    }
})
export class MediaListComponent implements OnInit, AfterViewInit, AfterViewChecked, OnDestroy {
  // @ViewChildren(MediaListItemComponent) mediaNodes: QueryList<MediaListItemComponent> = new QueryList();
  @ViewChild('mediaListContainer') mediaListContainer?: CdkVirtualScrollViewport;

  mediaList: MediaItem[] = [];
  // The list chunked into rows of `columns` items - cdk-virtual-scroll is
  // single column so the rows are what gets virtualized
  rows: MediaItem[][] = [];
  columns = 1;
  // filterText = '';
  // showOptions = ['All', 'Unwatched', 'Watched'];
  // sortOptions = ['name', 'name-desc', 'date', 'date-desc'];
  // currentShow = 'All';
  // currentSort = 'name';
  
  textHeightInt = 0;
  textHeight = "0";
  textWidth = "0";
  width = "0";
  widthInt = 0;
  height = "0";
  heightInt = 0;
  fontSize = "0";
  tagsHeightInt = 0;
  tagsHeight = "0";
  tagsFontSize = "0";
  infoWidth = "0";
  infoHeight = "0";

  thumbWidth = "0";
  thumbWidthInt = 0;
  thumbHeight = "0";
  thumbHeightInt = 0;
  iconSize = "0";
  mediaLoading = true;
  mediaLoadingStatus = "Loading...";
  // Drives the full screen overlay host classes (see host bindings)
  isFullScreen = false;
  overlayVisible = false;
  // Local setting: when checked the media list never overlays the video in
  // full screen and stays in its normal position in the app body.
  private fullScreenMediaListDisabled = false;
  // Full screen overlay active: full screen AND not disabled by the setting.
  // Host bindings use this instead of isFullScreen directly.
  get overlayActive(): boolean {
    return this.isFullScreen && !this.fullScreenMediaListDisabled;
  }

  @Output() rendered = new EventEmitter<void>();
  private renderedEmitted = false;
  private pendingRender = false;

  private mediaItemHeaderMultiplier: number = 0.15;
  private mediaItemHeaderIconMultiplier: number = 0.10;
  private resizeObserver: ResizeObserver | null = null;
  private resizeDebounce: ReturnType<typeof setTimeout> | null = null;
  private windowResizeHandler: (() => void) | null = null;
  // Single document-level listener that closes the (single) open context menu
  // when the user clicks/taps outside of it. Lives here (one instance) rather
  // than in each item so it doesn't fire N times per click.
  private contextMenuClickHandler = (event: MouseEvent) => {
    // Ignore clicks inside a context menu (its own rows handle those).
    if ((event.target as HTMLElement).closest('.media-context-portal')) {
      return;
    }
    if (this.mediaListService.hasOpenContextMenu()) {
      this.mediaListService.closeContextMenu();
    }
  };

  constructor(
    private mediaService: MediaService,
    private localSettingsService: LocalSettingsService,
    private messageHandlerService: MessageHandlerService,
    private videoService: VideoService,
    private mediaListService: MediaListService,
    private mediaListOverlayService: MediaListOverlayService,
    private elementRef: ElementRef<HTMLElement>
  ) {
    this.setDimensions();
  }

  ngOnInit(): void {
    this.videoService.isFullScreen$.subscribe(v => {
      this.isFullScreen = v;
      // The list is re-parented into/out of the video container on full
      // screen changes, which changes its width. The ResizeObserver can miss
      // the move (re-parent + resize land in the same frame), so force a
      // rebuild once the new layout has settled. Deferring to the next frame
      // keeps the heavy re-chunk off the full screen transition's first tick;
      // buildRows() is a no-op when the column count is unchanged, so this
      // never double-rebuilds.
      requestAnimationFrame(() => this.buildRows());
    });
    this.videoService.mediaSheetVisible$.subscribe(v => {
      this.overlayVisible = v;
    });
    this.localSettingsService.disableFullScreenMediaList$.subscribe(v => {
      this.fullScreenMediaListDisabled = v;
    });
    this.messageHandlerService.mediaLoaded$.subscribe(() => {
      this.mediaLoadingStatus = "Downloading list...";
    });
    this.messageHandlerService.mediaLoading$.subscribe(() => {
      this.mediaLoading = true;
    });
    this.messageHandlerService.mediaLoadingStatus$.subscribe(status => {
      this.mediaLoadingStatus = status || (this.mediaLoading ? "Loading..." : "Loaded");
    });
    this.messageHandlerService.updateMediaItem$.subscribe(v => {
      if(!v?.item)
        return;
      this.updateItem(v.item);
    });
    this.mediaListService.mediaListDisplayed$.subscribe(list => {
      if(list) {
        this.mediaList = list;
        this.mediaLoadingStatus = "Rendering rows...";
        this.buildRows();
        this.mediaLoading = false;
        this.mediaLoadingStatus = "";
        this.pendingRender = true;
      }
    });
    this.mediaListService.scrollToItem$.subscribe(item => {
      this.scrollToItem(item);
    });
    this.localSettingsService.thumbSize$.subscribe(() => {
      this.setDimensions();
      this.buildRows();
    });
  }

  ngAfterViewInit() {
    // Register this element with the overlay service: while full screen it is
    // re-parented into the video container and back again when it exits.
    this.mediaListOverlayService.registerMediaListElement(this.elementRef.nativeElement);
    if (this.mediaListContainer) {
      // Debounced - resizes fire continuously (window drag, video player
      // open/close) and each rebuild re-chunks the whole list
      this.resizeObserver = new ResizeObserver(() => this.scheduleRebuild());
      this.resizeObserver.observe(this.mediaListContainer.elementRef.nativeElement);
    }
    // Fallback: the ResizeObserver on the cdk viewport can miss width-only
    // changes in some browsers; a window resize listener guarantees the grid
    // re-flows when the viewport width changes.
    this.windowResizeHandler = () => {
      this.scheduleRebuild();
    };
    window.addEventListener('resize', this.windowResizeHandler);
    // Close the context menu when clicking/tapping outside of it.
    document.addEventListener('click', this.contextMenuClickHandler);
    this.buildRows();
  }

  ngAfterViewChecked(): void {
    if (this.pendingRender && !this.renderedEmitted) {
      this.renderedEmitted = true;
      this.pendingRender = false;
      this.rendered.emit();
    }
  }

  private scheduleRebuild() {
    if (this.resizeDebounce) {
      clearTimeout(this.resizeDebounce);
    }
    this.resizeDebounce = setTimeout(() => this.buildRows(), 150);
  }

  ngOnDestroy() {
    this.resizeObserver?.disconnect();
    this.resizeObserver = null;
    if (this.windowResizeHandler) {
      window.removeEventListener('resize', this.windowResizeHandler);
      this.windowResizeHandler = null;
    }
    document.removeEventListener('click', this.contextMenuClickHandler);
    if (this.resizeDebounce) {
      clearTimeout(this.resizeDebounce);
      this.resizeDebounce = null;
    }
  }

  // .media-item has a 5px margin top and bottom
  get rowHeight(): number {
    return this.heightInt + 10;
  }

  // Track rows by their first item's id + the current column count. The
  // column count is part of the key so that when the viewport resizes and
  // buildRows() re-chunks the list (different items per row), the virtual
  // scroller sees new track ids and re-stamps the visible rows instead of
  // reusing stale row views (which left the grid failing to re-flow).
  // Must be an arrow-function property (not a method): the CDK invokes the
  // trackBy as a bare function call, so a method would lose the component's
  // `this` and `this.columns` would be undefined.
  trackRow = (index: number, row?: MediaItem[]): string | number => {
    return row && row.length ? `${row[0].id}:${this.columns}` : index;
  };

  private buildRows() {
    const containerWidth = this.mediaListContainer?.elementRef.nativeElement.clientWidth || window.innerWidth;
    const itemWidth = this.widthInt + 10; // .media-item margin 5px each side
    const columns = Math.max(1, Math.floor(containerWidth / itemWidth));
    // The rows depend only on the column count and the list, so if neither
    // changed the re-chunk is a no-op. Skipping it also avoids forcing the
    // virtual scroller to re-diff every visible row (trackRow keys include the
    // column count). This drops the redundant rebuild that fires right after
    // the full screen re-parent, which was janking the transition.
    if (columns === this.columns
        && this.rows.length === Math.ceil(this.mediaList.length / this.columns)) {
      return;
    }
    this.columns = columns;
    const rows: MediaItem[][] = [];
    for (let i = 0; i < this.mediaList.length; i += columns) {
      rows.push(this.mediaList.slice(i, i + columns));
    }
    this.rows = rows;
  }

  setDimensions() {
    const thumbSizeGlobal = this.getThumbSize();
    // The thumbnail is 16:9. Its height is a fixed fraction of the thumb size
    // and its width follows from that ratio; the item width matches the
    // thumbnail so the image fills the card edge to edge.
    this.thumbHeightInt = thumbSizeGlobal * 0.78;
    this.thumbWidthInt = this.thumbHeightInt * (16 / 9);
    this.widthInt = this.thumbWidthInt;
    this.width = this.widthInt + "px";
    this.textWidth = this.width;
    // Font sizes scale non-linearly with the thumb size (see scaleFont): they
    // match the old linear size at the desktop reference (250px) but fall off
    // more gently on small/mobile thumbs so the text stays readable.
    const nameFont = this.scaleFont(thumbSizeGlobal, 0.06, 10);
    const tagsFont = this.scaleFont(thumbSizeGlobal, 0.042, 8);
    this.fontSize = nameFont + "px";
    this.tagsFontSize = tagsFont + "px";
    // The name + tags rows grow to fit their font (the info section can expand
    // on small thumbs) while keeping the font a fixed fraction of the row.
    this.textHeightInt = nameFont / 0.4; // name row
    this.textHeight = this.textHeightInt + "px";
    this.tagsHeightInt = tagsFont / 0.42; // tags row
    this.tagsHeight = this.tagsHeightInt + "px";
    this.heightInt = this.thumbHeightInt + this.textHeightInt + this.tagsHeightInt;
    this.height = this.heightInt + "px";
    this.thumbWidth = this.thumbWidthInt + "px";
    this.thumbHeight = this.thumbHeightInt + "px";
    this.infoHeight = this.widthInt * this.mediaItemHeaderMultiplier + "px";
    this.infoWidth = this.infoHeight;
    this.iconSize = this.widthInt * this.mediaItemHeaderIconMultiplier + "px";
  }

  // Non-linear font scaling. At/above the reference thumb size (250px, the
  // desktop size that was tuned) the font is the old linear size. Below it a
  // sub-linear power curve makes the font fall off more gently, and a floor
  // keeps it readable on the smallest (mobile) thumbs.
  private scaleFont(thumbSize: number, linearFactor: number, minFont: number): number {
    const REF_THUMB = 250;
    const POWER = 0.6;
    if (thumbSize >= REF_THUMB) {
      return thumbSize * linearFactor;
    }
    const refFont = REF_THUMB * linearFactor;
    return Math.max(minFont, refFont * Math.pow(thumbSize / REF_THUMB, POWER));
  }

  noMedia(): boolean {
    return this.mediaList?.length == 0;
  }

  onItemClick(item: MediaItem) {
    this.videoService.playVideo(item);
    // Selecting a video closes the full screen media sheet so the new video is
    // the focus.
    this.videoService.closeMediaSheet();
  }

  // Keep-alive state for the full screen overlay: the controls (and with
  // them the overlay) stay visible while any of these is active.
  private listMouseOver = false;
  private listTouchActive = false;
  private listFocused = false;

  private updateListKeepAlive(): void {
    const active = this.isFullScreen &&  (this.listMouseOver || this.listTouchActive || this.listFocused);
    if (this.overlayActive) {
      this.videoService.setMediaListActive(active);
    } else if (active) {
      this.videoService.showControls();
    } else {
      this.videoService.hideControlsDebounce();
    }
  }

  onMediaListMouseEnter(): void {
    this.listMouseOver = true;
    this.updateListKeepAlive();
  }

  onMediaListMouseLeave(): void {
    this.listMouseOver = false;
    this.updateListKeepAlive();
  }

  onMediaListTouchStart(): void {
    this.listTouchActive = true;
    this.updateListKeepAlive();
  }

  onMediaListTouchEnd(): void {
    this.listTouchActive = false;
    this.updateListKeepAlive();
  }

  onMediaListFocusIn(): void {
    this.listFocused = true;
    this.updateListKeepAlive();
  }

  onMediaListFocusOut(event: FocusEvent): void {
    // Focus moving to another element inside the list (e.g. from the tag
    // filter input to the search box) keeps the overlay alive.
    const related = event.relatedTarget as Node | null;
    if (related && this.elementRef.nativeElement.contains(related)) {
      return;
    }
    this.listFocused = false;
    this.updateListKeepAlive();
  }

  private scrollToItem(item: MediaItem) {
    const index = this.mediaList.findIndex(m => m.id === item.id);
    if (index === -1) return;
    const rowIndex = Math.floor(index / this.columns);
    this.mediaListContainer?.scrollToIndex(rowIndex, "smooth" );
  }

  getThumbSize(): number {
    return this.mediaListService.thumbSize;
  }

  updateMetadata(item: MediaItem) {
    return this.mediaService.updateItemMetadata(item.metaData);
  }
  
  validateMetadata(metaData: MediaItemMetaData) {
    if(metaData?.MFSScripts.length > 100)
    {
      // showAlertWindow("System error", `There is a possible issue with this items metadata.<br>
      //   Doing a metadata process by clicking one of the buttons below or<br>
      //   from 'Settings \> System \> Process metadata' should fix this issue<br>
      //   You can also click 'Update metadata' from this media items context menu.<br>
      //   Note: you may need to resave after the item has been updated.<br><br>
      //   <button style='margin-top:8px' onclick='closeAlertWindow();sendUpdateMetadata(\"${metaData.key}\");'>Process ONLY THIS items metadata</button>
      //   <button onclick='closeAlertWindow();startMetadataProcess();'>Start processing ALL metadata now</button>`
      // );
    }
  }

  private updateItem(item: MediaItem)
  {
    var index = this.mediaList.findIndex(x => x.id == item.id);
    if(index > -1) {
      this.mediaListService.updateMediaItem(item);
      if(this.videoService.isItemPlaying(item)) {
        this.videoService.playingMediaItem = item;
      }
      // Swap the item in place in its row. Rebuilding all rows here would
      // re-chunk the entire list and force the virtual scroller to re-diff
      // every row on each update message - the inner @for (track item.id)
      // picks up the in-place change on the next change detection pass.
      var row = this.rows[Math.floor(index / this.columns)];
      if (row) {
        row[index % this.columns] = item;
      }
      // if(selectedMediaItemForMetaData && selectedMediaItemForMetaData.id == libraryItem.id) {
      //   selectedMediaItemForMetaData = this.mediaList[index];
      // }
    }
  }
  // onFilterChange(value: string): void {
  //   this.mediaListService.setFilterCriteria(value);
  // }

  // onShowChange(value: string): void {
  //   this.mediaListService.setShow(value);
  // }

  // onSortChange(value: string): void {
  //   this.mediaListService.setSortBy(value);
  // }
}
