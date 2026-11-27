import { Injectable } from '@angular/core';
import { BehaviorSubject, Observable } from 'rxjs';

/**
 * UI-only state of the media toolbar. The toolbar is rendered in two places
 * (top of the page and as the full screen media sheet's header) and both
 * instances must show the same open drawers, tag filter text and tag select
 * mode, so that state lives here instead of in the component. (Everything
 * data-driven - filter text, sort/show/thumb settings, tags - already comes
 * from the shared services and syncs for free.)
 */
export interface MediaToolbarState {
    tagInputHidden: boolean;
    sortInputHidden: boolean;
    showInputHidden: boolean;
    thumbSizeInputHidden: boolean;
    tcodeActionsInputHidden: boolean;
    tagFilterText: string;
    tagSelectMode: 'include' | 'exclude';
}

@Injectable({ providedIn: 'root' })
export class MediaToolbarStateService {
    private stateSubject = new BehaviorSubject<MediaToolbarState>({
        tagInputHidden: true,
        sortInputHidden: true,
        showInputHidden: true,
        thumbSizeInputHidden: true,
        tcodeActionsInputHidden: true,
        tagFilterText: '',
        tagSelectMode: 'include'
    });
    state$: Observable<MediaToolbarState> = this.stateSubject.asObservable();

    get state(): MediaToolbarState {
        return this.stateSubject.value;
    }

    update(partial: Partial<MediaToolbarState>): void {
        this.stateSubject.next({ ...this.stateSubject.value, ...partial });
    }
}