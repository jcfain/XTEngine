// 3D panels for the WebXR media player: media wall, script status, motion
// profiles, TCode commands and the main player control panel.
//
// Part of the self-contained xr-player feature. Panels are data-driven:
// they receive plain data and fire callbacks, so all app logic stays in
// xr-player.service.ts (which is the only file that touches app services).
import * as THREE from 'three';
import { Panel, Button3D, Slider3D, ComboBox3D, makeLabel, makeFaceTexture, makeTitleMesh } from './xr-ui';
import { MediaItem, ChannelModel } from '../../models/interfaces';
import { ThumbState } from '../../models/enums';
import { getChannelColor } from '../../utils/utils';

// Same thumbnail resolution rules as the web media list.
export function getThumbPath(item: MediaItem): string {
    const thumbFile = '/thumb/';
    const cacheBust = item.thumbVersion ? `?v=${item.thumbVersion}` : '';
    switch (item.thumbState) {
        case ThumbState.Waiting:
            return thumbFile + item.thumbFileLoading;
        case ThumbState.Error:
            return thumbFile + item.thumbFileError;
        case ThumbState.Loading:
            return thumbFile + item.thumbFileLoadingCurrent;
        case ThumbState.Ready:
            return thumbFile + item.relativeThumb + cacheBust;
        case ThumbState.Unknown:
            return thumbFile + item.thumbFileUnknown;
        default:
            return thumbFile + item.relativeThumb + cacheBust;
    }
}

// Grid of media thumbnails. Point + trigger plays the item.
export class MediaWallPanel extends Panel {
    private static readonly MAX_ITEMS = 16;
    private static readonly COLS = 4;
    private tiles: THREE.Group[] = [];
    private items: MediaItem[] = [];
    private page = 0;
    private playingId: string | null = null;
    private textureLoader = new THREE.TextureLoader();
    private pageUpButton: Button3D;
    private pageDownButton: Button3D;
    private pageLabel: THREE.Mesh;

    constructor(private onItemSelect: (item: MediaItem) => void) {
        super('media', 'Media List', 2.5, 2.35);
        this.pageUpButton = new Button3D('▲', 0.16, 0.1);
        this.pageUpButton.position.set(-0.45, -1.02, 0);
        this.pageUpButton.onClick = () => this.changePage(-1);
        this.content.add(this.pageUpButton);
        this.pageDownButton = new Button3D('▼', 0.16, 0.1);
        this.pageDownButton.position.set(0.45, -1.02, 0);
        this.pageDownButton.onClick = () => this.changePage(1);
        this.content.add(this.pageDownButton);
        this.pageLabel = makeLabel('1 / 1', 0.4, 0.07, '#9fb0d0', 0.65);
        this.pageLabel.position.set(0, -1.02, 0);
        this.content.add(this.pageLabel);
    }

    private pageCount(): number {
        return Math.max(1, Math.ceil(this.items.length / MediaWallPanel.MAX_ITEMS));
    }

    private changePage(delta: number): void {
        this.page = THREE.MathUtils.clamp(this.page + delta, 0, this.pageCount() - 1);
        this.renderPage();
    }

    setItems(items: MediaItem[]): void {
        this.items = items;
        this.page = 0;
        this.renderPage();
    }

    private renderPage(): void {
        // Clear old tiles (the page buttons stay)
        for (const tile of this.tiles) {
            this.content.remove(tile);
            tile.traverse(obj => {
                const mesh = obj as THREE.Mesh;
                if (mesh.isMesh) {
                    mesh.geometry.dispose();
                    const mat = mesh.material as THREE.MeshBasicMaterial;
                    mat.map?.dispose?.();
                    mat.dispose();
                }
            });
        }
        this.tiles = [];

        const start = this.page * MediaWallPanel.MAX_ITEMS;
        const shown = this.items.slice(start, start + MediaWallPanel.MAX_ITEMS);
        const tileW = 0.55;
        const gapX = 0.02;
        const gapY = 0.04;
        const thumbH = tileW * 9 / 16;
        const labelH = 0.11;
        const rowH = thumbH + 0.02 + labelH + gapY;
        const gridW = MediaWallPanel.COLS * tileW + (MediaWallPanel.COLS - 1) * gapX;
        const gridH = shown.length / MediaWallPanel.COLS * rowH;
        const startX = -gridW / 2 + tileW / 2;
        const startY = gridH / 2 - thumbH / 2 - 0.02 - labelH / 2;

        shown.forEach((item, i) => {
            const col = i % MediaWallPanel.COLS;
            const row = Math.floor(i / MediaWallPanel.COLS);
            const tile = new THREE.Group();
            tile.position.set(startX + col * (tileW + gapX), startY - row * rowH, 0);

            // Hover highlight frame
            const highlight = new THREE.Mesh(
                new THREE.PlaneGeometry(tileW + 0.02, thumbH + 0.02),
                new THREE.MeshBasicMaterial({ color: 0x4d8dff })
            );
            highlight.renderOrder = 10;
            highlight.visible = false;
            tile.add(highlight);

            // Thumbnail
            const thumb = new THREE.Mesh(
                new THREE.PlaneGeometry(tileW, thumbH),
                new THREE.MeshBasicMaterial({ color: 0x2b3a55 })
            );
            thumb.renderOrder = 11;
            thumb.position.z = 0.002;
            const url = getThumbPath(item);
            if (url) {
                this.textureLoader.load(url, tex => {
                    tex.colorSpace = THREE.SRGBColorSpace;
                    (thumb.material as THREE.MeshBasicMaterial).map = tex;
                    (thumb.material as THREE.MeshBasicMaterial).color.set(0xffffff);
                    (thumb.material as THREE.MeshBasicMaterial).needsUpdate = true;
                });
            }
            thumb.userData['tile'] = tile;
            tile.add(thumb);

            // Name label (fixed font size, wrapped to 2 lines, ellipsized)
            const label = makeTitleMesh(item.name, tileW, labelH, '#cdd6ea', 2);
            label.position.set(0, -thumbH / 2 - 0.02 - labelH / 2, 0.002);
            label.userData['tile'] = tile;
            tile.add(label);

            tile.userData['tileData'] = { item, highlight };
            tile.userData['onClick'] = () => this.onItemSelect(item);
            this.content.add(tile);
            this.tiles.push(tile);
        });

        this.setPageLabel(`${this.page + 1} / ${this.pageCount()}`);
        this.pageUpButton.setEnabled(this.page > 0);
        this.pageDownButton.setEnabled(this.page < this.pageCount() - 1);
        this.setPlayingId(this.playingId);
    }

    private setPageLabel(text: string): void {
        const mat = this.pageLabel.material as THREE.MeshBasicMaterial;
        const old = mat.map;
        mat.map = makeFaceTexture(text, {
            width: Math.floor(128 * (0.4 / 0.07)),
            height: 128,
            color: '#9fb0d0',
            font: 'bold 64px sans-serif'
        });
        old?.dispose();
    }

    setPlayingId(id: string | null): void {
        this.playingId = id;
        for (const tile of this.tiles) {
            const data = tile.userData['tileData'] as { item: MediaItem; highlight: THREE.Mesh };
            (data.highlight.material as THREE.MeshBasicMaterial).color.set(data.item.id === id ? 0xffb347 : 0x4d8dff);
        }
    }
}

// Live channel bars, driven by the same channelPositionChange stream as the
// web script-status component.
export class ScriptStatusPanel extends Panel {
    private static readonly HEIGHT = 1.5;
    private bars: { channel: ChannelModel; fill: THREE.Mesh }[] = [];

    constructor() {
        super('script', 'Script Status', 1.7, ScriptStatusPanel.HEIGHT);
    }

    setChannels(channels: ChannelModel[]): void {
        for (const bar of this.bars) {
            this.content.remove(bar.fill);
        }
        // Remove old labels too (they are direct children of content)
        for (let i = this.content.children.length - 1; i >= 0; i--) {
            this.content.remove(this.content.children[i]);
        }
        this.bars = [];

        // Fit all rows inside the panel: shrink the row height when there
        // are many channels (the title takes the top ~0.2m).
        const rowH = Math.min(0.11, (ScriptStatusPanel.HEIGHT - 0.2) / Math.max(1, channels.length));
        const barH = Math.min(0.045, rowH * 0.45);
        const startY = (channels.length - 1) * rowH / 2;
        channels.forEach((channel, i) => {
            const y = startY - i * rowH;
            const label = makeLabel(
                channel.friendlyName || channel.channel, 0.45, 0.06, '#cdd6ea', 0.65);
            label.position.set(-0.55, y, 0);
            this.content.add(label);

            const track = new THREE.Mesh(
                new THREE.PlaneGeometry(1.1, barH),
                new THREE.MeshBasicMaterial({ color: 0x33405e })
            );
            track.renderOrder = 10;
            track.position.set(0.1, y, 0);
            this.content.add(track);

            const fill = new THREE.Mesh(
                new THREE.PlaneGeometry(1.1, barH),
                new THREE.MeshBasicMaterial({ color: new THREE.Color(getChannelColor(channel.channel)) })
            );
            fill.renderOrder = 11;
            fill.geometry.translate(0.55, 0, 0); // anchor at left edge
            fill.position.set(-0.45, y, 0.002);
            fill.scale.x = 0.0001;
            this.content.add(fill);

            this.bars.push({ channel, fill });
        });
    }

    updateChannel(channel: string, position: number): void {
        for (const bar of this.bars) {
            if (bar.channel.channel !== channel) continue;
            const range = bar.channel.max - bar.channel.min;
            const v = range > 0 ? THREE.MathUtils.clamp((position - bar.channel.min) / range, 0, 1) : 0;
            bar.fill.scale.x = Math.max(v, 0.0001);
        }
    }
}

// Motion profile toggle buttons (same action as the web Motion tab).
export class MotionProfilesPanel extends Panel {
    private buttons: Button3D[] = [];

    constructor(private onSelect: (profile: string) => void) {
        super('profiles', 'Motion Profiles', 1.3, 1.4);
    }

    setProfiles(profiles: string[], selected: string): void {
        for (const btn of this.buttons) {
            this.content.remove(btn);
        }
        this.buttons = [];

        const rowH = 0.13;
        const startY = (profiles.length - 1) * rowH / 2;
        profiles.forEach((profile, i) => {
            const btn = new Button3D(profile, 1.1, 0.1);
            btn.position.set(0, startY - i * rowH, 0);
            btn.setActive(profile === selected);
            btn.onClick = () => this.onSelect(profile);
            this.content.add(btn);
            this.buttons.push(btn);
        });
    }

    // Optimistically mark a profile button as active right after the selection
    // is sent, so the UI reflects the choice immediately (mirrors the web tab).
    setSelected(profile: string): void {
        for (const btn of this.buttons) {
            btn.setActive(btn.label === profile);
        }
    }
}

// TCode command buttons (same action as the web TCode dialog).
export class TCodePanel extends Panel {
    private buttons: Button3D[] = [];

    constructor(private onCommand: (command: string) => void) {
        super('tcode', 'TCode Commands', 1.5, 1.5);
    }

    setCommands(commands: { name: string; command: string; hidden: boolean }[]): void {
        for (const btn of this.buttons) {
            this.content.remove(btn);
        }
        this.buttons = [];

        const visible = commands.filter(c => !c.hidden);
        const rowH = 0.13;
        const startY = (visible.length - 1) * rowH / 2;
        visible.forEach((cmd, i) => {
            const btn = new Button3D(cmd.name, 1.3, 0.1);
            btn.position.set(0, startY - i * rowH, 0);
            btn.onClick = () => this.onCommand(cmd.command);
            this.content.add(btn);
            this.buttons.push(btn);
        });
    }
}

// Help panel: the controller instructions. Shown on session start, closed
// with the Close button, re-opened from the control panel's Help toggle.
export class HelpPanel extends Panel {
    constructor(onClose: () => void) {
        super('help', 'Help', 1.9, 1.5);

        const lines = [
            'Trigger: select & play',
            'Squeeze: grab & drag panels',
            'Squeeze skybox: tilt the video',
            'Button 3: re-center the world',
            'Button 4: controls + media list',
            'Settings: passthrough & 3D depth'
        ];
        const rowH = 0.15;
        const startY = (lines.length - 1) * rowH / 2 + 0.1;
        lines.forEach((line, i) => {
            const label = makeLabel(line, 1.7, 0.13, '#cdd6ea', 0.65, 256);
            label.position.set(0, startY - i * rowH, 0);
            this.content.add(label);
        });

        const closeButton = new Button3D('Close', 0.5, 0.14);
        closeButton.position.set(0, -0.55, 0);
        closeButton.onClick = onClose;
        this.content.add(closeButton);
    }
}

export interface MainControlCallbacks {
    onPlayPause: () => void;
    onStop: () => void;
    onPrev: () => void;
    onNext: () => void;
    onMoneyShot: () => void;
    onNextAction: () => void;
    onDevicePause: () => void;
    onSeek: (fraction: number) => void;
    onVolume: (value: number) => void;
    onMute: () => void;
    onRate: () => void;
    onSetProjection: (value: string) => void;
    onSetStereo: (value: string) => void;
    onTogglePanel: (panelId: string) => void;
    onPin: () => void;
    onExit: () => void;
}

// The main player control panel - the 3D equivalent of the web player's
// bottom control bar. It is the hub: the toggle row launches the other
// floating panels.
export class MainControlPanel extends Panel {
    private mediaName: THREE.Mesh;
    private timeLabel: THREE.Mesh;
    private rateButton: Button3D;
    private formatBox: ComboBox3D;
    private stereoBox: ComboBox3D;
    private playButton!: Button3D;
    private muteButton: Button3D;
    private pinButton: Button3D;
    private devicePauseButton!: Button3D;
    private panelButtons: Record<string, Button3D> = {};
    readonly seekSlider: Slider3D;
    readonly volumeSlider: Slider3D;

    constructor(cb: MainControlCallbacks) {
        super('controls', 'Player Controls', 2.3, 1.1);

        // Row 1: media name + pin/exit
        this.mediaName = makeLabel('', 1.5, 0.08, '#ffffff', 0.65, 192);
        this.mediaName.position.set(-0.3, 0.36, 0);
        this.content.add(this.mediaName);

        this.pinButton = new Button3D('Pin', 0.16, 0.08);
        this.pinButton.position.set(0.72, 0.36, 0);
        this.pinButton.onClick = cb.onPin;
        this.content.add(this.pinButton);

        const exitButton = new Button3D('Exit', 0.16, 0.08);
        exitButton.position.set(0.92, 0.36, 0);
        exitButton.onClick = cb.onExit;
        this.content.add(exitButton);

        // Row 2: panel toggles
        const toggles: [string, string][] = [
            ['media', 'Media'],
            ['script', 'Script'],
            ['profiles', 'Profiles'],
            ['tcode', 'TCode'],
            ['settings', 'Settings'],
            ['help', 'Help']
        ];
        toggles.forEach(([id, label], i) => {
            const btn = new Button3D(label, 0.32, 0.09);
            btn.position.set(-0.9 + i * 0.36, 0.21, 0);
            btn.onClick = () => cb.onTogglePanel(id);
            this.content.add(btn);
            this.panelButtons[id] = btn;
        });

        // Row 3: seek slider + time
        this.seekSlider = new Slider3D(1.5, cb.onSeek);
        this.seekSlider.position.set(-0.25, 0.04, 0);
        this.content.add(this.seekSlider);

        this.timeLabel = makeLabel('00:00 / 00:00', 0.6, 0.09, '#9fb0d0', 0.7, 192);
        this.timeLabel.position.set(0.85, 0.04, 0);
        this.content.add(this.timeLabel);

        // Row 4: transport
        const transport: [string, () => void][] = [
            ['Prev', cb.onPrev],
            ['Play', cb.onPlayPause],
            ['Stop', cb.onStop],
            ['Next', cb.onNext],
            ['Money Shot', cb.onMoneyShot],
            ['Next Action', cb.onNextAction],
            ['Dev Pause', cb.onDevicePause]
        ];
        transport.forEach(([label, action], i) => {
            const btn = new Button3D(label, 0.28, 0.09);
            btn.position.set(-0.945 + i * 0.29, -0.14, 0);
            btn.onClick = action;
            this.content.add(btn);
            if (label === 'Play') this.playButton = btn;
            if (label === 'Dev Pause') this.devicePauseButton = btn;
        });

        // Row 5: volume + mute + rate
        this.volumeSlider = new Slider3D(0.6, cb.onVolume);
        this.volumeSlider.position.set(-0.62, -0.32, 0);
        this.content.add(this.volumeSlider);

        this.muteButton = new Button3D('Mute', 0.18, 0.08);
        this.muteButton.position.set(-0.18, -0.32, 0);
        this.muteButton.onClick = cb.onMute;
        this.content.add(this.muteButton);

        this.rateButton = new Button3D('Rate: 1x', 0.3, 0.08);
        this.rateButton.position.set(0.14, -0.32, 0);
        this.rateButton.onClick = cb.onRate;
        this.content.add(this.rateButton);

        // Row 6: manual format override (for mis-named files). Each combo box
        // lets the user pick the projection / stereo layout directly; "Auto"
        // returns to detection from the file name. The list grows upward into
        // the panel body, so the boxes sit on the bottom row.
        this.formatBox = new ComboBox3D(0.6, 0.08, cb.onSetProjection);
        this.formatBox.position.set(-0.55, -0.46, 0.05);
        this.formatBox.setOptions([
            { value: 'auto', label: 'Auto' },
            { value: 'flat', label: '2D' },
            { value: 'equirect180', label: 'Equirect 180' },
            { value: 'equirect360', label: 'Equirect 360' },
            { value: 'fisheye180', label: 'Fisheye 180' },
            { value: 'fisheye190', label: 'Fisheye 190' },
            { value: 'fisheye200', label: 'Fisheye 200' },
            { value: 'fisheye220', label: 'Fisheye 220' }
        ]);
        this.content.add(this.formatBox);

        this.stereoBox = new ComboBox3D(0.6, 0.08, cb.onSetStereo);
        this.stereoBox.position.set(0.35, -0.46, 0.05);
        this.stereoBox.setOptions([
            { value: 'auto', label: 'Auto' },
            { value: 'mono', label: 'Mono' },
            { value: 'sbs', label: 'SBS' },
            { value: 'tb', label: 'TB' }
        ]);
        this.content.add(this.stereoBox);
    }

    setMediaName(name: string): void {
        this.setLabelMesh(this.mediaName, name || 'No media playing', 1.5, 0.08, '#ffffff', 0.65);
    }

    setTimeText(elapsed: string, duration: string): void {
        this.setLabelMesh(this.timeLabel, `${elapsed} / ${duration}`, 0.6, 0.09, '#9fb0d0', 0.7);
    }

    setPlaying(playing: boolean): void {
        this.playButton.setLabel(playing ? 'Pause' : 'Play');
    }

    setProgress(fraction: number): void {
        this.seekSlider.setValue(fraction, false);
    }

    setVolume(v: number): void {
        this.volumeSlider.setValue(v, false);
    }

    setMuted(muted: boolean): void {
        this.muteButton.setActive(muted);
    }

    setRate(rate: number): void {
        this.rateButton.setLabel(`Rate: ${rate}x`);
    }

    // Update the manual format override combo boxes (projection + stereo).
    // When "Auto" is selected, the resolved value for the current clip is
    // appended (e.g. "Auto (SBS)") so the user can see what was detected.
    setFormat(formatValue: string, stereoValue: string,
        resolvedFormatLabel?: string, resolvedStereoLabel?: string): void {
        this.formatBox.setValue(formatValue);
        this.stereoBox.setValue(stereoValue);
        this.formatBox.setOptionLabel('auto',
            formatValue === 'auto' && resolvedFormatLabel ? `Auto (${resolvedFormatLabel})` : 'Auto');
        this.stereoBox.setOptionLabel('auto',
            stereoValue === 'auto' && resolvedStereoLabel ? `Auto (${resolvedStereoLabel})` : 'Auto');
    }

    setDevicePaused(paused: boolean): void {
        this.devicePauseButton.setActive(paused);
    }

    setPanelActive(panelId: string, active: boolean): void {
        const btn = this.panelButtons[panelId];
        if (btn) btn.setActive(active);
    }

    setPinned(pinned: boolean): void {
        this.pinButton.setActive(pinned);
    }

    private setLabelMesh(mesh: THREE.Mesh, text: string,
        w: number, h: number, color: string, fontScale: number): void {
        const mat = mesh.material as THREE.MeshBasicMaterial;
        const aspect = w / h;
        const heightPx = 192;
        const old = mat.map;
        mat.map = makeFaceTexture(text, {
            width: Math.max(256, Math.floor(heightPx * aspect)),
            height: heightPx,
            color,
            font: `bold ${Math.floor(heightPx * fontScale)}px sans-serif`
        });
        old?.dispose();
    }
}

// Tabbed settings panel. Each tab is a group of controls; only the active
// tab's content is visible. New settings are added as new tabs via addTab().
export interface SettingsCallbacks {
    onTogglePassthrough: () => void;
    onStereoStrength: (value: number) => void;
    onResetStereo: () => void;
    onClearSettings: () => void;
    onSkyboxPitch: (value: number) => void;
    onSkyboxYaw: (value: number) => void;
    onSkyboxRoll: (value: number) => void;
    onResetSkyboxPitch: () => void;
    onResetSkyboxYaw: () => void;
    onResetSkyboxRoll: () => void;
    onZoom: (value: number) => void;
    onHeight: (value: number) => void;
    onSide: (value: number) => void;
    onResetPosition: () => void;
}

export class SettingsPanel extends Panel {
    private tabs: { id: string; label: string; content: THREE.Group; button: Button3D }[] = [];
    private passthroughButton: Button3D;
    readonly stereoSlider: Slider3D;
    private skyboxSliders: { name: string; label: THREE.Mesh; slider: Slider3D; last: number }[] = [];

    constructor(cb: SettingsCallbacks) {
        super('settings', 'Settings', 2.0, 1.5);

        // --- Display tab ---
        const display = new THREE.Group();
        this.passthroughButton = new Button3D('Passthrough', 0.6, 0.14);
        this.passthroughButton.position.set(0, 0.35, 0);
        this.passthroughButton.onClick = cb.onTogglePassthrough;
        display.add(this.passthroughButton);

        const stereoLabel = makeLabel('3D Depth', 0.8, 0.09, '#9fb0d0', 0.65);
        stereoLabel.position.set(-0.35, 0.05, 0);
        display.add(stereoLabel);

        // Range 0..2: 1.0 (center) is full 3D; below reduces it, above
        // adds parallax beyond full 3D.
        this.stereoSlider = new Slider3D(1.3, cb.onStereoStrength, 0, 2);
        this.stereoSlider.position.set(-0.15, -0.15, 0);
        this.stereoSlider.setValue(1.0, false);
        display.add(this.stereoSlider);

        // Reset the 3D depth back to full (1.0).
        const stereoReset = new Button3D('R', 0.18, 0.12);
        stereoReset.position.set(0.78, -0.15, 0);
        stereoReset.onClick = cb.onResetStereo;
        display.add(stereoReset);

        // "Clear XR settings": resets every XR setting to its default (with a
        // confirmation dialog). Only XR settings are affected.
        const clearButton = new Button3D('Clear XR settings', 1.2, 0.14);
        clearButton.position.set(0, -0.45, 0);
        clearButton.onClick = cb.onClearSettings;
        display.add(clearButton);

        this.addTab('display', 'Display', display);

        // --- Skybox tab ---
        // One row per axis: a live "Name  N°" label, a -180..180 slider, and a
        // reset button (R) that snaps the axis back to 0.
        const skybox = new THREE.Group();
        const axes: { name: string; onSet: (v: number) => void; onReset: () => void }[] = [
            { name: 'Pitch', onSet: cb.onSkyboxPitch, onReset: cb.onResetSkyboxPitch },
            { name: 'Yaw', onSet: cb.onSkyboxYaw, onReset: cb.onResetSkyboxYaw },
            { name: 'Roll', onSet: cb.onSkyboxRoll, onReset: cb.onResetSkyboxRoll }
        ];
        axes.forEach((axis, i) => {
            const y = 0.22 - i * 0.24;
            const label = makeLabel(`${axis.name} 0°`, 0.6, 0.09, '#9fb0d0', 0.65);
            label.position.set(-0.72, y, 0);
            skybox.add(label);
            const slider = new Slider3D(0.95, v => axis.onSet(v), -180, 180);
            slider.position.set(0.12, y, 0);
            slider.setValue(0, false);
            skybox.add(slider);
            const reset = new Button3D('R', 0.18, 0.12);
            reset.position.set(0.82, y, 0);
            reset.onClick = axis.onReset;
            skybox.add(reset);
            this.skyboxSliders.push({ name: axis.name, label, slider, last: 0 });
        });
        this.addTab('skybox', 'Skybox', skybox);

        // --- Position tab ---
        // Skybox position offsets (in the camera's local frame):
        //   Zoom:   forward/backward along the view direction
        //   Height: up/down (drag right = up)
        //   Side:   left/right (drag right = right)
        // Range -2..2 meters.
        const position = new THREE.Group();
        const posAxes: { name: string; onSet: (v: number) => void }[] = [
            { name: 'Zoom', onSet: cb.onZoom },
            { name: 'Height', onSet: cb.onHeight },
            { name: 'Side', onSet: cb.onSide }
        ];
        posAxes.forEach((axis, i) => {
            const y = 0.22 - i * 0.24;
            const label = makeLabel(`${axis.name} 0.0m`, 0.6, 0.09, '#9fb0d0', 0.65);
            label.position.set(-0.72, y, 0);
            position.add(label);
            const slider = new Slider3D(0.95, v => axis.onSet(v), -2, 2);
            slider.position.set(0.12, y, 0);
            slider.setValue(0, false);
            position.add(slider);
        });
        const posReset = new Button3D('Reset', 0.4, 0.12);
        posReset.position.set(0, -0.55, 0);
        posReset.onClick = cb.onResetPosition;
        position.add(posReset);
        this.addTab('position', 'Position', position);

        this.switchTab(0);
    }

    private addTab(id: string, label: string, content: THREE.Group): void {
        const index = this.tabs.length;
        const button = new Button3D(label, 0.52, 0.12);
        button.onClick = () => this.switchTab(index);
        this.content.add(button);
        content.position.set(0, -0.15, 0);
        this.content.add(content);
        this.tabs.push({ id, label, content, button });
        this.layoutTabs();
    }

    private layoutTabs(): void {
        const n = this.tabs.length;
        const spacing = 0.58;
        const startX = -(n - 1) * spacing / 2;
        this.tabs.forEach((t, i) => {
            t.button.position.set(startX + i * spacing, 0.55, 0);
        });
    }

    private switchTab(index: number): void {
        this.tabs.forEach((t, i) => {
            t.content.visible = (i === index);
            t.button.setActive(i === index);
        });
    }

    setPassthroughActive(active: boolean): void {
        this.passthroughButton.setActive(active);
    }

    // Update the Skybox tab sliders + value labels from the current angles
    // (degrees). Called after a skybox drag / slider move / reset so the UI
    // always reflects the sphere's orientation. The label texture is only
    // rebuilt when the rounded value changes (avoids per-frame canvas work).
    setSkyboxAngles(pitchDeg: number, yawDeg: number, rollDeg: number): void {
        const values = [pitchDeg, yawDeg, rollDeg];
        this.skyboxSliders.forEach((s, i) => {
            s.slider.setValue(values[i], false);
            const v = Math.round(values[i]);
            if (v !== s.last) {
                s.last = v;
                const mat = s.label.material as THREE.MeshBasicMaterial;
                const old = mat.map;
                mat.map = makeFaceTexture(`${s.name} ${v}°`, {
                    width: 256, height: 64, color: '#9fb0d0', font: 'bold 32px sans-serif'
                });
                old?.dispose();
            }
        });
    }
}

// A small modal confirmation dialog: a message plus Cancel / Confirm buttons.
// The panel's own X button (from the Panel base) acts as Cancel.
export class ConfirmDialog extends Panel {
    constructor(message: string, onConfirm: () => void, onCancel: () => void) {
        super('confirm', 'Confirm', 1.7, 1.0);
        this.onClose = onCancel; // X button behaves like Cancel

        const label = makeLabel(message, 1.5, 0.3, '#ffffff', 0.65, 256);
        label.position.set(0, 0.15, 0);
        this.content.add(label);

        const cancel = new Button3D('Cancel', 0.5, 0.14);
        cancel.position.set(-0.35, -0.28, 0);
        cancel.onClick = onCancel;
        this.content.add(cancel);

        const confirm = new Button3D('Confirm', 0.5, 0.14);
        confirm.position.set(0.35, -0.28, 0);
        confirm.onClick = onConfirm;
        this.content.add(confirm);
    }
}