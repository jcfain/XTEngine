import { Component, OnInit } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaService } from '../../services/media.service';
import { AppStateService } from '../../services/app-state.service';
import { MediaItem } from '../../models/interfaces';
import { MediaListItemComponent } from '../media-list-item/media-list-item.component';
import { VideoPlayerComponent } from '../video-player/video-player.component';
import { MediaListService } from '../../services/media-list-service';
import { VideoService } from '../../services/video.service';

@Component({
  selector: 'app-media-list',
  standalone: true,
  imports: [CommonModule, FormsModule, NgOptimizedImage, MediaListItemComponent],
  templateUrl: './media-list.component.html',
  styleUrl: './media-list.component.css'
})
export class MediaListComponent implements OnInit {
  mediaList: MediaItem[] = [];
  filterText = '';
  showOptions = ['All', 'Unwatched', 'Watched'];
  sortOptions = ['name', 'name-desc', 'date', 'date-desc'];
  currentShow = 'All';
  currentSort = 'name';
  
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

  private mediaItemHeaderMultiplier: number = 0.15;
  private mediaItemHeaderIconMultiplier: number = 0.10;

  constructor(
    private mediaService: MediaService,
    private videoService: VideoService,
    private appState: AppStateService,
    private mediaListService: MediaListService
  ) {
    this.setDimensions();
  }

  ngOnInit(): void {
    this.mediaListService.mediaListDisplayed$.subscribe(list => {
      this.mediaList = list;
    });
    this.mediaListService.filterCriteria$.subscribe(f => this.filterText = f);
    this.mediaListService.show$.subscribe(s => this.currentShow = s);
    this.mediaListService.sortBy$.subscribe(s => this.currentSort = s);
    this.mediaListService.thumbSize$.subscribe(() => {this.setDimensions();});
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

  onItemClick(item: MediaItem) {
    this.videoService.playVideo(item);
  }

  onFilterChange(value: string): void {
    this.mediaListService.setFilterCriteria(value);
  }

  onShowChange(value: string): void {
    this.mediaListService.setShow(value);
  }

  onSortChange(value: string): void {
    this.mediaListService.setSortBy(value);
  }

  getThumbSize(): number {
    return this.mediaListService.thumbSize;
  }
}