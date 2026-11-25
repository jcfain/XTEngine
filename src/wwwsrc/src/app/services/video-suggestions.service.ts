import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';
import { MediaItem } from '../models/interfaces';
import { MediaListService } from './media-list-service';
import { VideoService } from './video.service';

/**
 * Computes a YouTube-style "similar videos" list for the currently playing
 * item. Similarity comes from the item's metadata tags (weighted by how rare
 * each tag is across the library, so ubiquitous tags like "MFS" barely count)
 * plus name-token and creator overlap. The result is best-match-first but
 * randomly ordered within score bands.
 *
 * Performance: a tag document-frequency index and per-item name token sets
 * are built once per library load. Scoring runs only when the playing item
 * changes (debounced, so rapid switches don't recompute) and is a single O(N)
 * pass, so it stays cheap even for large libraries.
 */
@Injectable({ providedIn: 'root' })
export class VideoSuggestionsService {
  private static readonly MAX_SUGGESTIONS = 100;
  // The sorted list is split into bands of this size and each band is
  // shuffled - best matches first, random order within a band.
  private static readonly BAND_SIZE = 10;
  // Below this many real matches the list is topped up with a random sample
  // so the panel is never empty.
  private static readonly MIN_MATCHES = 10;
  private static readonly COMPUTE_DEBOUNCE_MS = 150;

  private suggestionsSubject = new BehaviorSubject<MediaItem[]>([]);
  suggestions$: Observable<MediaItem[]> = this.suggestionsSubject.asObservable();

  private tagDocFreq = new Map<string, number>();
  private nameTokens = new Map<string, Set<string>>();
  private librarySize = 0;
  private computeTimer: number | null = null;

  constructor(
    private mediaListService: MediaListService,
    private videoService: VideoService
  ) {
    this.mediaListService.mediaList$.subscribe(list => {
      this.buildIndexes(list ?? []);
      this.scheduleCompute();
    });
    this.videoService.playingMediaItem$.subscribe(() => {
      this.scheduleCompute();
    });
  }

  /** Re-randomize the ordering (the scores are deterministic, so this just
   *  re-shuffles within the score bands). */
  reshuffle(): void {
    this.compute();
  }

  private scheduleCompute(): void {
    if (this.computeTimer !== null) {
      return;
    }
    this.computeTimer = window.setTimeout(() => {
      this.computeTimer = null;
      this.compute();
    }, VideoSuggestionsService.COMPUTE_DEBOUNCE_MS);
  }

  private buildIndexes(list: MediaItem[]): void {
    this.tagDocFreq.clear();
    this.nameTokens.clear();
    this.librarySize = list.length;
    for (const item of list) {
      for (const tag of item.metaData?.tags ?? []) {
        this.tagDocFreq.set(tag, (this.tagDocFreq.get(tag) ?? 0) + 1);
      }
      this.nameTokens.set(item.id, this.tokenize(item.name));
    }
  }

  private compute(): void {
    const playing = this.videoService.playingMediaItem;
    const list = this.mediaListService.mediaList ?? [];
    if (!playing || list.length === 0) {
      this.suggestionsSubject.next([]);
      return;
    }

    const playingTags = new Set(playing.metaData?.tags ?? []);
    const playingTokens = this.nameTokens.get(playing.id) ?? this.tokenize(playing.name);
    const playingCreator = (playing.metaData?.creator ?? '').trim().toLowerCase();

    const scored: { item: MediaItem; score: number }[] = [];
    for (const item of list) {
      if (item.id === playing.id) {
        continue;
      }
      let score = 0;
      // Rarity-weighted shared tags (inverse document frequency): a tag on
      // few items is more meaningful than one on half the library.
      for (const tag of item.metaData?.tags ?? []) {
        if (playingTags.has(tag)) {
          const docFreq = this.tagDocFreq.get(tag) ?? 1;
          score += Math.max(0, Math.log(this.librarySize / (1 + docFreq)));
        }
      }
      // Name token overlap.
      for (const token of this.nameTokens.get(item.id) ?? this.tokenize(item.name)) {
        if (playingTokens.has(token)) {
          score += 0.5;
        }
      }
      // Same creator.
      if (playingCreator && (item.metaData?.creator ?? '').trim().toLowerCase() === playingCreator) {
        score += 1;
      }
      if (score > 0) {
        scored.push({ item, score });
      }
    }

    // Too few real matches: top up with a random sample so the panel isn't
    // empty (the sample scores 0, so it sorts after every real match).
    if (scored.length < VideoSuggestionsService.MIN_MATCHES) {
      const matched = new Set(scored.map(s => s.item.id));
      const rest = list.filter(m => m.id !== playing.id && !matched.has(m.id));
      this.shuffle(rest);
      for (const item of rest) {
        if (scored.length >= VideoSuggestionsService.MAX_SUGGESTIONS) {
          break;
        }
        scored.push({ item, score: 0 });
      }
    }

    // Best matches first, randomly ordered within each band.
    scored.sort((a, b) => b.score - a.score);
    const top = scored.slice(0, VideoSuggestionsService.MAX_SUGGESTIONS);
    const result: MediaItem[] = [];
    for (let i = 0; i < top.length; i += VideoSuggestionsService.BAND_SIZE) {
      const band = top.slice(i, i + VideoSuggestionsService.BAND_SIZE);
      this.shuffle(band);
      for (const entry of band) {
        result.push(entry.item);
      }
    }
    this.suggestionsSubject.next(result);
  }

  private tokenize(name: string): Set<string> {
    const tokens = new Set<string>();
    for (const part of (name ?? '').toLowerCase().split(/[^a-z0-9]+/)) {
      if (part.length >= 2) {
        tokens.add(part);
      }
    }
    return tokens;
  }

  // Fisher-Yates (in place).
  private shuffle<T>(items: T[]): void {
    let currentIndex = items.length;
    while (currentIndex !== 0) {
      const randomIndex = Math.floor(Math.random() * currentIndex);
      currentIndex--;
      [items[currentIndex], items[randomIndex]] = [items[randomIndex], items[currentIndex]];
    }
  }
}