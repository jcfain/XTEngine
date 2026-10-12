import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaService } from '../../services/media.service';
import { AppStateService } from '../../services/app-state.service';
import { MediaItem } from '../../models/interfaces';
import { ThumbState } from '../../models/enums';

@Component({
  selector: 'app-media-list',
  standalone: true,
  imports: [CommonModule, FormsModule],
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

  constructor(
    private appState: AppStateService
  ) {}

  ngOnInit(): void {
    this.appState.mediaListDisplayed$.subscribe(list => {
      this.mediaList = list;
    });
    this.appState.filterCriteria$.subscribe(f => this.filterText = f);
    this.appState.show$.subscribe(s => this.currentShow = s);
    this.appState.sortBy$.subscribe(s => this.currentSort = s);
    this.appState.thumbSize$.subscribe(() => {});
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