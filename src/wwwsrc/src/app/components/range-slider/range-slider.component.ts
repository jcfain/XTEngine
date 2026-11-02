import { Component, Input, Output, EventEmitter, ChangeDetectionStrategy } from '@angular/core';

import { FormsModule } from '@angular/forms';

export interface RangeValue {
    min: number;
    max: number;
}

@Component({
    selector: 'app-range-slider',
    imports: [FormsModule],
    templateUrl: './range-slider.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './range-slider.component.css'
})
export class RangeSliderComponent {
    @Input() min = 0;
    @Input() max = 100;
    @Input() valueMin = 0;
    @Input() valueMax = 100;
    @Input() changeAmount = 100;
    @Output() valueChange = new EventEmitter<RangeValue>();
    @Output() instantChange = new EventEmitter<number>();

    get midValue(): number {
        return Math.round((this.valueMin + this.valueMax) / 2);
    }

    onMinChange(value: number): void {
        let newMin = value;
        let newMax = this.valueMax;
        if (newMax < newMin) {
            newMax = newMin + 1;
            this.valueMax = newMax;
        }
        this.valueMin = newMin;
        this.instantChange.emit(newMin);
        this.valueChange.emit({ min: newMin, max: newMax });
    }

    onMaxChange(value: number): void {
        let newMax = value;
        let newMin = this.valueMin;
        if (newMin > newMax) {
            newMin = newMax - 1;
            this.valueMin = newMin;
        }
        this.valueMax = newMax;
        this.instantChange.emit(newMax);
        this.valueChange.emit({ min: newMin, max: newMax });
    }

    minIncrement(): void {
        const newValue = this.valueMin + this.changeAmount;
        this.valueMin = newValue <= this.valueMax ? newValue : this.valueMax - this.changeAmount;
        this.instantChange.emit(this.valueMin);
        this.valueChange.emit({ min: this.valueMin, max: this.valueMax });
    }

    minDecrement(): void {
        const newValue = this.valueMin - this.changeAmount;
        this.valueMin = newValue >= this.min ? newValue : this.min;
        this.instantChange.emit(this.valueMin);
        this.valueChange.emit({ min: this.valueMin, max: this.valueMax });
    }

    maxIncrement(): void {
        const newValue = this.valueMax + this.changeAmount;
        this.valueMax = newValue <= this.max ? newValue : this.max;
        this.instantChange.emit(this.valueMax);
        this.valueChange.emit({ min: this.valueMin, max: this.valueMax });
    }

    maxDecrement(): void {
        const newValue = this.valueMax - this.changeAmount;
        this.valueMax = newValue >= this.valueMin ? newValue : this.valueMin + this.changeAmount;
        this.instantChange.emit(this.valueMax);
        this.valueChange.emit({ min: this.valueMin, max: this.valueMax });
    }
}