import { AfterViewInit, Component, OnInit, QueryList, ViewChildren } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaItem } from '../../models/interfaces';
import { MediaListItemComponent } from '../media-list-item/media-list-item.component';
import { MediaListService } from '../../services/media-list-service';
import { VideoService } from '../../services/video.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';

@Component({
  selector: 'app-media-list',
  standalone: true,
  imports: [CommonModule, FormsModule, NgOptimizedImage, MediaListItemComponent],
  templateUrl: './media-list.component.html',
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
  fontSize = "0";
  infoWidth = "0";
  infoHeight = "0";

  thumbWidth = "0";
  thumbHeight = "0";
  iconSize = "0";
  mediaLoading = true;
  mediaLoadingStatus = "Loading...";

  private mediaItemHeaderMultiplier: number = 0.15;
  private mediaItemHeaderIconMultiplier: number = 0.10;

  constructor(
    private localSettingsService: LocalSettingsService,
    private messageHandlerService: MessageHandlerService,
    private videoService: VideoService,
    private mediaListService: MediaListService
  ) {
    this.setDimensions();
  }

  ngOnInit(): void {
    this.messageHandlerService.mediaLoaded$.subscribe(() => {
      // this.mediaLoading = false;
    });
    this.messageHandlerService.mediaLoading$.subscribe(() => {
      this.mediaLoading = true;
    });
    this.messageHandlerService.mediaLoadingStatus$.subscribe(status => {
      this.mediaLoadingStatus = status || (this.mediaLoading ? "Loading..." : "Loaded");
    });
    this.mediaListService.mediaListDisplayed$.subscribe(list => {
      this.mediaList = list;
    });
    this.localSettingsService.thumbSize$.subscribe(() => {this.setDimensions();});
  }

  ngAfterViewInit() {
    this.mediaNodes.changes.subscribe(t => {
      // if(this.mediaList.length == t.length) {
        this.mediaLoading = false;
        this.mediaLoadingStatus = "";
      // }
    });
  }

  setDimensions() {
    const thumbSizeGlobal = this.getThumbSize();
    this.textHeightInt = (thumbSizeGlobal * 0.25);
    this.textHeight = this.textHeightInt + "px";
    this.widthInt = thumbSizeGlobal + (thumbSizeGlobal * 0.15);
    this.width = this.widthInt+ "px";
    this.textWidth = this.width;
    this.height = thumbSizeGlobal + this.textHeightInt + "px";
    this.fontSize = (this.textHeightInt * 0.35) + "px";
    this.thumbWidth = thumbSizeGlobal + "px";
    this.thumbHeight = thumbSizeGlobal - this.textHeightInt + "px";
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