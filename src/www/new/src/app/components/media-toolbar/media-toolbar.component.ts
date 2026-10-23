import { Component, OnInit, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { ShowMode, SortMode } from '../../models/enums';
import { debounceTime, distinctUntilChanged, Subject } from 'rxjs';
import { MessageHandlerService } from '../../services/message-handler.service';
import { AppStateService } from '../../services/app-state.service';

@Component({
    selector: 'app-media-toolbar',
    imports: [FormsModule],
    templateUrl: './media-toolbar.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
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
    private localSettingsService: LocalSettingsService,
    private appStateService: AppStateService,
    private messageHandlerService: MessageHandlerService
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
  allTags: string[] = [];
  selectedTags: string[] = [];

  ngOnInit(): void {
    this.localSettingsService.filterCriteria$.subscribe(f => this.filterText = f);
    this.localSettingsService.show$.subscribe(s => {
      this.currentShow = s
    });
    this.localSettingsService.sort$.subscribe(s => {
      this.currentSort = s;
    });
    this.localSettingsService.selectedTags$.subscribe(s => {
      this.selectedTags = s;
    });
    this.messageHandlerService.tagsUpdateSubject$.subscribe(v => {
      this.allTags = v?.allTags || [];
    });
    this.appStateService.remoteUserSettings$.subscribe(v => {
      this.allTags = v?.allTags || [];
    })
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

  isTagSelected(tag: string): boolean {
    return this.selectedTags.findIndex(x => x === tag) > -1;
  }

  onTagClick($event: Event) {
    const node = ($event.target as HTMLInputElement);
    if(node.checked)
      this.localSettingsService.addToSelectedTags(node.value);
    else
      this.localSettingsService.removeFromSelectedTags(node.value);
  }

  private setRandom() {
    if(this.randomMode) {
      this.nonRandomSort = this.currentSort;
      this.localSettingsService.setSort(SortMode.Random)
    } else
      this.localSettingsService.setSort(this.nonRandomSort)
  }
}