import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaService } from '../../services/media.service';
import { AppStateService } from '../../services/app-state.service';
import { MediaItem } from '../../models/interfaces';

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
    private mediaService: MediaService,
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

  onFilterChange(value: string): void {
    this.appState.setFilterCriteria(value);
  }

  onShowChange(value: string): void {
    this.appState.setShow(value);
  }

  onSortChange(value: string): void {
    this.appState.setSortBy(value);
  }

  onPlay(item: MediaItem): void {
    this.mediaService.playVideo(item);
  }

  onPlayNext(): void {
    this.mediaService.playNextVideo();
  }

  onPlayPrevious(): void {
    this.mediaService.playPreviousVideo();
  }

  onStop(): void {
    this.mediaService.stopVideo();
  }

  onSkipToMoneyShot(): void {
    this.mediaService.skipToMoneyShot();
  }

  onSkipToNextAction(): void {
    this.mediaService.skipToNextAction();
  }

  isPlaying(item: MediaItem): boolean {
    return this.appState.playingMediaItem?.id === item.id;
  }

  getThumbSize(): number {
    return this.appState.thumbSize;
  }
}