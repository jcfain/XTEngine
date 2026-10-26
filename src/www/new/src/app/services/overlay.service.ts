// import { ComponentType, Overlay, OverlayConfig } from "@angular/cdk/overlay";
// import { Injector, TemplateRef, Type } from "@angular/core";
// import { ComponentPortal } from '@angular/cdk/portal';
// import { XOverlayRef } from "../utils/x-overlay-ref";


// export class OverlayService {
//   constructor(private overlay: Overlay, private injector: Injector) {}
  
//   open<C, R = any, T = any>(component: ComponentType<C>, content: string | TemplateRef<any> | Type<any>,data: T): XOverlayRef<R> {
      
//     const configs = new OverlayConfig({
//       hasBackdrop: true,
//       panelClass: ['modal', 'is-active'],
//       backdropClass: 'modal-background'
//     });

//     const overlayRef = this.overlay.create(configs);
//     const xOverlayRef = new XOverlayRef<R, T>(overlayRef, content, data);
//     const injector = this.createInjector(xOverlayRef, this.injector);
//     overlayRef.attach(new ComponentPortal(component, null, injector));
//     return xOverlayRef;
//   }
  
//   private createInjector(ref: XOverlayRef, inj: Injector): Injector {
//     return Injector.create({
//       parent: inj,
//       providers: [
//         { provide: XOverlayRef, useValue: ref },
//       ],
//     });
//   }
// }



// https://medium.com/@ahmeddin_77866/creating-a-generic-overlay-service-in-angular-a-comprehensive-guide-0bb43962126c

import { Overlay, OverlayRef } from '@angular/cdk/overlay';
import { ComponentPortal } from '@angular/cdk/portal';
import { Injectable, Injector, Type } from '@angular/core';

@Injectable({ providedIn: 'root' })
export class GenericOverlayService {
  private overlayRefs: {[name: string]: OverlayRef[]} = {};
  private outsideClickListeners: {[name: string]: ((event: MouseEvent) => void) | null} = {};
  constructor(private overlay: Overlay) {}
  /**
   * Opens a component in an overlay at a relative position and remains fixed on scroll.
   * @param component The component to display in the overlay.
   * @param data Optional data or handlers to inject into the component.
   * @param event The click event to determine the position.
   */
  open<T>(nameID: string, component: Type<T>, data?: any, event?: MouseEvent, closeOnClickOther?: boolean): void {
    // Multiple overlays with the same nameID are allowed and are stacked
    let overlayRef: OverlayRef;
    if (event && event.target) {
      const target = event.target as HTMLElement;
      // Create a position strategy relative to the clicked element
      const positionStrategy = this.overlay
        .position()
        .flexibleConnectedTo(target)
        .withPositions([
          {
            originX: 'center',
            originY: 'top',
            overlayX: 'center',
            overlayY: 'bottom',
            offsetY: -10, // 10px above the clicked element
          },
        ])
        .withViewportMargin(8) // Add some margin to ensure visibility
        .withPush(false); // Disable repositioning into the viewport
        overlayRef = this.overlay.create({
          positionStrategy,
          hasBackdrop: false, // No backdrop
          scrollStrategy: this.overlay.scrollStrategies.reposition(), // Adjust overlay position on scroll if needed
        });
    } else {
      overlayRef = this.overlay.create({
        hasBackdrop: true,
        panelClass: ['modal', 'is-active'],
        backdropClass: 'modal-background'
      });
    }
    // Attach the component
    const injector = this.createInjector(data);
    const componentPortal = new ComponentPortal(component, null, injector);
    // Attach the component and get its instance
    const componentRef = overlayRef.attach(componentPortal);
    // // Dynamically hook up the `voiceNote` event if it exists
    // if (data?.onVoiceNote && componentRef.instance['voiceNote']) {
    //   componentRef.instance['voiceNote'].subscribe(data.onVoiceNote);
    // }
    // Prevent bubbling from clicks inside the overlay
    overlayRef.overlayElement.addEventListener('click', (event: MouseEvent) => {
      event.stopPropagation();
    });
    if (!this.overlayRefs[nameID]) {
      this.overlayRefs[nameID] = [];
    }
    this.overlayRefs[nameID].push(overlayRef);
    // Close overlay on outside click
    if(closeOnClickOther)
      this.listenForOutsideClick(nameID);
  }
  /**
   * Closes the most recent overlay for the given nameID.
   */
  close(nameID: string ): void {
    const refs = this.overlayRefs[nameID];
    if (refs && refs.length) {
      refs.pop()?.dispose();
      if (!refs.length) {
        this.overlayRefs[nameID] = [];
        this.removeOutsideClickListener(nameID);
      }
    }
  }
  /**
   * Creates an injector with the given data.
   * @param data The data or handlers to inject.
   */
  private createInjector(data: any): Injector {
    return Injector.create({
      providers: [{ provide: 'overlayData', useValue: data }],
    });
  }
  /**
   * Adds a global click listener to detect outside clicks.
   */
  private listenForOutsideClick(nameID: string): void {
    const listener = ($event: MouseEvent) => {
      this.onDocumentClick($event, nameID);
    };
    this.outsideClickListeners[nameID] = listener;
    // Defer so the click that opened the overlay doesn't immediately close it
    setTimeout(() => {
      // Only attach if the overlay is still open (not closed before the timeout fired)
      if (this.outsideClickListeners[nameID] === listener) {
        document.addEventListener('click', listener);
      }
    });
  }
  /**
   * Removes the outside click listener for the given nameID.
   */
  private removeOutsideClickListener(nameID: string): void {
    const listener = this.outsideClickListeners[nameID];
    if (listener) {
      document.removeEventListener('click', listener);
      this.outsideClickListeners[nameID] = null;
    }
  }
  /**
   * Handles document clicks to close the overlay if clicked outside.
   */
  private onDocumentClick = (event: MouseEvent, nameID: string): void => {
    const refs = this.overlayRefs[nameID] || [];
    if (
      refs.length &&
      !refs.some(ref => ref.overlayElement && ref.overlayElement.contains(event.target as Node))
    ) {
      this.close(nameID);
    }
  };
}