import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaListService } from '../../services/media-list-service';

@Component({
  selector: 'app-media-toolbar',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './media-toolbar.component.html',
  styleUrl: './media-toolbar.component.css'
})
export class MediaToolbarComponent implements OnInit {
  filterText = '';
  showOptions = ['All', 'Unwatched', 'Watched'];
  sortOptions = ['name', 'name-desc', 'date', 'date-desc'];
  currentShow = 'All';
  currentSort = 'name';

  constructor(
    private mediaListService: MediaListService
  ) {}

  filterInputHidden: boolean = true;
  tagInputHidden: boolean = true;
  tcodeActionsInputHidden: boolean = true;

  ngOnInit(): void {
    this.mediaListService.filterCriteria$.subscribe(f => this.filterText = f);
    this.mediaListService.show$.subscribe(s => this.currentShow = s);
    this.mediaListService.sortBy$.subscribe(s => this.currentSort = s);
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

  toggleShufflePlay() {}

  sortChange(value: string) {}
}