import { Component, OnInit } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaService } from '../../services/media.service';
import { AppStateService } from '../../services/app-state.service';
import { MediaItem } from '../../models/interfaces';
import { ThumbState } from '../../models/enums';

@Component({
  selector: 'app-media-list',
  standalone: true,
  imports: [CommonModule, FormsModule, NgOptimizedImage],
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
    private appState: AppStateService
  ) {
    this.setDimensions();
  }

  ngOnInit(): void {
    this.appState.mediaListDisplayed$.subscribe(list => {
      this.mediaList = list;
    });
    this.appState.filterCriteria$.subscribe(f => this.filterText = f);
    this.appState.show$.subscribe(s => this.currentShow = s);
    this.appState.sortBy$.subscribe(s => this.currentSort = s);
    this.appState.thumbSize$.subscribe(() => {this.setDimensions();});
    this.setupLazyLoad();
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

  setupLazyLoad() {
    var lazyImages = [].slice.call(document.querySelectorAll("img.lazy"));;
    if ("IntersectionObserver" in window && "IntersectionObserverEntry" in window && "intersectionRatio" in window.IntersectionObserverEntry.prototype) {
      let lazyImageObserver = new IntersectionObserver(function(entries, observer) {
      entries.forEach(function(entry) {
        if (entry.isIntersecting) {
        let lazyImage = entry.target as HTMLImageElement;
        lazyImage.src = lazyImage.dataset['src'] ?? "";
        //lazyImage.srcset = lazyImage.dataset.srcset;
        lazyImage.classList.remove("lazy");
        lazyImageObserver.unobserve(lazyImage);
        }
      });
      });
    
      lazyImages.forEach(function(lazyImage) {
        lazyImageObserver.observe(lazyImage);
      });
    }
  }

  onFilterChange(value: string): void {
    this.appState.setFilterCriteria(value);
  }

  onShowChange(value: string): void {
    this.appState.setShow(value);
  }

  onSortChange(value: string): void {
    this.appState.setSortBy(value);
  }

  onItemClick(item: MediaItem): void {
    this.mediaService.playVideo(item);
  }

  isPlaying(item: MediaItem): boolean {
    return this.appState.playingMediaItem?.id === item.id;
  }

  getThumbSize(): number {
    return this.appState.thumbSize;
  }

  getLazyLoad(): boolean {
    return this.appState.disableBrowserLazyLoad;
  }

  getThumbPath(item: MediaItem): string {
		if(!this.appState.disableBrowserLazyLoad) {
			if (item.thumbFileExists)
				return "/thumb/" + item.relativeThumb;
			else
				return "/thumb/" + (item.thumbState === ThumbState.Unknown ? item.thumbFileUnknown : item.thumbState === ThumbState.Error ? item.thumbFileError : item.thumbFileLoading);
		} else {
			if (item.thumbFileExists)
				return "/thumb/" + item.relativeThumb;
			else 
				return "/thumb/" + (item.thumbState === ThumbState.Unknown ? item.thumbFileUnknown : item.thumbState === ThumbState.Error ? item.thumbFileError : item.thumbFileLoading);
		}
  }
}