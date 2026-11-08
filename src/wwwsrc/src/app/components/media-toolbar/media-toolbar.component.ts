import { Component, OnInit, OnDestroy, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { ShowMode, SortMode } from '../../models/enums';
import { debounceTime, distinctUntilChanged, Subject, Subscription } from 'rxjs';
import { MessageHandlerService } from '../../services/message-handler.service';
import { ServerService } from '../../services/server.service';
import { TCodeCommandsComponent } from '../tcode-commands/tcode-commands.component';
import { MediaListService } from '../../services/media-list-service';
import { AppStateService } from '../../services/app-state.service';


@Component({
    selector: 'app-media-toolbar',
    imports: [FormsModule, TCodeCommandsComponent],
    templateUrl: './media-toolbar.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-toolbar.component.css'
})
export class MediaToolbarComponent implements OnInit, OnDestroy {
  private subscriptions = new Subscription();
  filterText: string = this.localSettingsService.filterCriteriaDefault;
  filtertextChanged: Subject < string > = new Subject < string > (); 
  currentShow: ShowMode = this.localSettingsService.showDefault;
  currentSort: SortMode = this.localSettingsService.sortDefault;
  randomMode = false;

  // private nonRandomSort: SortMode = this.localSettingsService.sortDefault;

  constructor(
    private localSettingsService: LocalSettingsService,
    private serverService: ServerService,
    private messageHandlerService: MessageHandlerService,
    private appState: AppStateService,
    private mediaListService: MediaListService
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
  totalMediaItems: number = 0;

  getTagCount(tag: string): number {
    if(!this.tagCounts[tag])
      return 0;
    const count = this.tagCounts[tag].count;
    return !count ? 0 : count;
  }
  getTagTotal(tag: string): number {
    if(!this.tagCounts[tag])
      return 0;
    const count = this.tagCounts[tag].total;
    return !count ? 0 : count;
  }

  private tagCounts: Record<string, {count: number, total: number}> = {};

  ngOnInit(): void {
    this.subscriptions.add(this.localSettingsService.filterCriteria$.subscribe(f => this.filterText = f));
    this.subscriptions.add(this.localSettingsService.show$.subscribe(s => {
      this.currentShow = s
    }));
    this.subscriptions.add(this.localSettingsService.sort$.subscribe(s => {
      this.currentSort = s;
    }));
    this.subscriptions.add(this.localSettingsService.selectedTags$.subscribe(s => {
      this.selectedTags = s;
    }));
    this.subscriptions.add(this.mediaListService.mediaList$.subscribe(v => {
      this.updateTagCountTotals(this.allTags);
    }));
    this.subscriptions.add(this.mediaListService.mediaListDisplayed$.subscribe(v => {
      this.updateTagCountsDisplayed(this.allTags);
    }));
    this.subscriptions.add(this.messageHandlerService.tagsUpdateSubject$.subscribe(v => {
      this.allTags = v?.allTags || [];
      this.updateTagCounts(this.allTags);
    }));
    this.subscriptions.add(this.serverService.remoteUserSettings$.subscribe(v => {
      this.allTags = v?.allTags || [];
      this.updateTagCounts(this.allTags);
    }))
  }

  ngOnDestroy(): void {
    this.subscriptions.unsubscribe();
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
    // this.localSettingsService.setSort(SortMode.Random);
    this.mediaListService.playNextVideo();
  }

  toggleShufflePlay() {
    this.randomMode = !this.randomMode;
    this.appState.setShufflePlayMode(this.randomMode);
    // this.setRandom();
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

  private updateTagCounts(allTags: string[]): void {
    this.tagCounts = {};
    this.updateTagCountTotals(allTags);
    this.updateTagCountsDisplayed(allTags);
  }

  private updateTagCountTotals(allTags: string[]): void {
    this.tagCounts = {};
    allTags.forEach(tag => {
      this.tagCounts[tag] = {count: 0, total: 0};
    });
    // O(items * tags_per_item) instead of O(tags * items * tags_per_item)
    this.mediaListService.mediaList?.forEach(item => {
      item.metaData.tags.forEach(tag => {
        if(this.tagCounts[tag]) {
          this.tagCounts[tag].total++;
        }
      });
    });
  }

  private updateTagCountsDisplayed(allTags: string[]): void {
    allTags.forEach(tag => {
      if(!this.tagCounts[tag]) {
        this.tagCounts[tag] = {count: 0, total: 0};
      } else {
        this.tagCounts[tag].count = 0;
      }
    });
    // O(items * tags_per_item) instead of O(tags * items * tags_per_item)
    this.mediaListService.mediaListDisplayed?.forEach(item => {
      item.metaData.tags.forEach(tag => {
        if(this.tagCounts[tag]) {
          this.tagCounts[tag].count++;
        }
      });
    });
  }

  // private setRandom() {
  //   if(this.randomMode) {
  //     this.nonRandomSort = this.currentSort;
  //     this.localSettingsService.setSort(SortMode.Random)
  //   } else
  //     this.localSettingsService.setSort(this.nonRandomSort)
  // }
}