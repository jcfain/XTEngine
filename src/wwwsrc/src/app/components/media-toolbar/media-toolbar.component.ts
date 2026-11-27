import { Component, OnInit, OnDestroy, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { ShowMode, SortMode } from '../../models/enums';
import { debounceTime, distinctUntilChanged, Subject, Subscription } from 'rxjs';
import { MessageHandlerService } from '../../services/message-handler.service';
import { ServerService } from '../../services/server.service';
import { TCodeCommandsComponent } from '../tcode-commands/tcode-commands.component';
import { AppMenuComponent } from '../app-menu/app-menu.component';
import { MediaListService } from '../../services/media-list-service';
import { AppStateService } from '../../services/app-state.service';
import { VideoService } from '../../services/video.service';
import { MediaToolbarStateService, MediaToolbarState } from '../../services/media-toolbar-state.service';


@Component({
    selector: 'app-media-toolbar',
    imports: [FormsModule, TCodeCommandsComponent, AppMenuComponent],
    templateUrl: './media-toolbar.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-toolbar.component.css'
})
export class MediaToolbarComponent implements OnInit, OnDestroy {
  private subscriptions = new Subscription();
  private mediaLoading = false;
  filterText: string = this.localSettingsService.filterCriteriaDefault;
  filtertextChanged: Subject < string > = new Subject < string > (); 
  currentShow: ShowMode = this.localSettingsService.showDefault;
  currentSort: SortMode = this.localSettingsService.sortDefault;
  // Pill drawer options - same values/labels as the old settings tab selects.
  // Single selection: clicking a pill sets the mode, the active one is highlighted.
  sortOptions = [
    { value: SortMode.Name, label: 'Name (asc)' },
    { value: SortMode.NameDesc, label: 'Name (desc)' },
    { value: SortMode.Date, label: 'Date (asc)' },
    { value: SortMode.DateDesc, label: 'Date (desc)' },
    { value: SortMode.DateAdded, label: 'Added (asc)' },
    { value: SortMode.DateAddedDesc, label: 'Added (desc)' },
    { value: SortMode.Random, label: 'Random' }
  ];
  showOptions = [
    { value: ShowMode.All, label: 'All' },
    { value: ShowMode.ThreeD, label: '3D only' },
    { value: ShowMode.TwoD, label: '2D and audio only' }
  ];
  currentThumbSize: number = this.localSettingsService.thumbSizeDefault;
  // 50-500px in 25px steps, same range as the old settings tab select.
  thumbSizeOptions: number[] = Array.from({ length: 20 }, (_, i) => 50 + i * 25);
  randomMode = false;

  // private nonRandomSort: SortMode = this.localSettingsService.sortDefault;

  constructor(
    private localSettingsService: LocalSettingsService,
    private serverService: ServerService,
    private messageHandlerService: MessageHandlerService,
    private appState: AppStateService,
    private mediaListService: MediaListService,
    private videoService: VideoService,
    private toolbarStateService: MediaToolbarStateService
  ) {
    this.filtertextChanged.pipe(
      debounceTime(400),
      distinctUntilChanged())
      .subscribe(value => {
        this.localSettingsService.setFilter(value);
      });
  }

  // Full screen: the toolbar doubles as the media sheet's header. The close
  // button is only shown then, and dragging the header down closes the sheet
  // (pull-to-close).
  isFullScreen = false;
  private sheetDragStartY = 0;
  private sheetDragging = false;

  closeSheet(): void {
    this.videoService.closeMediaSheet();
  }

  // Y coordinate of a touch or mouse event.
  private eventY(event: TouchEvent | MouseEvent): number {
    if (event instanceof TouchEvent) {
      return event.touches[0].clientY;
    }
    return event.clientY;
  }

  onSheetDragStart(event: TouchEvent | MouseEvent): void {
    if (!this.isFullScreen) {
      return;
    }
    // Only start a drag from the header chrome, not from an interactive
    // control (button / input) so those keep working normally.
    if (event.target instanceof Element && event.target.closest('button, input')) {
      return;
    }
    this.sheetDragStartY = this.eventY(event);
    this.sheetDragging = true;
  }

  onSheetDragMove(event: TouchEvent | MouseEvent): void {
    if (!this.isFullScreen || !this.sheetDragging) {
      return;
    }
    const dy = this.eventY(event) - this.sheetDragStartY;
    // Dragging down past the threshold closes the sheet.
    if (dy > 40) {
      this.sheetDragging = false;
      this.videoService.closeMediaSheet();
    }
  }

  onSheetDragEnd(): void {
    this.sheetDragging = false;
  }

  // The toolbar is rendered twice (top of the page + full screen sheet
  // header), so its UI-only state lives in MediaToolbarStateService and both
  // instances mirror it (see the state$ subscription in ngOnInit).
  private updateState(partial: Partial<MediaToolbarState>): void {
    this.toolbarStateService.update(partial);
  }

  toggleTagDrawer(): void {
    this.updateState({ tagInputHidden: !this.tagInputHidden });
  }

  toggleSortDrawer(): void {
    this.updateState({ sortInputHidden: !this.sortInputHidden });
  }

  toggleShowDrawer(): void {
    this.updateState({ showInputHidden: !this.showInputHidden });
  }

  toggleThumbSizeDrawer(): void {
    this.updateState({ thumbSizeInputHidden: !this.thumbSizeInputHidden });
  }

  toggleTcodeDrawer(): void {
    this.updateState({ tcodeActionsInputHidden: !this.tcodeActionsInputHidden });
  }

  onTagFilterTextChange(value: string): void {
    this.updateState({ tagFilterText: value });
  }

  setTagSelectMode(mode: 'include' | 'exclude'): void {
    this.updateState({ tagSelectMode: mode });
  }

  tagInputHidden: boolean = true;
  sortInputHidden: boolean = true;
  showInputHidden: boolean = true;
  thumbSizeInputHidden: boolean = true;
  tcodeActionsInputHidden: boolean = true;
  scriptInputHidden: boolean = true;
  tagFilterText: string = '';
  // Shared with the other toolbar instance via the state service - which set
  // a clicked tag is added to. Not persisted - the color coding already shows
  // each tag's state.
  tagSelectMode: 'include' | 'exclude' = 'include';
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
  private tagCountGeneration = 0;

  ngOnInit(): void {
    this.subscriptions.add(this.videoService.isFullScreen$.subscribe(v => this.isFullScreen = v));
    // Mirror the shared toolbar UI state (open drawers, tag filter text, tag
    // select mode) so both toolbar instances stay in sync.
    this.subscriptions.add(this.toolbarStateService.state$.subscribe(s => {
      this.tagInputHidden = s.tagInputHidden;
      this.sortInputHidden = s.sortInputHidden;
      this.showInputHidden = s.showInputHidden;
      this.thumbSizeInputHidden = s.thumbSizeInputHidden;
      this.tcodeActionsInputHidden = s.tcodeActionsInputHidden;
      this.tagFilterText = s.tagFilterText;
      this.tagSelectMode = s.tagSelectMode;
    }));
    // Shuffle state is shared app state (the media list service and the XR
    // player read it too) - mirror it so both toolbar instances' buttons
    // agree.
    this.subscriptions.add(this.appState.shufflePlayMode$.subscribe(v => {
      this.randomMode = v;
    }));
    this.subscriptions.add(this.localSettingsService.filterCriteria$.subscribe(f => this.filterText = f));
    this.subscriptions.add(this.localSettingsService.show$.subscribe(s => {
      this.currentShow = s
    }));
    this.subscriptions.add(this.localSettingsService.sort$.subscribe(s => {
      this.currentSort = s;
    }));
    this.subscriptions.add(this.localSettingsService.thumbSize$.subscribe(s => {
      this.currentThumbSize = s;
    }));
    this.subscriptions.add(this.localSettingsService.selectedTags$.subscribe(s => {
      this.selectedTags = s;
    }));
    this.subscriptions.add(this.mediaListService.mediaListDisplayed$.subscribe(v => {
      this.updateTagCounts(this.allTags);
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

  onThumbSizeChange(value: number): void {
    this.localSettingsService.setThumbSize(value);
  }

  randomize() {
    // this.localSettingsService.setSort(SortMode.Random);
    this.mediaListService.playNextVideo();
  }

  toggleShufflePlay() {
    // Shared app state - the shufflePlayMode$ subscription mirrors the result
    // back into randomMode on both toolbar instances.
    this.appState.setShufflePlayMode(!this.randomMode);
    // this.setRandom();
  }

  clearFilterText() {
    this.localSettingsService.setFilter("");
  }

  get filteredTags(): string[] {
    const filter = this.tagFilterText.trim().toLowerCase();
    if (!filter)
      return this.allTags;
    return this.allTags.filter(tag => tag.toLowerCase().includes(filter));
  }

  get activeFilterCount(): number {
    let count = this.activeFilterTagsCount;
    if (this.filterText.trim())
      count++;
    return count;
  }

  get activeFilterTagsCount(): number {
    // Do not include the OR tag item in the list
    return this.selectedTags.filter(tag => tag !== 'OR').length;
  }

  clearAllFilters() {
    this.localSettingsService.setFilter("");
    this.localSettingsService.setSelectedTags([]);
  }

  onTagToggle(tag: string) {
    const includeKey = tag;
    const excludeKey = '!' + tag;
    const inInclude = this.isTagSelected(includeKey);
    const inExclude = this.isTagSelected(excludeKey);
    const activeKey = this.tagSelectMode === 'exclude' ? excludeKey : includeKey;

    if (this.isTagSelected(activeKey)) {
      // Clicking a tag already in the active set removes it
      this.localSettingsService.removeFromSelectedTags(activeKey);
      return;
    }
    // Adding to the active set: remove from the other set first so a tag is
    // never both included and excluded, then add.
    if (this.tagSelectMode === 'exclude') {
      if (inInclude) this.localSettingsService.removeFromSelectedTags(includeKey);
      this.localSettingsService.addToSelectedTags(excludeKey);
    } else {
      if (inExclude) this.localSettingsService.removeFromSelectedTags(excludeKey);
      this.localSettingsService.addToSelectedTags(includeKey);
    }
  }

  isTagSelected(tag: string): boolean {
    return this.selectedTags.findIndex(x => x === tag) > -1;
  }

  isTagExcluded(tag: string): boolean {
    return this.isTagSelected('!' + tag);
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
    const gen = ++this.tagCountGeneration;
    this.tagCounts = {};
    allTags.forEach(tag => {
      this.tagCounts[tag] = {count: 0, total: 0};
    });
    const list = this.mediaListService.mediaList;
    if (!list) return;
    // Re-entrant: abort if a newer update started
    for (let i = 0; i < list.length; i++) {
      if (this.tagCountGeneration !== gen) return;
      const tags = list[i].metaData.tags;
      for (let j = 0; j < tags.length; j++) {
        const tc = this.tagCounts[tags[j]];
        if (tc) tc.total++;
      }
    }
  }

  private updateTagCountsDisplayed(allTags: string[]): void {
    const gen = ++this.tagCountGeneration;
    allTags.forEach(tag => {
      if(!this.tagCounts[tag]) {
        this.tagCounts[tag] = {count: 0, total: 0};
      } else {
        this.tagCounts[tag].count = 0;
      }
    });
    const list = this.mediaListService.mediaListDisplayed;
    if (!list) return;
    // Re-entrant: abort if a newer update started
    for (let i = 0; i < list.length; i++) {
      if (this.tagCountGeneration !== gen) return;
      const tags = list[i].metaData.tags;
      for (let j = 0; j < tags.length; j++) {
        const tc = this.tagCounts[tags[j]];
        if (tc) tc.count++;
      }
    }
  }

  // private setRandom() {
  //   if(this.randomMode) {
  //     this.nonRandomSort = this.currentSort;
  //     this.localSettingsService.setSort(SortMode.Random)
  //   } else
  //     this.localSettingsService.setSort(this.nonRandomSort)
  // }
}