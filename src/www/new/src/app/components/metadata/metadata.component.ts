import { Component, OnInit, Input, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';
import { MediaService } from '../../services/media.service';
import { AppStateService } from '../../services/app-state.service';
import { MediaItem } from '../../models/interfaces';
import { MediaListService } from '../../services/media-list-service';
import { VideoService } from '../../services/video.service';
import { ServerService } from '../../services/server.service';

interface MetaData {
  offset: number;
  moneyShotMillis: number;
  funscriptModifier: number;
  libraryItemPath: string;
  dateAdded: string;
  hasAlternate: boolean;
  scripts: { filename: string; track: string }[];
  tags: string[];
  [key: string]: any;
}

@Component({
    selector: 'app-metadata',
    imports: [FormsModule],
    templateUrl: './metadata.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './metadata.component.css'
})
export class MetadataComponent implements OnInit {
  @Input() isOpen = false;
  @Output() close = new EventEmitter<void>();

  mediaItem: MediaItem | null = null;
  offset = 0;
  moneyShotMillis = -1;
  funscriptModifier = 0;
  allTags: string[] = [];
  isDirty = false;

  constructor(
    private serverService: ServerService,
    private mediaService: MediaService,
    private videoService: VideoService
  ) {}

  ngOnInit(): void {
    this.serverService.remoteUserSettings$.subscribe(settings => {
      if (settings) {
        this.allTags = (settings as any)['allTags'] || [];
      }
    });
  }

  open(item?: MediaItem): void {
    const mediaItem = item || this.videoService.playingMediaItem;
    if (!mediaItem) return;
    this.mediaItem = mediaItem;
    const meta = (mediaItem as any).metaData as MetaData;
    this.offset = meta?.offset ?? 0;
    this.moneyShotMillis = meta?.moneyShotMillis ?? -1;
    this.funscriptModifier = meta?.funscriptModifier ?? 0;
    this.isDirty = false;
    this.isOpen = true;
  }

  closeModal(): void {
    this.isOpen = false;
    this.close.emit();
  }

  onValueChange(): void {
    this.isDirty = true;
  }

  addToOffset(amount: number): void {
    this.offset += amount;
    this.onValueChange();
  }

  addToModifier(amount: number): void {
    this.funscriptModifier += amount;
    this.onValueChange();
  }

  resetMoneyShot(): void {
    this.moneyShotMillis = -1;
    this.onValueChange();
  }

  save(): void {
    if (!this.mediaItem) return;
    const meta = (this.mediaItem as any).metaData as MetaData;
    meta.offset = this.offset;
    meta.moneyShotMillis = this.moneyShotMillis;
    meta.funscriptModifier = this.funscriptModifier;
    this.mediaService.saveMetaData(this.mediaItem);
    this.isDirty = false;
    this.closeModal();
  }

  toggleTag(tag: string): void {
    if (!this.mediaItem) return;
    const meta = (this.mediaItem as any).metaData as MetaData;
    const index = meta.tags.findIndex(t => t === tag);
    if (index === -1) {
      meta.tags.push(tag);
    } else {
      meta.tags.splice(index, 1);
    }
    this.onValueChange();
  }

  hasTag(tag: string): boolean {
    if (!this.mediaItem) return false;
    const meta = (this.mediaItem as any).metaData as MetaData;
    return meta.tags.includes(tag);
  }

  getScripts(): string[] {
    if (!this.mediaItem) return [];
    const meta = (this.mediaItem as any).metaData as MetaData;
    return (meta.scripts || []).map(s => s.filename + (s.track ? '.' + s.track : ''));
  }

  get libraryItemPath(): string {
    if (!this.mediaItem) return '';
    return ((this.mediaItem as any).metaData as MetaData)?.libraryItemPath || '';
  }

  get dateAdded(): string {
    if (!this.mediaItem) return '';
    return ((this.mediaItem as any).metaData as MetaData)?.dateAdded || '';
  }

  get modifiedDate(): string {
    if (!this.mediaItem) return '';
    return (this.mediaItem as any).modifiedDate || '';
  }
}