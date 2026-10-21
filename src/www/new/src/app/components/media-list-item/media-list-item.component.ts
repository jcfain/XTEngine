import { Component, EventEmitter, input, Input, OnInit, Output, ChangeDetectionStrategy } from '@angular/core';
import { NgOptimizedImage } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaItem, ScriptInfo } from '../../models/interfaces';
import { ScriptContainerType, ScriptType, ThumbState } from '../../models/enums';
import { VideoService } from '../../services/video.service';
import { ClickStopPropagation } from '../../directives/stop-propegation';
import { MediaListItemContextMenuComponent } from '../media-list-item-context-menu/media-list-item-context-menu.component';
import { MessageHandlerService } from '../../services/message-handler.service';

@Component({
    selector: 'app-media-list-item',
    imports: [FormsModule, NgOptimizedImage, ClickStopPropagation, MediaListItemContextMenuComponent],
    templateUrl: './media-list-item.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './media-list-item.component.css'
})
export class MediaListItemComponent implements OnInit {
  @Input() 
  item!: MediaItem;
  @Input() 
  width!: number;
  @Input() 
  height!: number;
  @Input() 
  thumbHeight!: number;
  @Input()
  textHeight!: string;
  @Input()
  fontSize!: string;

  @Output()
  click = new EventEmitter<MediaItem>()
  @Output()
  update = new EventEmitter<MediaItem>()

  altScripts: ScriptInfo[] = [];

  get widthPX(): string {
    return this.width + "px";
  }
  get heightPX(): string {
    return this.height + "px";
  }
  get thumbHeightPX(): string {
    return this.thumbHeight + "px";
  }
  get contextMenuHidden(): boolean {
    return this.m_contextMenuHidden;
  }
  private playingMediaItem: MediaItem | null = null;
  private mediaItemHeaderIconMultiplier = 0.15;
  private mediaItemHeaderMultiplier = 0.2;
  private m_contextMenuHidden = true;

  // private mediaItemHeaderMultiplier: number = 0.15;
  // private mediaItemHeaderIconMultiplier: number = 0.10;

  constructor(
    private messageHandlerService: MessageHandlerService,
    private videoService: VideoService
  ) {
    this.videoService.playingMediaItem$.subscribe(v => {
      this.playingMediaItem = v;
    });
    this.messageHandlerService.updateThumb$.subscribe(v => {
      if(this.item)
        this.item.thumbVersion = Date.now();
    });
  }

  ngOnInit(): void {
    this.filterAltScripts(this.item);
  }

  onItemClick(item: MediaItem): void {
    this.click.emit(item);
  }

  isPlaying(item: MediaItem): boolean {
    return this.playingMediaItem?.id === item.id;
  }

  getThumbPath(item: MediaItem): string {
      const thumbFile = "/thumb/";
      const cacheBust = item.thumbVersion ? `?v=${item.thumbVersion}` : "";
      switch(item.thumbState)
      {
        case ThumbState.Error:
          return thumbFile + item.thumbFileError;
        case ThumbState.Loading:
          return thumbFile + item.thumbFileLoading;
        case ThumbState.Ready:
          return thumbFile + item.relativeThumb + cacheBust;
        case ThumbState.Unknown:
          return thumbFile + item.thumbFileUnknown;
        default:
				  return thumbFile + item.relativeThumb + cacheBust;

      }
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
  
  headerButtonDimension(): string {
    return this.thumbHeight * this.mediaItemHeaderMultiplier + "px"; //28.75
  }

  headerButtonIconDimension(): string {
		// let widthInt = this.thumbHeight + (this.thumbHeight * this.mediaItemHeaderMultiplier);
    return this.thumbHeight * this.mediaItemHeaderIconMultiplier + "px"; //28.75
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
    this.update.emit(this.item);
	};

  toggleConextMenu() {
    this.m_contextMenuHidden = !this.m_contextMenuHidden;
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