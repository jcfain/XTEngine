// 3D panels for the WebXR media player: media wall, script status, motion
// profiles, TCode commands and the main player control panel.
//
// Part of the self-contained xr-player feature. Panels are data-driven:
// they receive plain data and fire callbacks, so all app logic stays in
// xr-player.service.ts (which is the only file that touches app services).
import * as THREE from 'three';
import { Panel, Button3D, Slider3D, makeLabel, makeFaceTexture } from './xr-ui';
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
    private static readonly MAX_ITEMS = 48;
    private static readonly COLS = 8;
    private tiles: THREE.Group[] = [];
    private playingId: string | null = null;
    private textureLoader = new THREE.TextureLoader();

    constructor(private onItemSelect: (item: MediaItem) => void) {
        super('media', 'Media List', 2.5, 2.35);
    }

    setItems(items: MediaItem[]): void {
        // Clear old tiles
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

        const shown = items.slice(0, MediaWallPanel.MAX_ITEMS);
        const tileW = 0.29;
        const gapX = 0.015;
        const gapY = 0.035;
        const thumbH = tileW * 9 / 16;
        const rowH = thumbH + 0.05 + gapY;
        const gridW = MediaWallPanel.COLS * tileW + (MediaWallPanel.COLS - 1) * gapX;
        const gridH = shown.length / MediaWallPanel.COLS * rowH;
        const startX = -gridW / 2 + tileW / 2;
        const startY = gridH / 2 - thumbH / 2 - 0.05;

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

            // Name label
            const label = makeLabel(item.name, tileW, 0.045, '#cdd6ea', 0.45);
            label.position.set(0, -thumbH / 2 - 0.028, 0.002);
            label.userData['tile'] = tile;
            tile.add(label);

            tile.userData['tileData'] = { item, highlight };
            tile.userData['onClick'] = () => this.onItemSelect(item);
            this.content.add(tile);
            this.tiles.push(tile);
        });

        this.setPlayingId(this.playingId);
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
    private bars: { channel: ChannelModel; fill: THREE.Mesh }[] = [];

    constructor() {
        super('script', 'Script Status', 1.7, 1.5);
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

        const rowH = 0.11;
        const startY = (channels.length - 1) * rowH / 2;
        channels.forEach((channel, i) => {
            const y = startY - i * rowH;
            const label = makeLabel(
                channel.friendlyName || channel.channel, 0.45, 0.06, '#cdd6ea', 0.5);
            label.position.set(-0.55, y, 0);
            this.content.add(label);

            const track = new THREE.Mesh(
                new THREE.PlaneGeometry(1.1, 0.045),
                new THREE.MeshBasicMaterial({ color: 0x33405e })
            );
            track.renderOrder = 10;
            track.position.set(0.1, y, 0);
            this.content.add(track);

            const fill = new THREE.Mesh(
                new THREE.PlaneGeometry(1.1, 0.045),
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

export interface MainControlCallbacks {
    onPlayPause: () => void;
    onPrev: () => void;
    onNext: () => void;
    onMoneyShot: () => void;
    onNextAction: () => void;
    onDevicePause: () => void;
    onSeek: (fraction: number) => void;
    onVolume: (value: number) => void;
    onMute: () => void;
    onRate: () => void;
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
    private playButton!: Button3D;
    private muteButton: Button3D;
    private pinButton: Button3D;
    private devicePauseButton!: Button3D;
    private panelButtons: Record<string, Button3D> = {};
    readonly seekSlider: Slider3D;
    readonly volumeSlider: Slider3D;

    constructor(cb: MainControlCallbacks) {
        super('controls', 'Player Controls', 2.3, 1.0);

        // Row 1: media name + pin/exit
        this.mediaName = makeLabel('', 1.5, 0.08, '#ffffff', 0.5);
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
            ['tcode', 'TCode']
        ];
        toggles.forEach(([id, label], i) => {
            const btn = new Button3D(label, 0.34, 0.09);
            btn.position.set(-0.72 + i * 0.4, 0.21, 0);
            btn.onClick = () => cb.onTogglePanel(id);
            this.content.add(btn);
            this.panelButtons[id] = btn;
        });

        // Row 3: seek slider + time
        this.seekSlider = new Slider3D(1.7, cb.onSeek);
        this.seekSlider.position.set(-0.15, 0.04, 0);
        this.content.add(this.seekSlider);

        this.timeLabel = makeLabel('00:00 / 00:00', 0.5, 0.06, '#9fb0d0', 0.45);
        this.timeLabel.position.set(0.85, 0.04, 0);
        this.content.add(this.timeLabel);

        // Row 4: transport
        const transport: [string, () => void][] = [
            ['Prev', cb.onPrev],
            ['Play', cb.onPlayPause],
            ['Next', cb.onNext],
            ['Money Shot', cb.onMoneyShot],
            ['Next Action', cb.onNextAction],
            ['Dev Pause', cb.onDevicePause]
        ];
        transport.forEach(([label, action], i) => {
            const btn = new Button3D(label, 0.3, 0.09);
            btn.position.set(-0.84 + i * 0.33, -0.14, 0);
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
    }

    setMediaName(name: string): void {
        this.setLabelMesh(this.mediaName, name || 'No media playing', 1.5, 0.08, '#ffffff', 0.5);
    }

    setTimeText(elapsed: string, duration: string): void {
        this.setLabelMesh(this.timeLabel, `${elapsed} / ${duration}`, 0.5, 0.06, '#9fb0d0', 0.45);
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
        const heightPx = 128;
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