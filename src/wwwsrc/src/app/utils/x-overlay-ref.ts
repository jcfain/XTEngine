import { TemplateRef, Type } from "@angular/core";
import {OverlayRef} from '@angular/cdk/overlay';
import { Subject } from "rxjs";

export interface XOverlayCloseEvent<R> {
 type: 'backdropClick' | 'close';
 data: R;
}

export class XOverlayRef<R = any, T = any> {

  afterClosed$ = new Subject<XOverlayCloseEvent<R | undefined>>();

  constructor(public overlay: OverlayRef, public content: string | TemplateRef<any> | Type<any>, public data: T ) {
    overlay.backdropClick().subscribe(() => this._close('backdropClick'));
  }
  
  close(data?: R) {
    this._close('close', data);
  }


  private _close(type: 'backdropClick' | 'close', data?: R) {
    this.overlay.dispose();
    this.afterClosed$.next({
      type,
      data
    });
    this.afterClosed$.complete();
  }

}