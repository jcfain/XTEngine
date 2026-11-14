import { Component, ElementRef, EmbeddedViewRef, EventEmitter, Input, OnChanges, OnDestroy, OnInit, Output, AfterViewInit, ChangeDetectionStrategy, SimpleChanges, TemplateRef, ViewChild, ViewContainerRef } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MediaItem, ScriptInfo } from '../../models/interfaces';
import { VideoService } from '../../services/video.service';
import { WebsocketService } from '../../services/websocket.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { MediaType } from '../../models/enums';
import { AlertComponent } from '../alert-model/alert.component';
import { GenericOverlayService } from '../../services/overlay.service';
import { SettingsService } from '../../services/settings.service';
import { MediaItemPropertiesComponent } from '../media-item-properties/media-item-properties.component';
import { MediaItemAltScriptsComponent } from '../media-item-alt-scripts/media-item-alt-scripts.component';
import { MediaListOverlayService } from '../../services/media-list-overlay.service';
import { Subscription } from 'rxjs';
import { AlertService } from '../../services/alert.service';

@Component({
    selector: 'app-media-list-item-context-menu',
    imports: [FormsModule],
    templateUrl: './media-list-item-context-menu.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list-item-context-menu.component.css'
})
export class MediaListItemContextMenuComponent implements OnInit, OnChanges, AfterViewInit, OnDestroy {
  @Input() 
  item!: MediaItem;
  @Input()
  altScriptCount!: number;
  @Input()
  fontSize: string = "21.875px";
  @Input()
  hidden: boolean = true;

  @Output()
  close = new EventEmitter()

  // The menu is rendered into a portal inside the media list container (see
  // ngAfterViewInit) so that on touch devices it can be a true full-screen
  // fixed overlay. The media list uses a transformed virtual-scroll viewport,
  // which would otherwise become the containing block for position:fixed.
  // Portaling into app-media-list (not document.body) keeps the menu visible
  // in full-screen mode, where only the full-screen element's subtree renders.
  @ViewChild('menuTemplate') menuTemplate?: TemplateRef<any>;
  private menuHostEl?: HTMLElement;
  private embeddedMenuView?: EmbeddedViewRef<any>;

  altScripts: ScriptInfo[] = [];
  mediaManagementEnabled: boolean = false;

  private playingMediaItem: MediaItem | null = null;
  private deleteAlertNameID: number = -1;
  private overlaySub?: Subscription;

  constructor(
    private settingsService: SettingsService,
    private alertService: AlertService,
    private overlayService: GenericOverlayService,
    private videoService: VideoService,
    private websocketService: WebsocketService,
    private messageService: MessageHandlerService,
    private viewContainerRef: ViewContainerRef,
    private elementRef: ElementRef,
    private mediaListOverlayService: MediaListOverlayService
  ) {
    this.videoService.playingMediaItem$.subscribe(v => {
      this.playingMediaItem = v;
    });
    this.messageService.deleteItem$.subscribe(v => {
        this.alertService.closeConfirm(this.deleteAlertNameID );
        this.deleteAlertNameID = -1;
    });
    this.settingsService.mediaManagementEnabled$.subscribe(v => {
      this.mediaManagementEnabled = v;
    });
    // When the full-screen overlay state changes, move the portal host into
    // or out of the full-screen subtree so the menu stays visible.
    this.overlaySub = this.mediaListOverlayService.overlayActive$.subscribe(() => {
      this.repositionPortal();
    });
  }

  ngOnInit(): void {}

  ngAfterViewInit(): void {
    if (!this.menuTemplate) {
      return;
    }
    const host = document.createElement('div');
    host.className = 'media-context-portal';
    host.style.display = this.hidden ? 'none' : '';
    // Portal into the full-screen host (video container) in full-screen mode
    // so the menu is inside the full-screen subtree. In normal mode it goes
    // to document.body. The video container has no transform, so
    // position:fixed works correctly (app-media-list has a transform in
    // overlay mode which would break fixed positioning).
    const portalTarget = this.mediaListOverlayService.getPortalTarget();
    portalTarget.appendChild(host);
    this.menuHostEl = host;
    // Render the menu template and relocate its nodes into the portal host
    // (Angular still owns the view, so change detection keeps working).
    this.embeddedMenuView = this.viewContainerRef.createEmbeddedView(this.menuTemplate);
    this.embeddedMenuView.rootNodes.forEach((node: Node) => host.appendChild(node));
    this.positionMenu();
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes['hidden'] && this.menuHostEl) {
      this.menuHostEl.style.display = this.hidden ? 'none' : '';
      if (!this.hidden) {
        this.positionMenu();
      }
    }
  }

  ngAfterViewChecked(): void {
    // Keep the portaled menu aligned with its item (handles scroll/resize).
    if (!this.hidden && this.menuHostEl) {
      this.positionMenu();
    }
  }

  ngOnDestroy(): void {
    this.overlaySub?.unsubscribe();
    this.menuHostEl?.remove();
  }

  // Move the portal host into the correct parent (full-screen host vs
  // document.body) whenever the overlay state changes.
  private repositionPortal(): void {
    if (!this.menuHostEl) return;
    const target = this.mediaListOverlayService.getPortalTarget();
    if (this.menuHostEl.parentElement !== target) {
      target.appendChild(this.menuHostEl);
    }
  }

  // Position the body-level menu host: full-screen on touch devices, or over
  // the media item on desktop. Sets individual properties (not cssText) so it
  // doesn't clobber the display flag managed by the hidden input.
  private positionMenu(): void {
    if (!this.menuHostEl) {
      return;
    }
    const s = this.menuHostEl.style;
    s.position = 'fixed';
    s.zIndex = '1000';
    if (window.matchMedia('(pointer: coarse)').matches) {
      s.top = '0';
      s.left = '0';
      s.width = '100vw';
      s.height = '100vh';
      return;
    }
    const itemEl = this.elementRef.nativeElement.closest('.media-item') || this.elementRef.nativeElement;
    const rect = itemEl.getBoundingClientRect();
    s.top = `${rect.top}px`;
    s.left = `${rect.left}px`;
    s.width = `${rect.width}px`;
    s.height = `${rect.height}px`;
  }

  isPlaying(): boolean {
    return this.playingMediaItem?.id === this.item.id;
  }

  isThumbReadonly(): boolean {
    return this.item?.type != MediaType.Video && this.item?.type != MediaType.VR || !this.item?.managedThumb;
  }

  getRegenerateThumbTitle(): string {
    if(this.item.type != MediaType.Video && this.item.type != MediaType.VR) {
      return "This is not a media item that has frames";
    }
    return this.item.managedThumb ? "This thumb file is not modifiable because it is not managed by XTP." 
      : "Regenerate this thumb nail to a random time in the video"
  }

  getThumbTitle() {
    return this.isPlaying() ? "This is not the current playing video so it cannot be set at the current position."
    : "Set the thumbnail at the current playing position.";
  }

  getMoneyShotTitle(): string {
    return this.isPlaying() ? "This is not the current playing video so it cannot be set at the current position."
    : "Set the moneyshot time at the current playing position.";
  }

  getAlternateScriptsTitle(): string {
    return this.isPlaying() ? "This is not the current playing video."
    : "View and switch alternate scripts for the playing media item.";
  }

  isDisabled(): boolean {
    return false;
  }

  getItemClass(): string {
    let ret = "";
    // if(this.item.metaData.isSFMA) {
    //     ret += "media-item-sfma";
    // } else if (item.isMFS && item.hasScript) {
    //     ret += "media-item-mfs";
    // } else if(item.isMFS && !item.hasScript) {
    //     ret += "media-item-mfs-noscript";
    // } else if(item.metaData.hasAlternate && !item.hasScript) {
    //     ret += "media-item-alt-noscript";
    // } else if (!item.hasScript) {
    //     ret += "media-item-noscript";
    // } else {
    //     // ret += "media-item-noscript";
    // }
    // if(this.isPlaying(item))
    //   ret += " media-item-playing";
    return ret;
  }

  openMetaDataModal() {
    this.closeEvent();
  }

  openAltScriptsEvent() {
    const nameID = 'mediaItemAltScripts';
    this.overlayService.open(nameID, MediaItemAltScriptsComponent, {
      item: this.item
    });
    this.closeEvent();
  }

  openMetadataEvent() {
    const nameID = 'mediaItemProperties';
    this.overlayService.open(nameID, MediaItemPropertiesComponent, {
      item: this.item
    });
    this.closeEvent();
  }

  processMetadataEvent() {
    this.websocketService.sendUpdateMetadata(this.item.metaData.key);
    this.closeEvent();
  }

  setMoneyShotEvent() {
    this.websocketService.sendUpdateMoneyShotAt(this.item.id, this.videoService.currentPosition);
    this.closeEvent();
  }

  setThumbEvent() {
    this.websocketService.sendUpdateThumbAt(this.item.id, this.videoService.currentPosition);
    this.closeEvent();
  }

  regenerateThumbEvent() {
    this.websocketService.sendUpdateThumb(this.item.id)
    this.closeEvent();
  }

  deleteEvent($event: MouseEvent) {
    if(this.deleteAlertNameID != -1)
      return;
    this.deleteAlertNameID = this.alertService.confirm(
      {
        message: `Are you sure you wish to delete the media? 
              '${this.item.name}'
THIS ACTION CANNOT BE UNDONE!`, 
        header: "Warning!", 
        confirmText: "DELETE", 
        cancelText: "NO WAY!",
        onConfirm: () => {
          this.confirmDeleteEvent();
        },
        onCancel: () => {
          this.rejectDeleteEvent();
          this.alertService.closeConfirm(this.deleteAlertNameID );
          this.deleteAlertNameID = -1;
        }
      }
      //, $event 
    );
    this.closeEvent();
  }

  confirmDeleteEvent() {
    this.websocketService.sendDeleteMediaItem(this.item.id);
  }

  rejectDeleteEvent() {
    this.alertService.closeConfirm(this.deleteAlertNameID);
  }

  closeEvent() {
    this.close.emit();
  }
}
