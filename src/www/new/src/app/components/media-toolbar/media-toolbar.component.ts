import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';

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
    private appState: AppStateService
  ) {}

  filterInputHidden: boolean = true;
  tagInputHidden: boolean = true;
  tcodeActionsInputHidden: boolean = true;

  ngOnInit(): void {
    this.appState.filterCriteria$.subscribe(f => this.filterText = f);
    this.appState.show$.subscribe(s => this.currentShow = s);
    this.appState.sortBy$.subscribe(s => this.currentSort = s);
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

  toggleShufflePlay() {}

  sortChange(value: string) {}
}