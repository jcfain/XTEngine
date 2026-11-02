import { AfterViewInit, Component, OnDestroy, OnInit, ViewChild, ChangeDetectionStrategy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { CdkFixedSizeVirtualScroll, CdkVirtualForOf, CdkVirtualScrollViewport } from '@angular/cdk/scrolling';
import { MediaItem, MediaItemMetaData } from '../../models/interfaces';
import { MediaListItemComponent } from '../media-list-item/media-list-item.component';
import { MediaListService } from '../../services/media-list-service';
import { VideoService } from '../../services/video.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { MediaService } from '../../services/media.service';
import { Roles } from '../../models/enums';

@Component({
    selector: 'app-media-list',
    imports: [FormsModule, CdkVirtualScrollViewport, CdkFixedSizeVirtualScroll, CdkVirtualForOf, MediaListItemComponent],
    templateUrl: './media-list.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list.component.css'
})
export class MediaListComponent implements OnInit, AfterViewInit, OnDestroy {

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
  infoWidth = "0";
  infoHeight = "0";

  thumbWidth = "0";
  thumbHeight = "0";
  thumbHeightInt = 0;
  iconSize = "0";
  mediaLoading = true;
  mediaLoadingStatus = "Loading...";

  private mediaItemHeaderMultiplier: number = 0.15;
  private mediaItemHeaderIconMultiplier: number = 0.10;
  private resizeObserver: ResizeObserver | null = null;
  private resizeDebounce: ReturnType<typeof setTimeout> | null = null;
  private windowResizeHandler: (() => void) | null = null;

  constructor(
    private mediaService: MediaService,
    private localSettingsService: LocalSettingsService,
    private messageHandlerService: MessageHandlerService,
    private videoService: VideoService,
    private mediaListService: MediaListService
  ) {
    this.setDimensions();
  }

  ngOnInit(): void {
    this.messageHandlerService.mediaLoaded$.subscribe(() => {
      this.mediaLoading = false;
      this.mediaLoadingStatus = "";
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
      this.mediaList = list;
      this.buildRows();
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
    if (this.mediaListContainer) {
      // Debounced - resizes fire continuously (window drag, video player
      // open/close) and each rebuild re-chunks the whole list
      this.resizeObserver = new ResizeObserver(() => this.scheduleRebuild());
      this.resizeObserver.observe(this.mediaListContainer.elementRef.nativeElement);
    }
    // Fallback: the ResizeObserver on the cdk viewport can miss width-only
    // changes in some browsers; a window resize listener guarantees the grid
    // re-flows when the viewport width changes.
    this.windowResizeHandler = () => this.scheduleRebuild();
    window.addEventListener('resize', this.windowResizeHandler);
    this.buildRows();
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
    this.columns = Math.max(1, Math.floor(containerWidth / itemWidth));
    const rows: MediaItem[][] = [];
    for (let i = 0; i < this.mediaList.length; i += this.columns) {
      rows.push(this.mediaList.slice(i, i + this.columns));
    }
    this.rows = rows;
  }

  setDimensions() {
    const thumbSizeGlobal = this.getThumbSize();
    this.textHeightInt = (thumbSizeGlobal * 0.25);
    this.textHeight = this.textHeightInt + "px";
    this.widthInt = thumbSizeGlobal + (thumbSizeGlobal * 0.15);
    this.width = this.widthInt+ "px";
    this.textWidth = this.width;
    this.heightInt = thumbSizeGlobal + this.textHeightInt;
    this.height = this.heightInt + "px";
    this.fontSize = (this.textHeightInt * 0.35) + "px";
    this.thumbWidth = thumbSizeGlobal + "px";
    this.thumbHeightInt = thumbSizeGlobal - this.textHeightInt;
    this.thumbHeight = this.thumbHeightInt + "px";
    this.infoHeight = this.widthInt * this.mediaItemHeaderMultiplier + "px";
    this.infoWidth = this.infoHeight;
    this.iconSize = this.widthInt * this.mediaItemHeaderIconMultiplier + "px";
  }

  noMedia(): boolean {
    return !this.mediaLoading && this.mediaList?.length == 0;
  }

  onItemClick(item: MediaItem) {
    this.videoService.playVideo(item);
  }

  private scrollToItem(item: MediaItem) {
    const index = this.mediaList.findIndex(m => m.id === item.id);
    if (index === -1) return;
    const rowIndex = Math.floor(index / this.columns);
    this.mediaListContainer?.scrollToIndex(rowIndex);
  }

  getThumbSize(): number {
    return this.mediaListService.thumbSize;
  }

  updateMetadata(item: MediaItem) {
    return this.mediaService.updateItemMetadata(item.metaData);
  }
  
  validateMetadata(metaData: MediaItemMetaData) {
    if(metaData?.mfsScripts.length > 100)
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
      var updated = JSON.parse(JSON.stringify(item));
      this.mediaList[index] = updated;
      if(this.videoService.isItemPlaying(item)) {
        this.videoService.playingMediaItem = updated;
      }
      // Swap the item in place in its row. Rebuilding all rows here would
      // re-chunk the entire list and force the virtual scroller to re-diff
      // every row on each update message - the inner @for (track item.id)
      // picks up the in-place change on the next change detection pass.
      var row = this.rows[Math.floor(index / this.columns)];
      if (row) {
        row[index % this.columns] = updated;
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
