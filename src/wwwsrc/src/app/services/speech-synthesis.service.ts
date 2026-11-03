import { Injectable } from '@angular/core';
import { LocalSettingsService } from './local-settings.service';
import { BehaviorSubject, Observable } from 'rxjs';

@Injectable({
    providedIn: 'root'
})
export class SpeechSynthesisService {

    private voicesSubject = new BehaviorSubject<SpeechSynthesisVoice[]>([]);
    voices$: Observable<SpeechSynthesisVoice[]> = this.voicesSubject.asObservable();

    private enableTextToSpeech = true;
    private selectedVoiceIndex = 0;
    private speechPitch = 1;
    private speechRate = 1;
    private speechVolume = 0.5;
    private speechNotInbrowser = false;

    constructor(
        private localSettings: LocalSettingsService
    ) {
        this.localSettings.enableTextToSpeech$.subscribe(v => this.enableTextToSpeech = v);
        this.localSettings.selectedVoiceIndex$.subscribe(v => this.selectedVoiceIndex = v);
        this.localSettings.speechPitch$.subscribe(v => this.speechPitch = v);
        this.localSettings.speechRate$.subscribe(v => this.speechRate = v);
        this.localSettings.speechVolume$.subscribe(v => this.speechVolume = v);

        this.setupVoices();
    }

    private setupVoices(): void {
        if (typeof window.speechSynthesis === 'undefined') {
            this.speechNotInbrowser = true;
            return;
        }

        window.speechSynthesis.addEventListener("voiceschanged", (event) => { 
            this.loadVoices();
        });
        // if (window.speechSynthesis.onvoiceschanged !== undefined) {
        //     window.speechSynthesis.onvoiceschanged = () => this.loadVoices();
        // } else {
        //     setTimeout(() => this.loadVoices(), 1000);
        // }
    }

    private loadVoices(): void {
        this.voicesSubject.next(window.speechSynthesis.getVoices());
    }

    get isAvailable(): boolean {
        return !this.speechNotInbrowser;
    }

    speak(message: string): void {
        if (!message || !this.enableTextToSpeech || typeof window.speechSynthesis === 'undefined') {
            return;
        }
        if (this.voicesSubject.value.length === 0) {
            this.loadVoices();
        }
        if (this.voicesSubject.value.length > 0) {
            const utterance = new SpeechSynthesisUtterance();
            utterance.voice = this.voicesSubject.value[this.selectedVoiceIndex] || null;
            utterance.volume = this.speechVolume;
            utterance.rate = this.speechRate;
            utterance.pitch = this.speechPitch;
            utterance.text = message;
            window.speechSynthesis.speak(utterance);
        }
    }
}