import { Directive, ElementRef, Input, OnDestroy, OnInit } from "@angular/core";
import { SimpleChanges } from "@angular/core";
import { Subscription } from "rxjs";
import { LocalSettingsService } from "../services/local-settings.service";

/**
 * Defers setting the img src until the element is near the viewport.
 * Unlike the native loading="lazy" attribute this works reliably inside
 * nested scroll containers (e.g. .app-body) and the pre-load margin is
 * controllable via [lazySrcMargin].
 * Usage: <img [lazySrc]="thumbPath" [lazySrcMargin]="'300px'" />
 */
@Directive({
    selector: "[lazySrc]",
    standalone: true,
    // Replicates the NgOptimizedImage "fill" behavior the thumbs were using
    host: {
        "width": "100%",
        "height": "100%",
        "object-fit": "cover",
        "(load)": "onLoaded()",
        "(error)": "onLoaded()"
    }
})
export class LazySrcDirective implements OnInit, OnDestroy {
    @Input() lazySrc = "";
    @Input() lazySrcMargin = "500px";

    private observer: IntersectionObserver | null = null;
    private loaded = false;
    private subscription = new Subscription();
    loading = false;

    constructor(
        private el: ElementRef<HTMLImageElement>,
        private localSettingsService: LocalSettingsService
    ) { }

    ngOnInit(): void {
        if (!this.lazySrc) {
            return;
        }
        this.loading = true;
        this.el.nativeElement.classList.add("lazy-src-loading");
        // Honor the "disable lazy load" local setting
        this.subscription.add(
            this.localSettingsService.disableBrowserLazyLoad$.subscribe(disabled => {
                if (disabled) {
                    this.load();
                }
            })
        );
        if (this.loaded) {
            return;
        }
        if (typeof IntersectionObserver === "undefined") {
            this.load();
            return;
        }
        this.observer = new IntersectionObserver(entries => {
            for (const entry of entries) {
                if (entry.isIntersecting) {
                    this.load();
                    break;
                }
            }
        }, { root: null, rootMargin: this.lazySrcMargin, threshold: 0 });
        this.observer.observe(this.el.nativeElement);
    }

    ngOnChanges(changes: SimpleChanges): void {
        // The thumb path can change (e.g. cache-bust on thumb update) - reload if already loaded
        if (changes['lazySrc'] && this.loaded) {
            this.loading = true;
            this.el.nativeElement.classList.add("lazy-src-loading");
            this.load();
        }
    }

    ngOnDestroy(): void {
        this.subscription.unsubscribe();
        this.observer?.disconnect();
        this.observer = null;
    }

    onLoaded(): void {
        this.loading = false;
        this.el.nativeElement.classList.remove("lazy-src-loading");
    }

    private load(): void {
        this.loaded = true;
        this.observer?.disconnect();
        this.observer = null;
        if (this.lazySrc) {
            this.el.nativeElement.src = this.lazySrc;
        }
    }
}