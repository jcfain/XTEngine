import { Component, Input, HostBinding, ChangeDetectionStrategy, forwardRef, ElementRef, AfterViewInit } from '@angular/core';
import { FormsModule, NG_VALUE_ACCESSOR, ControlValueAccessor } from '@angular/forms';
import { AppStateService } from '../../services/app-state.service';
import { GenericOverlayService } from '../../services/overlay.service';
import { SelectDialogComponent, SelectDialogOption } from '../select-dialog/select-dialog.component';

let selectHsInstanceCount = 0;

/**
 * Drop-in replacement for a native <select> on browsers where the native
 * select dropdown is broken (HereSphere). On HereSphere it renders a button
 * that opens a select dialog overlay; everywhere else it renders a native
 * <select>. Its <option> children are projected straight into the native
 * select, so the template looks exactly like a native select.
 *
 * It implements ControlValueAccessor, so it supports the same template API as
 * a native select: [(ngModel)], [ngModel] + (ngModelChange), [disabled], id
 * and title. If HereSphere ever fixes the native select, every <select-hs> in
 * the app can be swapped for a plain <select> by renaming the tag.
 */
@Component({
    selector: 'select-hs',
    imports: [FormsModule],
    templateUrl: './select-hs.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './select-hs.component.css',
    providers: [
        {
            provide: NG_VALUE_ACCESSOR,
            useExisting: forwardRef(() => SelectHsComponent),
            multi: true
        }
    ]
})
export class SelectHsComponent implements ControlValueAccessor, AfterViewInit {
    /** Element id applied to the visible control so a <label for="..."> can target it. */
    @Input() id = '';
    /** Tooltip shown when hovering the control (mirrors the native select's title attribute). */
    @Input() title = '';
    /** Disables the control (mirrors the native select's disabled attribute). */
    @Input() disabled = false;

    @HostBinding('attr.title')
    get hostTitle(): string {
        return this.title;
    }

    userAgentIsHereSphere = false;
    value: any = null;

    private options: SelectDialogOption[] = [];
    private optionsSignature = '';
    private fallbackNameID = 'selectHs' + (selectHsInstanceCount++);
    private onChange: (value: any) => void = () => {};
    private onTouched: () => void = () => {};

    constructor(
        private el: ElementRef<HTMLElement>,
        private appState: AppStateService,
        private overlayService: GenericOverlayService
    ) {
        this.userAgentIsHereSphere = this.appState.userAgentIsHereSphere;
    }

    get selectedLabel(): string {
        const option = this.options.find(o => o.value === String(this.value));
        return option ? option.label : '';
    }

    // ControlValueAccessor

    writeValue(value: any): void {
        this.value = value;
    }

    registerOnChange(fn: (value: any) => void): void {
        this.onChange = fn;
    }

    registerOnTouched(fn: () => void): void {
        this.onTouched = fn;
    }

    setDisabledState(isDisabled: boolean): void {
        this.disabled = isDisabled;
    }

    ngAfterViewInit(): void {
        this.refreshOptions();
    }

    ngAfterViewChecked(): void {
        // The projected <option> elements can appear or update after the first
        // render (e.g. voices and profiles load asynchronously), so re-read
        // them on each change detection pass while in HereSphere mode.
        if (this.userAgentIsHereSphere) {
            this.refreshOptions();
        }
    }

    // Change handlers

    onNativeChange(value: any): void {
        this.value = value;
        this.onTouched();
        this.onChange(value);
    }

    openSelectDialog(): void {
        const nameID = this.overlayNameID();
        this.overlayService.open(nameID, SelectDialogComponent, {
            options: this.options,
            // DOM option values are strings, so compare against the string
            // form of the current value. The consuming setters already convert
            // numeric strings back to numbers (same as the native select,
            // which always emits strings).
            selectedValue: this.value === null || this.value === undefined ? undefined : String(this.value),
            onAccept: (value: any) => {
                this.value = value;
                this.onTouched();
                this.onChange(value);
            },
            onClose: () => this.overlayService.close(nameID)
        }, true);
    }

    /**
     * Reads the projected <option> elements out of the DOM. The native select
     * is always rendered (hidden on HereSphere) so the options are available
     * to the button/dialog branch.
     */
    private refreshOptions(): void {
        const optionEls = this.el.nativeElement.querySelectorAll('option');
        const signature = Array.from(optionEls)
            .map(o => o.value + '\u0000' + (o.textContent ?? ''))
            .join('\u0001');
        if (signature === this.optionsSignature) {
            return;
        }
        this.optionsSignature = signature;
        this.options = Array.from(optionEls).map(o => ({ value: o.value, label: o.textContent ?? '' }));
    }

    /**
     * The overlay service tracks overlays by name. Use the element id when
     * available, otherwise a generated unique id so multiple instances (e.g.
     * one per channel row) never collide.
     */
    private overlayNameID(): string {
        return this.id || this.fallbackNameID;
    }
}