import { Component, EventEmitter, input, Input, OnInit, Output } from '@angular/core';
import { CommonModule, NgOptimizedImage } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { MediaItem } from '../../models/interfaces';
import { ThumbState } from '../../models/enums';
import { MediaListService } from '../../services/media-list-service';

@Component({
  selector: 'app-media-list-item',
  standalone: true,
  imports: [CommonModule, FormsModule, NgOptimizedImage],
  templateUrl: './media-list-item.component.html',
  styleUrl: './media-list-item.component.css'
})
export class MediaListItemComponent implements OnInit {
  @Input() 
  item!: MediaItem;
  @Input() 
  width!: string;
  @Input() 
  height!: string;
  @Input() 
  thumbHeight!: string;
  @Input()
  textHeight!: string;
  @Input()
  fontSize!: string;

  @Output()
  click = new EventEmitter<MediaItem>()

  // private mediaItemHeaderMultiplier: number = 0.15;
  // private mediaItemHeaderIconMultiplier: number = 0.10;

  constructor(
    private mediaListService: MediaListService
  ) {
  }

  ngOnInit(): void { }

  onItemClick(item: MediaItem): void {
    this.click.emit(item);
  }

  isPlaying(item: MediaItem): boolean {
    return this.mediaListService.playingMediaItem?.id === item.id;
  }

  getThumbPath(item: MediaItem): string {
			if (item.thumbFileExists)
				return "/thumb/" + item.relativeThumb;
			else 
				return "/thumb/" + (item.thumbState === ThumbState.Unknown ? item.thumbFileUnknown : item.thumbState === ThumbState.Error ? item.thumbFileError : item.thumbFileLoading);
  }
}