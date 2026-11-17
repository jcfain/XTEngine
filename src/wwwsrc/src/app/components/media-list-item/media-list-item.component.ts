import { Component, AfterViewChecked, ElementRef, EventEmitter, input, Input, OnChanges, OnInit, Output, ChangeDetectionStrategy, QueryList, ViewChild, ViewChildren } from '@angular/core';
import { SimpleChanges } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MediaItem, ScriptInfo } from '../../models/interfaces';
import { ScriptContainerType, ScriptType, ThumbState } from '../../models/enums';
import { VideoService } from '../../services/video.service';
import { ClickStopPropagation } from '../../directives/stop-propegation';
import { LazySrcDirective } from '../../directives/lazy-src';
import { MediaListItemContextMenuComponent } from '../media-list-item-context-menu/media-list-item-context-menu.component';
import { GenericOverlayService } from '../../services/overlay.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { MediaListService } from '../../services/media-list-service';
import { MediaItemPropertiesComponent } from '../media-item-properties/media-item-properties.component';
import { Subscription } from 'rxjs';

@Component({
    selector: 'app-media-list-item',
    imports: [FormsModule, ClickStopPropagation, LazySrcDirective, MediaListItemContextMenuComponent],
    templateUrl: './media-list-item.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list-item.component.css'
})
export class MediaListItemComponent implements OnInit, OnChanges, AfterViewChecked {
  @Input() 
  item!: MediaItem;
  @Input() 
  width!: number;
  @Input() 
  height!: number;
  @Input() 
  thumbHeight!: number;
  @Input() 
  thumbSize!: number;
  @Input()
  textHeight!: string;
  @Input()
  fontSize!: string;
  @Input()
  tagsHeight!: string;
  @Input()
  tagsFontSize!: string;

  @Output()
  click = new EventEmitter<MediaItem>()
  @Output()
  update = new EventEmitter<MediaItem>()

  altScripts: ScriptInfo[] = [];
  // All tags for the current item, and how many are hidden behind the "+<n>"
  // overflow badge (see computeVisibleTags()).
  tags: string[] = [];
  overflowCount = 0;

  // The tags row is measured against these DOM nodes to decide how many tags
  // fit within half the item width.
  @ViewChildren('tagRef') tagEls: QueryList<ElementRef<HTMLElement>> = new QueryList();
  @ViewChild('tagsContainer') tagsContainer?: ElementRef<HTMLElement>;
  @ViewChild('overflowBadge') overflowBadgeEl?: ElementRef<HTMLElement>;
  // Set when the item or its dimensions change so the tag truncation is
  // re-measured on the next change-detection pass (ngAfterViewChecked).
  private tagsDirty = false;
  // Bounded number of re-measure passes (the "+<n>" badge width is measured
  // from the DOM, which changes as the count changes).
  private tagsMeasurePasses = 0;
  private static readonly MAX_TAG_MEASURE_PASSES = 4;
  // Gap between tag chips - must match the CSS gap on .media-item-tags.
  private static readonly TAG_GAP_PX = 4;

  get widthPX(): string {
    return this.width + "px";
  }
  get heightPX(): string {
    return this.height + "px";
  }
  get thumbHeightPX(): string {
    return this.thumbHeight + "px";
  }
  // Tags occupy half the item width (see .media-item-tags).
  get tagsWidthPX(): string {
    return (this.width / 2) + "px";
  }
  get contextMenuHidden(): boolean {
    return !this.mediaListService.isContextMenuOpen(this.item.id);
  }
  get headerButtonPX() {
    return this.scaleDim(this.thumbSize, 1.03 * this.mediaItemHeaderIconMultiplier, 20) + "px";
  }
  get headerIconPX(): string {
    return this.scaleDim(this.thumbSize, 0.78 * this.mediaItemHeaderIconMultiplier, 18) + "px";
  }
  // Smaller buttons/icons for the favorite + context actions in the name row.
  // (1.03 is the item-height / thumb-size ratio at the 250px reference, so the
  // 250px size is preserved; see scaleDim.)
  get actionButtonPX() {
    return this.scaleDim(this.thumbSize, 1.03 * 0.11, 18) + "px";
  }
  actionIconDimension(): string {
    return this.scaleDim(this.thumbSize, 0.78 * 0.11, 12) + "px";
  }
  // Font size for the (circular) alternate-script count badge.
  altBadgeFontDimension(): string {
    return this.scaleDim(this.thumbSize, 0.78 * this.mediaItemHeaderIconMultiplier * 0.65, 10) + "px";
  }
  // Non-linear dimension scaling (mirrors the font scaling in the media list):
  // linear at/above the 250px reference, a gentler power curve below it, with a
  // floor so buttons/icons stay usable on small/mobile thumbs.
  private scaleDim(thumbSize: number, linearFactor: number, minDim: number): number {
    const REF_THUMB = 250;
    const POWER = 0.6;
    if (thumbSize >= REF_THUMB) {
      return thumbSize * linearFactor;
    }
    const refDim = REF_THUMB * linearFactor;
    return Math.max(minDim, refDim * Math.pow(thumbSize / REF_THUMB, POWER));
  }
  private playingMediaItem: MediaItem | null = null;
  private mediaItemHeaderIconMultiplier = 0.15;
  private mediaItemHeaderMultiplier = 0.2;
  private subscriptions = new Subscription();
  // Hover state for animated thumbs: while not hovered they show a static
  // first frame (see thumbPath()).
  private thumbHovered = false;
  // Local setting: animate GIF/WebP thumbs only on hover (default) or always.
  private animateGifsOnHover = true;

  // Long-press opens the item's context menu on touch devices - the touch
  // equivalent of desktop right-click. The browser's built-in long-press
  // callout is suppressed by CSS on .media-item (see -webkit-touch-callout).
  private static readonly LONG_PRESS_MS = 500;
  private static readonly LONG_PRESS_SLOP_PX = 10;
  private longPressTimer: ReturnType<typeof setTimeout> | null = null;
  private longPressStartX = 0;
  private longPressStartY = 0;
  // Set when the long-press fires: the gesture ends with a synthetic click
  // (and a contextmenu event) that must not be treated as a tap / right-click.
  private lastLongPressTime = 0;

  // private mediaItemHeaderMultiplier: number = 0.15;
  // private mediaItemHeaderIconMultiplier: number = 0.10;

  constructor(
    private videoService: VideoService,
    private overlayService: GenericOverlayService,
    private localSettingsService: LocalSettingsService,
    private mediaListService: MediaListService
  ) {
    this.subscriptions.add(
      this.videoService.playingMediaItem$.subscribe(v => {
        this.playingMediaItem = v;
      })
    );
    this.subscriptions.add(
      this.localSettingsService.animateGifsOnHover$.subscribe(v => {
        this.animateGifsOnHover = v;
      })
    );
  }

  ngOnInit(): void {
  }

  // The component is recycled by the virtual scroller (and reused when an
  // item is updated in place) so re-derive item state whenever the input changes
  ngOnChanges(changes: SimpleChanges): void {
    if (changes['item'] && this.item) {
      this.altScripts = [];
      this.filterAltScripts(this.item);
      this.tags = this.item.metaData?.tags ? [...this.item.metaData.tags] : [];
      this.overflowCount = 0;
      this.tagsMeasurePasses = 0;
      this.tagsDirty = true;
      // The component is recycled, so a stale hover state from the previous
      // item would keep the new item's animated thumb playing.
      this.thumbHovered = false;
    }
    // The tag row is measured in pixels, so any change to the item's size
    // (e.g. the user changing the thumb size) needs a re-measure.
    if (changes['width'] || changes['tagsHeight'] || changes['tagsFontSize']) {
      this.tagsMeasurePasses = 0;
      this.tagsDirty = true;
    }
  }

  // Measure the tags once the DOM reflects the current item/dimensions and
  // hide the ones that don't fit within half the item width (reserving room
  // for the "+<n>" overflow badge). Guarded by tagsDirty so it only runs when
  // something relevant changed, not on every change-detection pass. Returns
  // false if the layout isn't ready yet so the dirty flag is kept for a retry.
  ngAfterViewChecked(): void {
    if (!this.tagsDirty) {
      return;
    }
    if (this.computeVisibleTags()) {
      this.tagsDirty = false;
    }
  }

  private computeVisibleTags(): boolean {
    const container = this.tagsContainer?.nativeElement;
    if (!container) {
      return false;
    }
    const els = this.tagEls ? this.tagEls.toArray() : [];
    if (els.length === 0) {
      this.overflowCount = 0;
      return true; // no tags to truncate
    }
    // Reset visibility first so we measure the true width of every tag (a
    // tag hidden from a previous item would otherwise report 0).
    els.forEach(el => el.nativeElement.style.display = '');
    const containerWidth = container.clientWidth;
    if (containerWidth <= 0) {
      return false; // not laid out yet, retry on the next pass
    }
    const widths = els.map(el => el.nativeElement.offsetWidth);
    const gap = MediaListItemComponent.TAG_GAP_PX;
    // Total width of every tag (with the gaps between them).
    let total = 0;
    widths.forEach((w, i) => total += w + (i > 0 ? gap : 0));
    if (total <= containerWidth) {
      this.overflowCount = 0;
      return true; // everything fits, no overflow badge
    }
    // Reserve room for the "+<n>" badge. Measure its real rendered width once
    // it exists (from a previous pass); fall back to an estimate on the first
    // pass before it has been rendered.
    const badgeShownCount = this.overflowCount; // count the badge currently shows
    const badgeEl = this.overflowBadgeEl?.nativeElement;
    const hasMeasuredBadge = !!(badgeEl && badgeShownCount > 0);
    const badgeWidth = hasMeasuredBadge
      ? badgeEl.offsetWidth
      : this.estimateBadgeWidth(widths.length);
    const available = containerWidth - badgeWidth - gap;
    let acc = 0;
    let fitting = 0;
    for (let i = 0; i < widths.length; i++) {
      const needed = widths[i] + (fitting > 0 ? gap : 0);
      if (acc + needed <= available) {
        acc += needed;
        fitting++;
      } else {
        break;
      }
    }
    const newOverflow = widths.length - fitting;
    els.forEach((el, i) => {
      el.nativeElement.style.display = i < fitting ? '' : 'none';
    });
    this.overflowCount = newOverflow;
    // Re-measure when the badge's shown count no longer matches the computed
    // overflow (its width depends on the count). Bounded to avoid a loop.
    if (newOverflow > 0 && newOverflow !== badgeShownCount &&
        this.tagsMeasurePasses < MediaListItemComponent.MAX_TAG_MEASURE_PASSES) {
      this.tagsMeasurePasses++;
      return false;
    }
    return true;
  }

  // Approximate the rendered width of the "+<n>" badge at the current tag font
  // size (plus the chip's horizontal padding) so we can reserve room for it
  // before deciding how many tags fit.
  private estimateBadgeWidth(overflow: number): number {
    const fontSizePx = parseFloat(this.tagsFontSize) || 12;
    const digits = String(overflow).length;
    return fontSizePx * (0.75 + 0.62 * digits) + 12;
  }


  ngOnDestroy(): void {
    if (this.longPressTimer !== null) {
      clearTimeout(this.longPressTimer);
      this.longPressTimer = null;
    }
    this.subscriptions.unsubscribe();
  }

  onItemClick(event: Event, item: MediaItem): void {
    // A long-press ends with a synthetic click - ignore it and stop it from
    // reaching the document-level handler that would close the context menu
    // the long-press just opened.
    if (Date.now() - this.lastLongPressTime < MediaListItemComponent.LONG_PRESS_MS) {
      event.stopPropagation();
      return;
    }
    this.click.emit(item);
  }

  isPlaying(item: MediaItem): boolean {
    return this.playingMediaItem?.id === item.id;
  }

  getThumbPath(item: MediaItem, staticFrame = false): string {
      const thumbFile = "/thumb/";
      const cacheBust = item.thumbVersion ? `?v=${item.thumbVersion}` : "";
      // "static=1" asks the server for a static (first frame) version of
      // animated thumbs. Only applied to the real thumb paths - the
      // loading/error placeholders are static images.
      const staticParam = staticFrame ? (item.thumbVersion ? "&static=1" : "?static=1") : "";
      switch(item.thumbState)
      {
        case ThumbState.Waiting:
          return thumbFile + item.thumbFileLoading;
        case ThumbState.Error:
          return thumbFile + item.thumbFileError;
        case ThumbState.Loading:
          return thumbFile + item.thumbFileLoadingCurrent;
        case ThumbState.Ready:
          return thumbFile + item.relativeThumb + cacheBust + staticParam;
        case ThumbState.Unknown:
          return thumbFile + item.thumbFileUnknown;
        default:
				  return thumbFile + item.relativeThumb + cacheBust + staticParam;

      }
  }

  // Thumb src for the <img>: animated (GIF/WebP) thumbs show a static first
  // frame until the item is hovered, unless the local setting restores the
  // old always-animate behavior.
  thumbPath(): string {
    if (this.animateGifsOnHover && this.item?.animatedThumb && !this.thumbHovered) {
      return this.getThumbPath(this.item, true);
    }
    return this.getThumbPath(this.item);
  }

  onThumbMouseEnter(): void {
    this.thumbHovered = true;
  }

  onThumbMouseLeave(): void {
    this.thumbHovered = false;
  }

  getItemClass(item: MediaItem): string {
    let ret = "";
    if(item.metaData.isSFMA) {
        ret += "media-item-sfma";
    } else if (item.isMFS && item.hasScript) {
        ret += "media-item-mfs";
    } else if(item.isMFS && !item.hasScript) {
        ret += "media-item-mfs-noscript";
    } else if(item.metaData.hasAlternate && !item.hasScript) {
        ret += "media-item-alt-noscript";
    } else if (!item.hasScript) {
        ret += "media-item-noscript";
    } else {
        // ret += "media-item-noscript";
    }
    if(this.isPlaying(item))
      ret += " media-item-playing";
    return ret;
  }

  getItemName(item: MediaItem): string {
    let ret = "";
    if(item.metaData.isSFMA) {
        ret = "(SFMA) ";
    } else if (item.isMFS && item.hasScript) {
        ret = "(MFS) ";
    } 
    ret += item.name;
    return ret; 
  }
  
  headerButtonDimension(): string {
    return this.thumbHeight * this.mediaItemHeaderMultiplier + "px"; //28.75
  }

  headerButtonIconDimension(): string {
    return this.scaleDim(this.thumbSize, 0.78 * this.mediaItemHeaderIconMultiplier, 18) + "px";
  }

  hasWatched(): boolean {
    return this.item.metaData.tags?.findIndex(x => x == "viewed") > -1;
  }

  isFavorite(): boolean {
    return this.item.metaData.tags?.findIndex(x => x == "favorite") > -1;
  }

  hasSubtitle(): boolean {
    return this.item.metaData.subtitle != null && this.item.metaData.subtitle.length > 0;
  }

  hasAltScripts(): boolean {
    return this.altScripts.length > 0;
  }

  altScriptCount(): number {
    return this.altScripts.length;
  }

	toggleFavorite() {
    const index = this.item.metaData.tags?.findIndex(x => x == "favorite")
		if(index > -1) {
			this.item.metaData.tags.splice(index, 1);
		} else {
			this.item.metaData.tags.push("favorite");
		}
    // The item reference is unchanged (mutated in place), so refresh the tags
    // row manually - the "favorite" tag is shown with the other tags.
    this.tags = this.item.metaData.tags ? [...this.item.metaData.tags] : [];
    this.overflowCount = 0;
    this.tagsMeasurePasses = 0;
    this.tagsDirty = true;
    this.update.emit(this.item);
	};

  toggleContextMenu() {
    if (this.mediaListService.isContextMenuOpen(this.item.id)) {
      this.mediaListService.closeContextMenu();
    } else {
      this.mediaListService.openContextMenu(this.item.id);
    }
  }

  // Long-press gesture: a single touch held still for LONG_PRESS_MS opens the
  // context menu. NOT preventDefault() on touchstart so scrolling and the
  // browser's system gestures (e.g. Android edge swipes) keep working.
  onItemTouchStart(event: TouchEvent): void {
    if (event.touches.length !== 1) {
      return;
    }
    this.longPressStartX = event.touches[0].clientX;
    this.longPressStartY = event.touches[0].clientY;
    if (this.longPressTimer !== null) {
      clearTimeout(this.longPressTimer);
    }
    this.longPressTimer = setTimeout(() => {
      this.longPressTimer = null;
      this.lastLongPressTime = Date.now();
      // Haptic feedback (Android only, no-op elsewhere).
      navigator.vibrate?.(10);
      if (!this.mediaListService.isContextMenuOpen(this.item.id)) {
        this.mediaListService.openContextMenu(this.item.id);
      }
    }, MediaListItemComponent.LONG_PRESS_MS);
  }

  onItemTouchMove(event: TouchEvent): void {
    if (this.longPressTimer === null || event.touches.length !== 1) {
      return;
    }
    const dx = event.touches[0].clientX - this.longPressStartX;
    const dy = event.touches[0].clientY - this.longPressStartY;
    if (Math.hypot(dx, dy) > MediaListItemComponent.LONG_PRESS_SLOP_PX) {
      // The finger moved - it's a scroll, not a long-press.
      clearTimeout(this.longPressTimer);
      this.longPressTimer = null;
    }
  }

  onItemTouchEnd(): void {
    if (this.longPressTimer !== null) {
      clearTimeout(this.longPressTimer);
      this.longPressTimer = null;
    }
  }

  onItemContextMenu(event: MouseEvent): void {
    // Suppress the browser's system context menu (fires on desktop right-click
    // and after a long-press on touch devices).
    event.preventDefault();
    // Android fires contextmenu right after a long-press; the touch timer
    // already handled it, so ignore the event here.
    if (Date.now() - this.lastLongPressTime < MediaListItemComponent.LONG_PRESS_MS) {
      return;
    }
    this.toggleContextMenu();
  }

  // Opens the media properties dialog on the Tags tab for this item - the
  // "+<n>" tag overflow badge is a shortcut to it.
  openTagsProperties() {
    this.localSettingsService.setLastSelectedPropertiesTab(1); // 1 = Tags tab
    this.overlayService.open('mediaItemProperties', MediaItemPropertiesComponent, {
      item: this.item
    });
  }

  private filterAltScripts(item: MediaItem) {
    var scripts = item.metaData.scripts;
    if(!scripts || !scripts.length) {
      return;
    }
    
    scripts.forEach(x => {
      if((x.containerType == ScriptContainerType.MFS || x.containerType == ScriptContainerType.ZIP) && x.type == ScriptType.MAIN)
      {
        return;
      }
      if(x.type != ScriptType.MAIN) // Dont count the "Default" main script// TODO is this needed?
        this.altScripts.push(x);
    });
  }
}