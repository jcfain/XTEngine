import { Component, EventEmitter, input, Input, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MediaItem, ScriptInfo } from '../../models/interfaces';
import { VideoService } from '../../services/video.service';
import { ClickStopPropagation } from '../../directives/stop-propegation';
import { WebsocketService } from '../../services/websocket.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { MediaType } from '../../models/enums';
import { AlertComponent } from '../alert-model/alert.component';
import { GenericOverlayService } from '../../services/overlay.service';
import { AppStateService } from '../../services/app-state.service';
import { SettingsService } from '../../services/settings.service';

@Component({
    selector: 'app-media-list-item-context-menu',
    imports: [FormsModule, ClickStopPropagation],
    templateUrl: './media-list-item-context-menu.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list-item-context-menu.component.css'
})
export class MediaListItemContextMenuComponent implements OnInit {
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

  altScripts: ScriptInfo[] = [];
  mediaManagementEnabled: boolean = false;

  private playingMediaItem: MediaItem | null = null;

  constructor(
    private settingsService: SettingsService,
    private overlayService: GenericOverlayService,
    private videoService: VideoService,
    private websocketService: WebsocketService,
    private messageService: MessageHandlerService
  ) {
    this.videoService.playingMediaItem$.subscribe(v => {
      this.playingMediaItem = v;
    });
    this.messageService.deleteItem$.subscribe(v => {
      this.overlayService.close();
    });
    this.settingsService.mediaManagementEnabled$.subscribe(v => {
      this.mediaManagementEnabled = v;
    });
  }

  ngOnInit(): void {}  

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
    // this.openAltScripts.emit();
    this.closeEvent();
  }

  openMetadataEvent() {
    this.closeEvent();
  }

  processMetadataEvent() {
    this.websocketService.sendUpdateMetadata(this.item.id);
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
    this.overlayService.open(AlertComponent, 
      {
        content: `Are you sure you wish to delete the media? 
              '${this.item.name}'
THIS ACTION CANNOT BE UNDONE!`, 
        titleText: "Warning!", 
        acceptText: "DELETE", 
        rejectText: "NO WAY!",
        onAccept: () => {this.confirmDeleteEvent();},
        onReject: () => {this.rejectDeleteEvent();},
        onClose:  () => {this.overlayService.close();},
      }
      //, $event 
    );
    this.closeEvent();
  }

  confirmDeleteEvent() {
    this.websocketService.sendDeleteMediaItem(this.item.id);
  }

  rejectDeleteEvent() {
    this.overlayService.close();
  }

  closeEvent() {
    this.close.emit();
  }
}
