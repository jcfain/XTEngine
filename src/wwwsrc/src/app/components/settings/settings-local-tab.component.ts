import { Component, OnInit, ChangeDetectionStrategy, Input } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { LocalSettingsService } from '../../services/local-settings.service';
import { SpeechSynthesisService } from '../../services/speech-synthesis.service';
import { SelectHsComponent } from '../select-hs/select-hs.component';

@Component({
    selector: 'app-settings-local-tab',
    imports: [FormsModule, SelectHsComponent],
    templateUrl: './settings-local-tab.component.html',
    changeDetection: ChangeDetectionStrategy.Eager,
    styleUrl: './settings-local-tab.component.css'
})
export class SettingsLocalTabComponent implements OnInit {
    @Input()
    isVisible = false;
    
    themes = ['Dark', 'Light', 'Midnight'];
    private _theme = 'dark';
    public get theme(): string {
        return this._theme;
    }
    public set theme(value: string) {
        this.localSettings.setTheme(value);
        this._theme = value.toLowerCase();
    }
    externalStreaming = false;
    disableFullScreenMediaList = false;
    disableHeatmap = false;
    animateGifsOnHover = true;

    // Text to speech
    enableTextToSpeech = true;
    speechNotInbrowser = false;
    systemVoices: { value: number; name: string, label: string; lang: string }[] = [];
    selectedVoiceIndex = 0;
    speechPitch = 1;
    speechRate = 1;
    speechVolume = 0.5;

    constructor(
        private localSettings: LocalSettingsService,
        private speechSynthesis: SpeechSynthesisService
    ) {}

    ngOnInit(): void {
        this.localSettings.externalStreaming$.subscribe(v => this.externalStreaming = v);
        this.localSettings.disableFullScreenMediaList$.subscribe(v => this.disableFullScreenMediaList = v);
        this.localSettings.disableHeatmap$.subscribe(v => this.disableHeatmap = v);
        
        this.localSettings.animateGifsOnHover$.subscribe(v => this.animateGifsOnHover = v);
        this.localSettings.theme$.subscribe(v => this._theme = v);

        this.setupTextToSpeech();
    }

    onExternalStreamingChange(checked: boolean): void {
        this.localSettings.setExternalStreamingDefault(checked);
    }

    onDisableFullScreenMediaListChange(checked: boolean): void {
        this.localSettings.setDisableFullScreenMediaList(checked);
    }

    onDisableHeatmapChange(checked: boolean): void {
        this.localSettings.setDisableHeatmap(checked);
    }

    onAnimateGifsOnHoverChange(checked: boolean): void {
        this.localSettings.setAnimateGifsOnHover(checked);
    }

    // Text to speech methods
    private setupTextToSpeech(): void {
        this.speechNotInbrowser = !this.speechSynthesis.isAvailable;
        if (this.speechNotInbrowser) {
            this.disableTextToSpeech();
            return;
        }
        this.speechSynthesis.voices$.subscribe(v => {
            this.systemVoices = v.sort((a, b) => {
                return a.name.localeCompare(b.name);
            }).map((v, i) => ({ 
                value: i, name: v.name, label: v.name + ' (' + v.lang + ')', lang: v.lang 
            }))
        })

        this.localSettings.selectedVoiceIndex$.subscribe(v => this.selectedVoiceIndex = v);
        this.localSettings.enableTextToSpeech$.subscribe(v => this.enableTextToSpeech = v);
        this.localSettings.speechPitch$.subscribe(v => this.speechPitch = v);
        this.localSettings.speechRate$.subscribe(v => this.speechRate = v);
        this.localSettings.speechVolume$.subscribe(v => this.speechVolume = v);
    }

    onEnableTextToSpeechChange(checked: boolean): void {
        this.localSettings.setEnableTextToSpeech(checked);
        if (checked) {
            this.speechSynthesis.speak('Voice enabled');
        }
    }

    onVoiceSelectChange(value: number): void {
        const valueNum = Number(value);
        this.localSettings.setSelectedVoiceIndex(valueNum);
        this.selectedVoiceIndex = valueNum;
        const voice = this.systemVoices[valueNum];
        if (voice) {
            this.speechSynthesis.speak('Hello I am ' + voice.name);
        }
    }

    onSpeechPitchChange(): void {
        this.localSettings.setSpeechPitch(this.speechPitch);
        this.speechSynthesis.speak('Pitch changed to ' + this.speechPitch);
    }

    onSpeechRateChange(): void {
        this.localSettings.setSpeechRate(this.speechRate);
        this.speechSynthesis.speak('Rate changed to ' + this.speechRate);
    }

    onSpeechVolumeChange(): void {
        this.localSettings.setSpeechVolume(this.speechVolume);
        this.speechSynthesis.speak('Volume changed to ' + this.speechVolume);
    }

    private disableTextToSpeech(message?: string): void {
        this.enableTextToSpeech = false;
        this.localSettings.setEnableTextToSpeech(false);
    }
}