import { Component, OnInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { ShowMode, SortMode } from '../../models/enums';
import { debounceTime, distinctUntilChanged, Subject } from 'rxjs';

@Component({
  selector: 'app-media-toolbar',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './media-toolbar.component.html',
  styleUrl: './media-toolbar.component.css'
})
export class MediaToolbarComponent implements OnInit {
  filterText: string = this.localSettingsService.filterCriteriaDefault;
  filtertextChanged: Subject < string > = new Subject < string > (); 
  currentShow: ShowMode = this.localSettingsService.showDefault;
  currentSort: SortMode = this.localSettingsService.sortDefault;
  randomMode = false;

  private nonRandomSort: SortMode = this.localSettingsService.sortDefault;

  constructor(
    private localSettingsService: LocalSettingsService
  ) {
    this.filtertextChanged.pipe(
      debounceTime(400),
      distinctUntilChanged())
      .subscribe(value => {
        this.localSettingsService.setFilter(value);
      });
  }

  filterInputHidden: boolean = true;
  tagInputHidden: boolean = true;
  tcodeActionsInputHidden: boolean = true;
  scriptInputHidden: boolean = true;

  ngOnInit(): void {
    this.localSettingsService.filterCriteria$.subscribe(f => this.filterText = f);
    this.localSettingsService.show$.subscribe(s => {
      this.currentShow = s
    });
    this.localSettingsService.sort$.subscribe(s => {
      this.currentSort = s;
    });
  }

  onFilterChange(value: string): void {
    // this.localSettingsService.setFilter(value);
    this.filtertextChanged.next(value);
  }

  onShowChange(value: ShowMode): void {
    this.localSettingsService.setShow(value);
  }

  onSortChange(value: SortMode): void {
    this.localSettingsService.setSort(value);
  }

  randomize() {
    this.localSettingsService.setSort(SortMode.Random);
  }

  toggleShufflePlay() {
    this.randomMode = !this.randomMode;
    this.setRandom();
  }

  clearFilterText() {
    this.localSettingsService.setFilter("");
  }

  private setRandom() {
    if(this.randomMode) {
      this.nonRandomSort = this.currentSort;
      this.localSettingsService.setSort(SortMode.Random)
    } else
      this.localSettingsService.setSort(this.nonRandomSort)
  }
}