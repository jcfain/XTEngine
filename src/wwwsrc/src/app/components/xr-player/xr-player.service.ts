// WebXR media player service: owns the Three.js scene, the XR session,
// controller interaction, video playback and the 3D panels.
//
// This is the only file in the xr-player feature that talks to app services
// (everything else is plain three.js). Removing the whole feature = delete
// the xr-player folder plus the VR button in app-header and the one line in
// app.component.html.
import { Injectable, OnDestroy } from '@angular/core';
import * as THREE from 'three';
import { Subscription } from 'rxjs';
import { MediaItem } from '../../models/interfaces';
import { MediaListService } from '../../services/media-list-service';
import { VideoService } from '../../services/video.service';
import { WebsocketService } from '../../services/websocket.service';
import { ServerService } from '../../services/server.service';
import { MessageHandlerService } from '../../services/message-handler.service';
import { LocalSettingsService } from '../../services/local-settings.service';
import { AppStateService } from '../../services/app-state.service';
import { BehaviorSubject } from 'rxjs';
import {
    Panel, Button3D, Slider3D, makeFaceTexture
} from './xr-ui';
import {
    MediaWallPanel, ScriptStatusPanel, MotionProfilesPanel, TCodePanel,
    MainControlPanel
} from './xr-panels';

interface InteractiveTarget {
    object: THREE.Object3D;
    button?: Button3D;
    slider?: Slider3D;
    tile?: THREE.Group;
    panel?: Panel;
}

// Typed view of the userData we attach to scene objects (the tsconfig
// forbids property access on index signatures).
interface XrUserData {
    index?: number;
    cursor?: THREE.Mesh;
    // Live XRInputSource: its .gamepad is updated by the runtime every
    // frame. A gamepad snapshot taken in the 'connected' event goes stale
    // (axes stay 0), so always read through the input source.
    inputSource?: any;
    button?: Button3D;
    slider?: Slider3D;
    tile?: THREE.Group;
    tileData?: { item: MediaItem; highlight: THREE.Mesh };
    panel?: Panel;
    onClick?: () => void;
}

@Injectable({
    providedIn: 'root'
})
export class XrPlayerService implements OnDestroy {
    // Component-facing state
    readonly supported$ = new BehaviorSubject<boolean>(false);
    readonly sessionActive$ = new BehaviorSubject<boolean>(false);
    readonly status$ = new BehaviorSubject<string>('');

    private renderer: THREE.WebGLRenderer | null = null;
    private scene = new THREE.Scene();
    // All scene content lives in this group so the whole world can be
    // re-centered under the user (the local-floor origin is frozen at the
    // spot where the session started - walking away makes panels feel far).
    private worldRoot = new THREE.Group();
    private camera = new THREE.PerspectiveCamera(70, 16 / 9, 0.05, 200);
    private raycaster = new THREE.Raycaster();
    private tempMatrix = new THREE.Matrix4();
    private tempVec = new THREE.Vector3();
    private clock = new THREE.Clock();
    private initialized = false;

    private controllers: THREE.Object3D[] = [];
    private hovered: (InteractiveTarget | null)[] = [null, null];
    private draggingSlider: { controller: THREE.Object3D; slider: Slider3D } | null = null;
    private grabbing: { controller: THREE.Object3D; panel: Panel; rayDist: number } | null = null;
    private lastInteraction = 0;
    private pinned = false;
    private onGlobalError: ((e: ErrorEvent) => void) | null = null;
    private onGlobalRejection: ((e: PromiseRejectionEvent) => void) | null = null;

    private panels: Panel[] = [];
    private mediaWall: MediaWallPanel | null = null;
    private scriptPanel: ScriptStatusPanel | null = null;
    private profilesPanel: MotionProfilesPanel | null = null;
    private tcodePanel: TCodePanel | null = null;
    private mainPanel: MainControlPanel | null = null;

    // Video
    private videoEl: HTMLVideoElement | null = null;
    // The Quest's XR render path does not upload VideoTextures (the screen
    // stays black while the element plays fine). Copying frames to a 2D
    // canvas and uploading a CanvasTexture works reliably.
    private videoTexture: THREE.CanvasTexture | null = null;
    private videoCopyCanvas: HTMLCanvasElement | null = null;
    private videoCopyCtx: CanvasRenderingContext2D | null = null;
    private lastVideoCopy = 0;
    private videoScreen: THREE.Mesh | null = null;
    private videoSphere: THREE.Mesh | null = null;
    private videoFrame: THREE.Mesh | null = null;
    private audio: THREE.Audio | null = null;
    private previousVideoNode: HTMLVideoElement | null = null;
    private stateTimer: number | null = null;
    private externalStreaming = false;
    private shuffleMode = false;
    private lastTimeLabelUpdate = 0;
    // DOM overlay (visible inside the headset) and the canvas it must hide.
    private canvas: HTMLCanvasElement | null = null;
    private overlayRoot: HTMLElement | null = null;
    // 3D HUD: an opaque text plane attached to the camera. The Quest
    // composites dom-overlay unreliably, so in-VR diagnostics are rendered
    // in-scene instead (opaque material - no Quest transparency bug).
    private hudCanvas: HTMLCanvasElement | null = null;
    private hudCtx: CanvasRenderingContext2D | null = null;
    private hudTexture: THREE.CanvasTexture | null = null;
    private hudText = '';
    private hudSubText = '';
    private statusText = '';
    private lastHudDraw = 0;

    // Non-VR preview look-around
    private previewYaw = 0;
    private previewPitch = 0;
    private previewDragging = false;
    private lastMouse = { x: 0, y: 0 };

    private subs: Subscription[] = [];

    constructor(
        private mediaListService: MediaListService,
        private videoService: VideoService,
        private websocketService: WebsocketService,
        private serverService: ServerService,
        private messageHandlerService: MessageHandlerService,
        private localSettingsService: LocalSettingsService,
        private appState: AppStateService
    ) {
        // Data subscriptions are established in init() (after the panels
        // exist), not here: the sources are BehaviorSubjects, so a
        // constructor-time subscription fires while mediaWall & co are
        // still null and the rebuilt panels would never receive data.
    }

    // Wire the app services to the 3D panels. Called from init() once the
    // panels exist; the BehaviorSubjects push their current values
    // immediately on subscribe. Unsubscribed in ngOnDestroy().
    private subscribeToData(): void {
        this.subs.push(
            this.mediaListService.mediaListDisplayed$.subscribe(items => {
                this.mediaWall?.setItems(items || []);
            }),
            this.videoService.playingMediaItem$.subscribe(item => {
                this.mediaWall?.setPlayingId(item?.id ?? null);
                this.mainPanel?.setMediaName(item?.name ?? '');
            }),
            this.videoService.isPaused$.subscribe(paused => {
                this.mainPanel?.setPlaying(!paused);
            }),
            this.localSettingsService.volume$.subscribe(v => {
                if (this.videoEl) this.videoEl.volume = v;
                this.audio?.setVolume(v);
                this.mainPanel?.setVolume(v);
            }),
            this.localSettingsService.mute$.subscribe(m => {
                if (this.videoEl) this.videoEl.muted = m;
                this.mainPanel?.setMuted(m);
            }),
            this.localSettingsService.externalStreaming$.subscribe(v => this.externalStreaming = v),
            this.appState.shufflePlayMode$.subscribe(v => this.shuffleMode = v),
            this.serverService.remoteUserSettings$.subscribe(settings => {
                if (!settings) return;
                const channels = settings.availableChannels
                    ? Object.values(settings.availableChannels)
                    : [];
                this.scriptPanel?.setChannels(channels);
                this.profilesPanel?.setProfiles(
                    settings.allChannelProfileNames || [],
                    settings.selectedChannelProfile || '');
            }),
            this.messageHandlerService.channelPositionChange$.subscribe(v => {
                if (v) this.scriptPanel?.updateChannel(v.channel, v.position);
            }),
            this.serverService.tcodeCommands$.subscribe(commands => {
                this.tcodePanel?.setCommands((commands || []) as { name: string; command: string; hidden: boolean }[]);
            })
        );
    }

    init(canvas: HTMLCanvasElement): void {
        if (this.initialized) return;
        this.initialized = true;
        // Start the idle timer "now" so the control panel is visible for the
        // first few seconds instead of auto-hiding immediately.
        this.lastInteraction = performance.now();
        this.canvas = canvas;
        this.overlayRoot = canvas.parentElement;

        // This service is a singleton that outlives the component: on
        // re-init (player reopened) the scene must start clean, otherwise
        // every environment/panel/video mesh gets added a second time.
        this.scene.clear();
        this.worldRoot = new THREE.Group();
        this.scene.add(this.worldRoot);
        this.panels = [];
        this.controllers = [];
        this.hovered = [null, null];
        this.grabbing = null;
        this.draggingSlider = null;
        this.mediaWall = null;
        this.scriptPanel = null;
        this.profilesPanel = null;
        this.tcodePanel = null;
        this.mainPanel = null;
        this.videoScreen = null;
        this.videoSphere = null;
        this.videoFrame = null;
        this.audio = null;
        this.videoTexture = null;

        const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
        renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
        renderer.setSize(window.innerWidth, window.innerHeight);
        renderer.xr.enabled = true;
        this.renderer = renderer;

        this.buildEnvironment();
        this.buildVideo();
        this.buildHud();
        this.buildPanels();
        this.subscribeToData();
        this.subs.push(this.status$.subscribe(t => this.statusText = t));
        this.loadLayout();
        this.setupControllers();
        this.setupPreviewMouse(canvas);

        // Check WebXR support
        if (navigator.xr) {
            navigator.xr.isSessionSupported('immersive-vr').then(supported => {
                this.supported$.next(supported);
                this.status$.next(supported
                    ? 'Ready [v23]. Put on your headset and press Enter VR.'
                    : 'WebXR immersive-vr is not supported on this device/browser. ' +
                      'The page must be served over HTTPS (or localhost).');
            }).catch((e) => {
                this.supported$.next(false);
                this.status$.next('WebXR support check failed: ' + this.errorMessage(e));
            });
        } else {
            this.supported$.next(false);
            this.status$.next('WebXR is not available in this browser.');
        }

        // Surface unexpected errors on the overlay so they are readable on
        // the headset (the DOM overlay stays visible when a session dies).
        this.onGlobalError = (e: ErrorEvent) => {
            console.error('XR player error:', e.error);
            this.status$.next('Error: ' + this.errorMessage(e.error));
        };
        this.onGlobalRejection = (e: PromiseRejectionEvent) => {
            console.error('XR player rejection:', e.reason);
            this.status$.next('Error: ' + this.errorMessage(e.reason));
        };
        window.addEventListener('error', this.onGlobalError);
        window.addEventListener('unhandledrejection', this.onGlobalRejection);

        renderer.setAnimationLoop(this.render);
    }

    private errorMessage(e: unknown): string {
        if (e instanceof Error) return e.message;
        if (typeof e === 'string') return e;
        try { return JSON.stringify(e); } catch { return String(e); }
    }

    private buildEnvironment(): void {
        this.scene.background = new THREE.Color(0x0a0e1a);
        this.scene.fog = new THREE.Fog(0x0a0e1a, 12, 40);

        // Gradient sky dome
        const skyCanvas = document.createElement('canvas');
        skyCanvas.width = 16;
        skyCanvas.height = 256;
        const skyCtx = skyCanvas.getContext('2d')!;
        const grad = skyCtx.createLinearGradient(0, 0, 0, 256);
        grad.addColorStop(0, '#1a2440');
        grad.addColorStop(0.6, '#0d1322');
        grad.addColorStop(1, '#05070d');
        skyCtx.fillStyle = grad;
        skyCtx.fillRect(0, 0, 16, 256);
        const skyTex = new THREE.CanvasTexture(skyCanvas);
        skyTex.colorSpace = THREE.SRGBColorSpace;
        const sky = new THREE.Mesh(
            new THREE.SphereGeometry(60, 32, 16),
            new THREE.MeshBasicMaterial({ map: skyTex, side: THREE.BackSide, fog: false })
        );
        this.worldRoot.add(sky);

        // Floor grid
        const grid = new THREE.PolarGridHelper(12, 16, 10, 64, 0x2a3550, 0x1a2133);
        this.worldRoot.add(grid);

        // Lights
        this.worldRoot.add(new THREE.HemisphereLight(0x8899cc, 0x223344, 1.0));
        const dir = new THREE.DirectionalLight(0xffffff, 0.8);
        dir.position.set(2, 4, 1);
        this.worldRoot.add(dir);

        // Preview camera start (in VR the headset drives the camera)
        this.camera.position.set(0, 1.6, 0);
        this.scene.add(this.camera);
    }

    // Build the camera-attached diagnostic HUD (opaque, always in front).
    private buildHud(): void {
        const canvas = document.createElement('canvas');
        canvas.width = 1280;
        canvas.height = 128;
        const ctx = canvas.getContext('2d')!;
        const texture = new THREE.CanvasTexture(canvas);
        const hud = new THREE.Mesh(
            new THREE.PlaneGeometry(0.8, 0.08),
            new THREE.MeshBasicMaterial({ map: texture })
        );
        hud.position.set(0, -0.35, -1);
        hud.renderOrder = 999;
        this.camera.add(hud);
        this.hudCanvas = canvas;
        this.hudCtx = ctx;
        this.hudTexture = texture;
        this.drawHud();
    }

    private drawHud(): void {
        const ctx = this.hudCtx;
        if (!ctx) return;
        ctx.fillStyle = '#05070d';
        ctx.fillRect(0, 0, 1280, 128);
        ctx.font = '40px sans-serif';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = '#7fd4ff';
        let line1 = this.hudText;
        while (line1.length > 2 && ctx.measureText(line1).width > 1240) {
            line1 = line1.slice(0, -1);
        }
        ctx.fillText(line1, 16, 34);
        if (this.hudSubText) {
            ctx.fillStyle = '#9fd47f';
            let line2 = this.hudSubText;
            while (line2.length > 2 && ctx.measureText(line2).width > 1240) {
                line2 = line2.slice(0, -1);
            }
            ctx.fillText(line2, 16, 94);
        }
        if (this.hudTexture) this.hudTexture.needsUpdate = true;
    }

    private buildVideo(): void {
        // The VR video element. It is registered with VideoService while a
        // video plays so all state (playing item, media state posts, seek,
        // rate) flows through the same service as the web player.
        const video = document.createElement('video');
        video.playsInline = true;
        video.autoplay = true;
        // Keep the element IN the DOM (small but real): the Quest compositor
        // does not produce frames for detached or 1px videos, so the texture
        // copy stays black. 64x36px at full opacity is visually negligible
        // but forces real frame production.
        video.style.cssText =
            'position:fixed;left:0;bottom:0;width:64px;height:36px;' +
            'opacity:1;pointer-events:none;';
        document.body.appendChild(video);
        this.videoEl = video;

        video.addEventListener('timeupdate', () => {
            if (!video.duration) return;
            this.videoService.onPositionChange(video.currentTime);
            const now = performance.now();
            if (now - this.lastTimeLabelUpdate > 500 && this.mainPanel) {
                this.lastTimeLabelUpdate = now;
                this.mainPanel.setProgress(video.currentTime / video.duration);
                this.mainPanel.setTimeText(
                    this.formatTime(video.currentTime), this.formatTime(video.duration));
            }
        });
        video.addEventListener('ended', () => {
            if (this.shuffleMode) {
                this.mediaListService.playNextVideo();
            } else {
                this.stopVideo();
            }
        });

        // Video screen (regular media)
        const screen = new THREE.Mesh(
            new THREE.PlaneGeometry(4.2, 2.36),
            new THREE.MeshBasicMaterial({ color: 0x000000 })
        );
        screen.position.set(0, 1.6, -3.5);
        screen.visible = false;
        this.videoScreen = screen;
        this.worldRoot.add(screen);

        // Frame copy for the video texture (see videoTexture field).
        // FIXED size 960x540 (16:9, matching the screen mesh): resizing a
        // canvas changes the GPU texture size, which on the Quest leaves the
        // texture mapped at the stale size (video appears in a corner).
        const copyCanvas = document.createElement('canvas');
        copyCanvas.width = 960;
        copyCanvas.height = 540;
        this.videoCopyCanvas = copyCanvas;
        this.videoCopyCtx = copyCanvas.getContext('2d');
        const videoTexture = new THREE.CanvasTexture(copyCanvas);
        videoTexture.colorSpace = THREE.SRGBColorSpace;
        this.videoTexture = videoTexture;

        const frame = new THREE.Mesh(
            new THREE.PlaneGeometry(4.35, 2.5),
            new THREE.MeshBasicMaterial({ color: 0x05070d })
        );
        frame.position.set(0, 1.6, -3.51);
        frame.visible = false;
        this.videoFrame = frame;
        this.worldRoot.add(frame);

        // 360 sphere (equirectangular media tagged "360")
        const sphere = new THREE.Mesh(
            new THREE.SphereGeometry(12, 60, 40),
            new THREE.MeshBasicMaterial({ color: 0x000000, side: THREE.BackSide })
        );
        sphere.visible = false;
        this.videoSphere = sphere;
        this.worldRoot.add(sphere);

        // Spatial audio
        const listener = new THREE.AudioListener();
        this.camera.add(listener);
        const audio = new THREE.Audio(listener);
        audio.setMediaElementSource(video);
        this.audio = audio;
    }

    private buildPanels(): void {
        // Main control panel (the hub)
        this.mainPanel = new MainControlPanel({
            onPlayPause: () => this.videoService.togglePause(),
            onPrev: () => this.mediaListService.playPreviousVideo(),
            onNext: () => this.mediaListService.playNextVideo(),
            onMoneyShot: () => this.websocketService.sendSkipToMoneyShot(),
            onNextAction: () => this.websocketService.sendSkipToNextAction(),
            onDevicePause: () => this.websocketService.togglePauseAllDeviceActions(),
            onSeek: (fraction) => this.onSeekFraction(fraction),
            onVolume: (v) => this.videoService.onSetVolume(v),
            onMute: () => this.videoService.toggleMute(),
            onRate: () => this.cyclePlayRate(),
            onTogglePanel: (id) => this.togglePanel(id),
            onPin: () => this.togglePin(),
            onExit: () => this.exitVR()
        });
        this.mainPanel.position.set(0, 1.4, -1.9);
        this.worldRoot.add(this.mainPanel);
        this.panels.push(this.mainPanel);

        this.mediaWall = new MediaWallPanel(item => this.startVideo(item));
        this.mediaWall.position.set(-2.3, 1.6, -2.6);
        this.mediaWall.rotation.y = 0.55;
        this.worldRoot.add(this.mediaWall);
        this.panels.push(this.mediaWall);

        this.scriptPanel = new ScriptStatusPanel();
        this.scriptPanel.position.set(2.1, 1.5, -2.3);
        this.scriptPanel.rotation.y = -0.5;
        this.scriptPanel.visible = false;
        this.worldRoot.add(this.scriptPanel);
        this.panels.push(this.scriptPanel);

        this.profilesPanel = new MotionProfilesPanel(profile =>
            this.websocketService.setSelectedProfile(profile));
        this.profilesPanel.position.set(2.7, 1.5, -1.1);
        this.profilesPanel.rotation.y = -0.95;
        this.profilesPanel.visible = false;
        this.worldRoot.add(this.profilesPanel);
        this.panels.push(this.profilesPanel);

        this.tcodePanel = new TCodePanel(command =>
            this.websocketService.sendTCode(command));
        this.tcodePanel.position.set(-2.9, 1.5, -1.1);
        this.tcodePanel.rotation.y = 0.95;
        this.tcodePanel.visible = false;
        this.worldRoot.add(this.tcodePanel);
        this.panels.push(this.tcodePanel);

        this.syncPanelToggleButtons();
    }

    // ---- Session lifecycle -------------------------------------------------

    async enterVR(): Promise<void> {
        if (!this.renderer || !navigator.xr) return;
        let session: XRSession;
        try {
            session = await navigator.xr.requestSession('immersive-vr', {
                // dom-overlay keeps the status line readable inside the
                // headset (the only way to surface errors in VR).
                optionalFeatures: ['local-floor', 'bounded-floor', 'dom-overlay'],
                domOverlay: { root: this.overlayRoot ?? document.body }
            });
        } catch (e) {
            console.error('Failed to request WebXR session:', e);
            this.status$.next('Failed to start the VR session: ' + this.errorMessage(e) +
                '. The page must be served over HTTPS (or localhost) and the headset must be connected.');
            return;
        }
        try {
            // Use floor-level tracking when the device granted it, otherwise
            // fall back to plain local (head-anchored) tracking.
            const granted = (session as any).enabledFeatures as string[] | undefined;
            this.renderer.xr.setReferenceSpaceType(
                granted?.includes('local-floor') ? 'local-floor' : 'local');
            await this.renderer.xr.setSession(session);
        } catch (e) {
            console.error('Failed to attach WebXR session:', e);
            this.status$.next('Failed to attach the VR session: ' + this.errorMessage(e));
            try { session.end(); } catch { /* ignore */ }
            return;
        }
        this.lastInteraction = performance.now();
        this.sessionActive$.next(true);
        // Visible status on session start: an empty status line is
        // indistinguishable from a missing dom-overlay.
        const feats = (session as any).enabledFeatures as string[] | undefined;
        this.status$.next(
            'In VR. Point + trigger to select. Squeeze a panel to drag (it follows your laser, faces your hand). Squeeze the SKY/FLOOR to re-center.' +
            (feats?.includes('dom-overlay') ? '' : ' [DOM overlay NOT granted]'));
        // The XR view renders to the XR layer, not the DOM canvas - hide the
        // canvas so the dom-overlay doesn't paint a blank rectangle over the
        // VR view.
        if (this.canvas) this.canvas.style.visibility = 'hidden';
        session.addEventListener('end', () => this.onSessionEnd());
    }

    exitVR(): void {
        this.renderer?.xr.getSession()?.end();
    }

    private onSessionEnd(): void {
        this.sessionActive$.next(false);
        if (this.canvas) this.canvas.style.visibility = '';
        const hadVideo = this.videoService.isMediaItemPlaying();
        this.handoffToWebPlayer();
        if (!hadVideo) {
            // Session ended without playback - say so, so an unexpected
            // early exit is visible on the overlay.
            this.status$.next('VR session ended.');
        }
    }

    // When the VR session ends, hand playback back to the normal in-page
    // player so the video keeps playing on the home page.
    private handoffToWebPlayer(): void {
        const item = this.videoService.playingMediaItem;
        const pos = this.videoEl?.currentTime || 0;
        if (this.videoEl) this.videoEl.pause();
        this.stopStateTimer();
        this.hideVideoMeshes();
        if (this.previousVideoNode) {
            this.videoService.setVideoNode(this.previousVideoNode);
        }
        if (item && this.previousVideoNode) {
            // clearVideo() resets the playing item so playVideo() below will
            // actually load the source into the page player.
            this.videoService.clearVideo();
            this.videoService.playVideo(item);
            if (pos > 1) {
                this.videoService.onSeek(pos);
            }
            this.status$.next(`Continuing playback: ${item.name}`);
        }
    }

    // ---- Video -------------------------------------------------------------

    private startVideo(item: MediaItem): void {
        if (this.externalStreaming) {
            this.status$.next('External streaming is enabled - disable it in Settings to play media in VR.');
            return;
        }
        // Re-selecting the playing item toggles pause
        if (this.videoService.playingMediaItem?.id === item.id && this.videoEl) {
            this.videoService.togglePause();
            return;
        }
        if (!this.videoEl) return;

        // Register our video element with VideoService so playVideo() and the
        // media state posts operate on it. Remember the original (page) node
        // once so handoff on exit restores it - starting a second video in
        // VR must not overwrite it with our own element.
        if (this.previousVideoNode === null) {
            this.previousVideoNode = this.videoService.video;
        }
        this.videoService.setVideoNode(this.videoEl);
        this.videoService.playVideo(item);
        // playVideo() calls showVideo(), which makes the PAGE's video
        // element visible - hide it again: the VR screen is the display.
        this.videoService.hideVideo();

        // Explicitly start playback: the autoplay attribute's failure is
        // silent (no promise, no event), so if the browser blocks it the
        // video would just sit there. play() gives us a catchable rejection.
        this.videoEl.play().catch(e => {
            this.status$.next('Video playback was blocked: ' + this.errorMessage(e) +
                '. Pull the trigger on a media item again.');
        });

        const is360 = item.metaData?.tags?.includes('360') === true;
        if (is360) {
            const m = this.videoSphere!.material as THREE.MeshBasicMaterial;
            m.map = this.videoTexture;
            // The material starts black (blank screen) - the color
            // MULTIPLIES the texture, so it must be white once a map is set.
            m.color.set(0xffffff);
            m.needsUpdate = true;
            this.videoSphere!.visible = true;
            this.videoScreen!.visible = false;
            this.videoFrame!.visible = false;
        } else {
            const m = this.videoScreen!.material as THREE.MeshBasicMaterial;
            m.map = this.videoTexture;
            m.color.set(0xffffff);
            m.needsUpdate = true;
            this.videoScreen!.visible = true;
            this.videoFrame!.visible = true;
            this.videoSphere!.visible = false;
        }

        this.startStateTimer();
        this.status$.next(`Playing: ${item.name}`);
    }

    private stopVideo(): void {
        if (!this.videoEl) return;
        this.videoEl.pause();
        this.stopStateTimer();
        this.videoService.clearVideo();
        this.hideVideoMeshes();
        this.status$.next('Stopped.');
    }

    private hideVideoMeshes(): void {
        if (this.videoScreen) this.videoScreen.visible = false;
        if (this.videoFrame) this.videoFrame.visible = false;
        if (this.videoSphere) this.videoSphere.visible = false;
    }

    // Post media state to the backend periodically while playing (the web
    // player does the same on its timeupdate event).
    private startStateTimer(): void {
        this.stopStateTimer();
        this.videoService.sendMediaState();
        this.stateTimer = window.setInterval(() => this.videoService.sendMediaState(), 5000);
    }

    private stopStateTimer(): void {
        if (this.stateTimer) {
            clearInterval(this.stateTimer);
            this.stateTimer = null;
        }
    }

    // ---- Control panel actions ----------------------------------------------

    private onSeekFraction(fraction: number): void {
        const video = this.videoEl;
        if (video && video.duration > 0) {
            this.videoService.onSeek(fraction * video.duration);
        }
    }

    private static readonly RATES = [0.5, 0.75, 1, 1.25, 1.5, 2];

    private cyclePlayRate(): void {
        const current = this.videoEl?.playbackRate ?? 1;
        const idx = XrPlayerService.RATES.findIndex(r => r > current + 0.001);
        const next = idx === -1 ? XrPlayerService.RATES[0] : XrPlayerService.RATES[idx];
        this.videoService.setPlayRate(next);
        this.mainPanel?.setRate(next);
    }

    private togglePin(): void {
        this.pinned = !this.pinned;
        this.mainPanel?.setPinned(this.pinned);
        if (this.pinned && this.mainPanel) {
            this.mainPanel.visible = true;
        }
    }

    private togglePanel(id: string): void {
        const panel = this.panels.find(p => p.panelId === id);
        if (!panel) return;
        panel.visible = !panel.visible;
        this.syncPanelToggleButtons();
        this.saveLayout();
    }

    private syncPanelToggleButtons(): void {
        if (!this.mainPanel) return;
        for (const p of this.panels) {
            if (p.panelId !== 'controls') {
                this.mainPanel.setPanelActive(p.panelId, p.visible);
            }
        }
    }

    private formatTime(secs: number): string {
        if (!secs || isNaN(secs)) return '00:00';
        const h = Math.floor(secs / 3600);
        const m = Math.floor((secs % 3600) / 60);
        const s = Math.floor(secs % 60);
        const mm = m < 10 ? '0' + m : String(m);
        const ss = s < 10 ? '0' + s : String(s);
        return h > 0 ? `${h}:${mm}:${ss}` : `${mm}:${ss}`;
    }

    // ---- Controllers & interaction ------------------------------------------

    private setupControllers(): void {
        if (!this.renderer) return;
        for (let i = 0; i < 2; i++) {
            const controller = this.renderer.xr.getController(i);
            this.ud(controller).index = i;

            // Ray line
            const geo = new THREE.BufferGeometry().setFromPoints([
                new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, -1)
            ]);
            const line = new THREE.Line(geo,
                new THREE.LineBasicMaterial({ color: 0x9fb0d0, transparent: true, opacity: 0.6 }));
            line.scale.z = 5;
            controller.add(line);

            // Cursor dot
            const cursor = new THREE.Mesh(
                new THREE.SphereGeometry(0.012, 12, 8),
                new THREE.MeshBasicMaterial({ color: 0xffffff }));
            cursor.visible = false;
            controller.add(cursor);
            this.ud(controller).cursor = cursor;

            controller.addEventListener('selectstart', () => this.onSelectStart(controller));
            controller.addEventListener('selectend', () => this.onSelectEnd(controller));
            controller.addEventListener('squeezestart', () => this.onSqueezeStart(controller));
            controller.addEventListener('squeezeend', () => this.onSqueezeEnd(controller));
            controller.addEventListener('connected', (e: any) => {
                this.ud(controller).inputSource = e.data ?? null;
            });
            controller.addEventListener('disconnected', () => {
                this.ud(controller).inputSource = null;
            });

            this.scene.add(controller);
            this.controllers.push(controller);
        }
    }

    private markInteraction(): void {
        this.lastInteraction = performance.now();
        if (this.mainPanel && !this.pinned) {
            this.mainPanel.visible = true;
        }
    }

    private onSelectStart(controller: THREE.Object3D): void {
        this.markInteraction();
        const target = this.hovered[this.ud(controller).index!];
        if (target?.slider) {
            this.draggingSlider = { controller, slider: target.slider };
        }
    }

    private onSelectEnd(controller: THREE.Object3D): void {
        this.markInteraction();
        if (this.draggingSlider?.controller === controller) {
            this.draggingSlider = null;
            return;
        }
        const target = this.hovered[this.ud(controller).index!];
        if (target?.button) {
            if (target.button.enabled) target.button.onClick?.();
        } else if (target?.tile) {
            this.ud(target.tile).onClick?.();
        }
    }

    private onSqueezeStart(controller: THREE.Object3D): void {
        this.markInteraction();
        if (this.grabbing) return;
        const target = this.hovered[this.ud(controller).index!];
        if (target?.panel) {
            // Ray-follow grab: the panel goes wherever the laser pointer
            // aims, at the depth it had when grabbed (distance from the
            // controller to the panel, preserved).
            const panelWorld = target.panel.getWorldPosition(this.tempVec);
            this.grabbing = {
                controller,
                panel: target.panel,
                rayDist: controller.position.distanceTo(panelWorld)
            };
        } else if (this.renderer?.xr.isPresenting) {
            // Squeezing empty space re-centers the world under the user -
            // the scene origin is frozen where the session started, so
            // walking away makes panels feel far and drags feel sluggish.
            this.recenterWorld();
        }
    }

    private recenterWorld(): void {
        const head = this.camera.position;
        const dist = Math.hypot(head.x, head.z);
        // Move the world TO the user: panels keep their local offsets, so
        // e.g. the media wall ends up 1.9m in front of the head again.
        this.worldRoot.position.set(head.x, 0, head.z);
        this.status$.next(dist > 0.5
            ? `Re-centered (you were ${dist.toFixed(1)}m from the scene). Squeeze empty space to re-center.`
            : 'Scene is already in front of you. Squeeze empty space to re-center.');
    }

    private onSqueezeEnd(controller: THREE.Object3D): void {
        if (this.grabbing?.controller === controller) {
            this.grabbing = null;
            this.saveLayout();
        }
    }

    // Raycast both controllers, update hover states, slider drags and grabs.
    private updateInteraction(dt: number): void {
        for (const controller of this.controllers) {
            const index = this.ud(controller).index!;
            this.tempMatrix.identity().extractRotation(controller.matrixWorld);
            this.raycaster.ray.origin.setFromMatrixPosition(controller.matrixWorld);
            this.raycaster.ray.direction.set(0, 0, -1).applyMatrix4(this.tempMatrix);

            const hits = this.raycaster.intersectObjects(this.scene.children, true)
                .filter(h => this.findInteractive(h.object) != null && this.isInVisibleBranch(h.object));
            const target = hits.length ? this.findInteractive(hits[0].object) : null;

            // Hover state
            if (this.hovered[index] !== target) {
                this.setHover(this.hovered[index], false);
                this.setHover(target, true);
                this.hovered[index] = target;
            }

            // Cursor
            const cursor = this.ud(controller).cursor;
            if (cursor) {
                cursor.visible = hits.length > 0;
                if (hits.length > 0) {
                    cursor.position.copy(controller.worldToLocal(hits[0].point.clone()));
                }
            }

            // Slider drag
            if (this.draggingSlider?.controller === controller && target?.slider && hits.length) {
                target.slider.setFromPoint(hits[0].point);
            }

            // Panel grab: the position follow and the face-the-controller
            // yaw happen after render() below, where the pose is freshest.
        }
    }

    // Walk up the scene graph to the interactive ancestor of a hit object.
    private findInteractive(obj: THREE.Object3D): InteractiveTarget | null {
        let current: THREE.Object3D | null = obj;
        while (current) {
            const ud = this.ud(current);
            if (ud.button) return { object: current, button: ud.button };
            if (ud.slider) return { object: current, slider: ud.slider };
            if (ud.tile) return { object: current, tile: ud.tile };
            if (ud.panel) return { object: current, panel: ud.panel };
            current = current.parent;
        }
        return null;
    }

    private ud(obj: THREE.Object3D): XrUserData {
        return obj.userData as XrUserData;
    }

    // Ignore hits on hidden panels (the raycaster tests geometry regardless
    // of the visible flag on ancestors).
    private isInVisibleBranch(obj: THREE.Object3D): boolean {
        let current: THREE.Object3D | null = obj;
        while (current) {
            if (!current.visible) return false;
            current = current.parent;
        }
        return true;
    }

    private setHover(target: InteractiveTarget | null, hovered: boolean): void {
        if (!target) return;
        if (target.button) {
            target.button.setHovered(hovered);
        } else if (target.tile) {
            const data = this.ud(target.tile).tileData;
            if (data) data.highlight.visible = hovered;
        } else if (target.panel) {
            target.panel.setHovered(hovered);
        } else if (target.slider) {
            target.slider.scale.setScalar(hovered ? 1.15 : 1.0);
        }
    }

    // ---- Layout persistence --------------------------------------------------
    // Panel positions/visibility are stored under a feature-private key so
    // removing the feature leaves the rest of the app's storage untouched.

    private static readonly LAYOUT_KEY = 'xtpXrPlayerLayout';

    private saveLayout(): void {
        try {
            const data: Record<string, { pos: number[]; rotY: number; visible: boolean }> = {};
            for (const p of this.panels) {
                data[p.panelId] = {
                    pos: [p.position.x, p.position.y, p.position.z],
                    rotY: p.rotation.y,
                    visible: p.visible
                };
            }
            localStorage.setItem(XrPlayerService.LAYOUT_KEY, JSON.stringify(data));
        } catch {
            // Storage unavailable - layout just won't persist.
        }
    }

    private loadLayout(): void {
        try {
            const raw = localStorage.getItem(XrPlayerService.LAYOUT_KEY);
            if (!raw) return;
            const data = JSON.parse(raw);
            for (const p of this.panels) {
                const entry = data[p.panelId];
                if (!entry) continue;
                if (Array.isArray(entry.pos) && entry.pos.length === 3) {
                    p.position.set(entry.pos[0], entry.pos[1], entry.pos[2]);
                }
                if (typeof entry.rotY === 'number') p.rotation.y = entry.rotY;
                if (typeof entry.visible === 'boolean') p.visible = entry.visible;
            }
            this.syncPanelToggleButtons();
        } catch {
            // Corrupt layout - fall back to defaults.
        }
    }

    // ---- Non-VR preview -------------------------------------------------------

    private setupPreviewMouse(canvas: HTMLCanvasElement): void {
        canvas.addEventListener('mousedown', e => {
            this.previewDragging = true;
            this.lastMouse = { x: e.clientX, y: e.clientY };
        });
        window.addEventListener('mousemove', e => {
            if (!this.previewDragging) return;
            const dx = e.clientX - this.lastMouse.x;
            const dy = e.clientY - this.lastMouse.y;
            this.lastMouse = { x: e.clientX, y: e.clientY };
            this.previewYaw -= dx * 0.003;
            this.previewPitch = THREE.MathUtils.clamp(
                this.previewPitch - dy * 0.003, -1.2, 1.2);
        });
        window.addEventListener('mouseup', () => this.previewDragging = false);
    }

    // ---- Render loop -----------------------------------------------------------

    private render = (_time: number): void => {
        if (!this.renderer) return;
        try {
            const dt = Math.min(this.clock.getDelta(), 0.1);

            if (!this.renderer.xr.isPresenting) {
                // Preview: orbit the camera with the mouse
                this.camera.rotation.set(0, 0, 0);
                this.camera.rotateY(this.previewYaw);
                this.camera.rotateX(this.previewPitch);
            }

            this.updateInteraction(dt);

            // Copy the current video frame into the texture canvas (~30fps).
            // The Quest's XR path does not upload VideoTextures directly.
            // The canvas is FIXED at 960x540 (16:9 = the screen mesh); the
            // video is letterboxed into it so the texture size never changes.
            {
                const v = this.videoEl;
                const cc = this.videoCopyCanvas;
                const cctx = this.videoCopyCtx;
                if (v && cc && cctx && this.videoTexture &&
                    (this.videoScreen?.visible || this.videoSphere?.visible) && !v.paused && v.videoWidth) {
                    const now = performance.now();
                    if (now - this.lastVideoCopy > 33) {
                        this.lastVideoCopy = now;
                        const W = cc.width, H = cc.height;
                        const va = v.videoWidth / v.videoHeight;
                        const ca = W / H;
                        let dw: number, dh: number, dx: number, dy: number;
                        if (va > ca) {
                            dw = W; dh = W / va; dx = 0; dy = (H - dh) / 2;
                        } else {
                            dh = H; dw = H * va; dx = (W - dw) / 2; dy = 0;
                        }
                        cctx.fillStyle = '#000';
                        cctx.fillRect(0, 0, W, H);
                        cctx.drawImage(v, dx, dy, dw, dh);
                        this.videoTexture.needsUpdate = true;
                    }
                }
            }

            // Update the in-scene status HUD (throttled).
            {
                const now = performance.now();
                if (now - this.lastHudDraw > 200) {
                    this.lastHudDraw = now;
                    this.hudText = this.statusText;
                    this.drawHud();
                }
            }

            // Auto-hide the main control panel after idle (VR only - the
            // preview always shows it, and it must never hide on frame one).
            if (this.mainPanel && !this.pinned && this.renderer.xr.isPresenting) {
                const idle = performance.now() - this.lastInteraction > 3000;
                const interacting = !!(this.grabbing || this.draggingSlider);
                this.mainPanel.visible = !idle || interacting;
            }

            this.renderer.render(this.scene, this.camera);

            // Apply the grab position AFTER render: three.js updates the
            // controller poses from the XR frame during render(), so doing
            // this first would always lag a frame behind the controller.
            if (this.grabbing) {
                // Panel follows the laser pointer at the grabbed depth:
                // point along the controller ray, scaled to rayDist.
                this.tempMatrix.identity().extractRotation(this.grabbing.controller.matrixWorld);
                this.tempVec.set(0, 0, -1).applyMatrix4(this.tempMatrix);
                this.tempVec.multiplyScalar(this.grabbing.rayDist).add(this.grabbing.controller.position);
                this.worldRoot.worldToLocal(this.tempVec);
                this.grabbing.panel.position.copy(this.tempVec);

                // Rotate while dragging: the panel yaws to face the
                // controller. The Quest exposes no thumbstick axes (both
                // XRInputSource.gamepad and getGamepads report 0), so hand
                // movement is the rotation input.
                const pw = this.grabbing.panel.getWorldPosition(this.tempVec);
                const dx = this.grabbing.controller.position.x - pw.x;
                const dz = this.grabbing.controller.position.z - pw.z;
                if (dx * dx + dz * dz > 0.001) {
                    const targetRot = Math.atan2(dx, dz);
                    let diff = targetRot - this.grabbing.panel.rotation.y;
                    while (diff > Math.PI) diff -= 2 * Math.PI;
                    while (diff < -Math.PI) diff += 2 * Math.PI;
                    this.grabbing.panel.rotation.y += diff * Math.min(1, dt * 8);
                }
            }
        } catch (e) {
            // An exception inside the XR frame loop can kill the session on
            // some headsets - log it and surface it on the overlay.
            console.error('XR render error:', e);
            this.status$.next('Render error: ' + this.errorMessage(e));
        }
    };

    // ---- Teardown ----------------------------------------------------------------

    ngOnDestroy(): void {
        this.stopStateTimer();
        if (this.onGlobalError) {
            window.removeEventListener('error', this.onGlobalError);
            this.onGlobalError = null;
        }
        if (this.onGlobalRejection) {
            window.removeEventListener('unhandledrejection', this.onGlobalRejection);
            this.onGlobalRejection = null;
        }
        // Safety net: make sure the page player's video node is restored if
        // we are torn down while our element was registered.
        if (this.previousVideoNode) {
            this.videoService.setVideoNode(this.previousVideoNode);
            this.previousVideoNode = null;
        }
        for (const sub of this.subs) sub.unsubscribe();
        this.subs = [];
        if (this.videoEl) {
            this.videoEl.pause();
            this.videoEl.src = '';
            this.videoEl.remove();
        }
        if (this.renderer) {
            this.renderer.setAnimationLoop(null);
            this.renderer.dispose();
            this.renderer = null;
        }
        this.initialized = false;
        this.mediaWall = null;
        this.scriptPanel = null;
        this.profilesPanel = null;
        this.tcodePanel = null;
        this.mainPanel = null;
        this.panels = [];
        this.controllers = [];
        this.supported$.next(false);
        this.sessionActive$.next(false);
        this.status$.next('');
    }
}