import { AfterViewInit, Component, OnInit, QueryList, ViewChildren, ChangeDetectionStrategy } from '@angular/core';
import { FormsModule } from '@angular/forms';
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
    imports: [FormsModule, MediaListItemComponent],
    templateUrl: './media-list.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list.component.css'
})
export class MediaListComponent implements OnInit, AfterViewInit {

  @ViewChildren('mediaNodes') mediaNodes: QueryList<any> = new QueryList();

  mediaList: MediaItem[] = [];
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
    this.messageHandlerService.updateMediaItem$.subscribe(item => {
      if(!item)
        return;
      this.updateItem(item.item);
    });
    this.mediaListService.mediaListDisplayed$.subscribe(list => {
      this.mediaList = list;
    });
    this.localSettingsService.thumbSize$.subscribe(() => {this.setDimensions();});
  }

  ngAfterViewInit() {
    // this.mediaNodes.changes.subscribe(t => {
    //   // if(this.mediaList.length == t.length) {
    //     this.mediaLoading = false;
    //     this.mediaLoadingStatus = "";
    //   // }
    // });
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
    return !this.mediaLoading && this.mediaList.length == 0;
  }

  onItemClick(item: MediaItem) {
    this.videoService.playVideo(item);
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
      this.mediaList[index] = JSON.parse(JSON.stringify(item));
      if(this.videoService.isItemPlaying(item)) {
        this.videoService.playingMediaItem = this.mediaList[index];
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
