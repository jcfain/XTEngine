import { Component, OnInit, TemplateRef, Type } from "@angular/core";
import { XOverlayRef } from "../../utils/x-overlay-ref";

@Component({
  selector: 'app-modal'
})
export class ModalComponent implements OnInit {
  
  contentType: 'template' | 'string' | 'component' = 'component';
  content: string | TemplateRef<any> | Type<any> = "";
  context: any;

  constructor(private ref: XOverlayRef) {}

  ngOnInit() {
    if (typeof this.content === 'string') {
      this.contentType = 'string';
    } else if (this.content instanceof TemplateRef) {
      this.contentType = 'template';
      this.context = {
        close: this.ref.close.bind(this.ref)
      };
    } else {
      this.contentType = 'component';
    }
  }
  close() {
    this.ref.close(null);
  }
}