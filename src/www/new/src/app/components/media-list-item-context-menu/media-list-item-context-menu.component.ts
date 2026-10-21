import { Component, EventEmitter, input, Input, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MediaItem, ScriptInfo } from '../../models/interfaces';
import { VideoService } from '../../services/video.service';
import { ClickStopPropagation } from '../../directives/stop-propegation';
import { WebsocketService } from '../../services/websocket.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { AlertComponent } from '../model/alert/alert.component';

@Component({
    selector: 'app-media-list-item-context-menu',
    imports: [FormsModule, ClickStopPropagation, AlertComponent],
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
  // @Output()
  // update = new EventEmitter<MediaItem>()

  altScripts: ScriptInfo[] = [];
  deleteAlert = false;

  // get widthPX(): string {
  //   return this.width + "px";
  // }
  // get heightPX(): string {
  //   return this.height + "px";
  // }
  // get thumbHeightPX(): string {
  //   return this.thumbHeight + "px";
  // }
  private playingMediaItem: MediaItem | null = null;
  private mediaItemHeaderIconMultiplier = 0.15;
  private mediaItemHeaderMultiplier = 0.2;

  // private mediaItemHeaderMultiplier: number = 0.15;
  // private mediaItemHeaderIconMultiplier: number = 0.10;

  constructor(
    private videoService: VideoService,
    private websocketService: WebsocketService,
    private messageService: MessageHandlerService
  ) {
    this.videoService.playingMediaItem$.subscribe(v => {
      this.playingMediaItem = v;
    });
    this.messageService.deleteItem$.subscribe(v => {
      this.deleteAlert = false;
    });
  }

  ngOnInit(): void {
  }

  isPlaying(): boolean {
    return this.playingMediaItem?.id === this.item.id;
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

  deleteEvent() {
    this.deleteAlert = true;
    this.closeEvent();
  }

  confirmDeleteEvent() {
    // this.deleteAlert = false;
    // this.websocketService.sendDeleteMediaItem(this.item.id);
  }

  rejectDeleteEvent() {
    this.deleteAlert = false;
  }

  closeEvent() {
    this.close.emit();
  }
}
