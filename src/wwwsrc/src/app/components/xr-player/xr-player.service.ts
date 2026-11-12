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
    MainControlPanel, SettingsPanel, HelpPanel
} from './xr-panels';

interface InteractiveTarget {
    object: THREE.Object3D;
    button?: Button3D;
    slider?: Slider3D;
    tile?: THREE.Group;
    panel?: Panel;
    skybox?: THREE.Mesh;
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
    skybox?: THREE.Mesh;
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
    private grabbingSkybox: { controller: THREE.Object3D; lastY: number } | null = null;
    private btn3Down: boolean[] = [false, false];
    private btn4Down: boolean[] = [false, false];
    private controlsVisible = false;
    private lastInteraction = 0;
    private pinned = false;
    // Passthrough (immersive-ar) support and state. When enabled and
    // available, the scene background goes transparent so the headset's
    // color passthrough (the room) shows through.
    private passthroughAvailable = false;
    private passthroughEnabled = false;
    private envFog: THREE.Fog | null = null;
    // 3D depth strength: 1 = full parallax. Below 1 each eye is blended
    // toward the mono center (less 3D, no image shift). Above 1 the two
    // eyes' 360 textures are panned apart to add parallax beyond full 3D.
    private stereoStrength = 1.0;
    private onGlobalError: ((e: ErrorEvent) => void) | null = null;
    private onGlobalRejection: ((e: PromiseRejectionEvent) => void) | null = null;

    private panels: Panel[] = [];
    private mediaWall: MediaWallPanel | null = null;
    private scriptPanel: ScriptStatusPanel | null = null;
    private profilesPanel: MotionProfilesPanel | null = null;
    private tcodePanel: TCodePanel | null = null;
    private mainPanel: MainControlPanel | null = null;
    private settingsPanel: SettingsPanel | null = null;
    private helpPanel: HelpPanel | null = null;

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
    // Stereo mode parsed from the file name (DeoVR convention). Stereo
    // videos render true 3D: one texture per eye, swapped per eye pass.
    private videoStereo: 'sbs' | 'tb' | null = null;
    // Projection mode of the current video (drives the copy-loop layout).
    private videoMode: 'flat' | '180' | '360' = 'flat';
    private videoLeftCanvas: HTMLCanvasElement | null = null;
    private videoLeftTexture: THREE.CanvasTexture | null = null;
    private videoRightCanvas: HTMLCanvasElement | null = null;
    private videoRightTexture: THREE.CanvasTexture | null = null;
    // Temp canvas holding the mono center (average of both eyes), used to
    // blend down the 3D depth without shifting the image.
    private videoMonoCanvas: HTMLCanvasElement | null = null;
    private videoFrame: THREE.Mesh | null = null;
    // Opaque black background sphere: occludes the AR passthrough when
    // passthrough is off (scene.background alone doesn't reliably occlude).
    private bgSphere: THREE.Mesh | null = null;
    private audio: THREE.Audio | null = null;
    private previousVideoNode: HTMLVideoElement | null = null;
    private stateTimer: number | null = null;
    // Auto-clear timer for transient status messages (so they don't linger
    // in the camera-attached HUD in front of the user's face).
    private statusClearTimer: number | null = null;
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
    private hudMesh: THREE.Mesh | null = null;
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

        const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
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
                if (supported) {
                    this.setStatus('Ready [v50]. Put on your headset and press Enter VR.');
                } else {
                    this.setStatus('WebXR immersive-vr is not supported on this device/browser. ' +
                        'The page must be served over HTTPS (or localhost).', false);
                }
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
        // Black background by default. Passthrough (immersive-ar) makes it
        // transparent so the room shows through - see applyPassthrough().
        this.scene.background = new THREE.Color(0x000000);
        this.scene.fog = new THREE.Fog(0x000000, 12, 40);
        this.envFog = this.scene.fog;

        // Opaque black background sphere: reliably occludes the AR
        // passthrough when passthrough is off (scene.background alone
        // doesn't). Toggled in applyPassthrough().
        this.bgSphere = new THREE.Mesh(
            new THREE.SphereGeometry(80, 32, 16),
            new THREE.MeshBasicMaterial({ color: 0x000000, side: THREE.BackSide })
        );
        this.worldRoot.add(this.bgSphere);

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
        // Start hidden: drawHud() shows it only while a message is active,
        // so a cleared/transient status leaves no opaque bar in view.
        hud.visible = false;
        this.camera.add(hud);
        this.hudMesh = hud;
        this.hudCanvas = canvas;
        this.hudCtx = ctx;
        this.hudTexture = texture;
        this.drawHud();
    }

    private drawHud(): void {
        const ctx = this.hudCtx;
        if (!ctx) return;
        const hasText = this.hudText.length > 0 || this.hudSubText.length > 0;
        // Hide the whole HUD (opaque background included) when the message
        // is empty, so a cleared/transient status leaves no dark bar in view.
        if (this.hudMesh) this.hudMesh.visible = hasText;
        if (!hasText) return;
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
        // Created at a small default size; setupVideoMeshes resizes each
        // canvas to the video's native resolution (capped) BEFORE the first
        // texture upload - a mid-playback resize leaves the Quest texture
        // mapped at the stale size (video appears in a corner).
        // Mono uses one canvas; stereo gets one canvas per eye (true 3D).
        const makeCopy = () => {
            const c = document.createElement('canvas');
            c.width = 960;
            c.height = 540;
            const t = new THREE.CanvasTexture(c);
            t.colorSpace = THREE.SRGBColorSpace;
            // Dynamic texture: regenerating mipmaps on every upload causes
            // shimmer/flicker and costs GPU time - use plain linear filtering.
            t.generateMipmaps = false;
            t.minFilter = THREE.LinearFilter;
            t.magFilter = THREE.LinearFilter;
            return { c, t };
        };
        const mono = makeCopy();
        this.videoCopyCanvas = mono.c;
        this.videoCopyCtx = mono.c.getContext('2d');
        this.videoTexture = mono.t;
        const left = makeCopy();
        this.videoLeftCanvas = left.c;
        this.videoLeftTexture = left.t;
        const right = makeCopy();
        this.videoRightCanvas = right.c;
        this.videoRightTexture = right.t;
        // Temp canvas for the mono center (average of both eyes) used to
        // blend down the 3D depth. Kept the same size as the eye canvases.
        this.videoMonoCanvas = document.createElement('canvas');
        this.videoMonoCanvas.width = 960;
        this.videoMonoCanvas.height = 540;

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
        // Grabbable: squeeze the skybox and drag up/down to tilt it on pitch.
        sphere.userData['skybox'] = sphere;
        // True stereo: onBeforeRender fires once per eye pass with that
        // eye's camera, so swap the sphere's texture per eye.
        sphere.onBeforeRender = (_renderer, _scene, camera) => {
            if (!this.videoStereo) return;
            const xrCam = this.renderer?.xr.getCamera();
            if (!xrCam) return;
            const isLeft = xrCam.cameras.length > 0 && camera === xrCam.cameras[0];
            (sphere.material as THREE.MeshBasicMaterial).map =
                isLeft ? this.videoLeftTexture : this.videoRightTexture;
        };
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
        // Hidden until button 4 is pressed (no auto-show, no idle timeout).
        this.mainPanel.visible = false;
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

        this.settingsPanel = new SettingsPanel({
            onTogglePassthrough: () => this.togglePassthrough(),
            onStereoStrength: (v) => this.setStereoStrength(v)
        });
        this.settingsPanel.position.set(1.3, 1.5, -1.7);
        this.settingsPanel.rotation.y = -0.4;
        this.settingsPanel.visible = false;
        this.worldRoot.add(this.settingsPanel);
        this.panels.push(this.settingsPanel);

        // Help panel: the controller instructions. Shown on session start
        // (replaces the old long startup status message), closed with its
        // Close button, re-opened from the control panel's Help toggle.
        this.helpPanel = new HelpPanel(() => this.closeHelp());
        this.helpPanel.position.set(0, 1.5, -1.7);
        this.worldRoot.add(this.helpPanel);
        this.panels.push(this.helpPanel);

        this.syncPanelToggleButtons();
    }

    // ---- Session lifecycle -------------------------------------------------

    async enterVR(): Promise<void> {
        if (!this.renderer || !navigator.xr) return;
        const xr = navigator.xr;
        let session: XRSession;
        // Prefer immersive-ar: it exposes the headset's color passthrough so
        // the "Passthrough" toggle can reveal the room. Fall back to
        // immersive-vr (opaque) where AR isn't available - passthrough is
        // then unavailable and the background stays black.
        const requestSession = (mode: XRSessionMode) => xr.requestSession(mode, {
            // dom-overlay keeps the status line readable inside the
            // headset (the only way to surface errors in VR).
            optionalFeatures: ['local-floor', 'bounded-floor', 'dom-overlay'],
            domOverlay: { root: this.overlayRoot ?? document.body }
        });
        try {
            try {
                session = await requestSession('immersive-ar');
                this.passthroughAvailable = true;
            } catch {
                session = await requestSession('immersive-vr');
                this.passthroughAvailable = false;
            }
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
        // Apply the current passthrough state (black background by default).
        this.applyPassthrough();
        // Show the help panel with the controller instructions (replaces the
        // old long startup status message). A short status line remains so an
        // empty line is indistinguishable from a missing dom-overlay.
        if (this.helpPanel) this.helpPanel.visible = true;
        const feats = (session as any).enabledFeatures as string[] | undefined;
        this.setStatus(
            'In VR. See the Help panel for controls.' +
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

    // Toggle between the opaque black background and the headset's color
    // passthrough (room visible). Only works when the session was granted
    // as immersive-ar.
    togglePassthrough(): void {
        if (!this.passthroughAvailable) {
            this.setStatus('Passthrough is not available in this session (no AR support).');
            return;
        }
        this.passthroughEnabled = !this.passthroughEnabled;
        this.applyPassthrough();
        this.settingsPanel?.setPassthroughActive(this.passthroughEnabled);
        this.setStatus(this.passthroughEnabled
            ? 'Passthrough ON (room visible).'
            : 'Passthrough OFF (black background).');
    }

    private applyPassthrough(): void {
        if (!this.renderer) return;
        if (this.passthroughEnabled && this.passthroughAvailable) {
            // Transparent: the headset's color passthrough shows through.
            this.scene.background = null;
            this.scene.fog = null;
            if (this.bgSphere) this.bgSphere.visible = false;
            this.renderer.setClearColor(0x000000, 0);
        } else {
            // Opaque black: the sphere occludes the passthrough.
            this.scene.background = new THREE.Color(0x000000);
            this.scene.fog = this.envFog;
            if (this.bgSphere) this.bgSphere.visible = true;
            this.renderer.setClearColor(0x000000, 1);
        }
    }

    // Set the 3D depth strength (1 = full parallax, lower = reduced). The
    // copy loop reads this each frame: below 1 it blends each eye toward
    // the mono center by (1 - strength); above 1 it pans the eyes' 360
    // images apart (left eye left, right eye right) to add parallax.
    setStereoStrength(value: number): void {
        this.stereoStrength = THREE.MathUtils.clamp(value, 0, 2);
    }

    private onSessionEnd(): void {
        this.sessionActive$.next(false);
        if (this.canvas) this.canvas.style.visibility = '';
        const hadVideo = this.videoService.isMediaItemPlaying();
        this.handoffToWebPlayer();
        if (!hadVideo) {
            // Session ended without playback - say so, so an unexpected
            // early exit is visible on the overlay.
            this.setStatus('VR session ended.', false);
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
            this.setStatus(`Continuing playback: ${item.name}`);
        }
    }

    // ---- Video -------------------------------------------------------------

    private startVideo(item: MediaItem): void {
        if (this.externalStreaming) {
            this.setStatus('External streaming is enabled - disable it in Settings to play media in VR.', false);
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
            this.setStatus('Video playback was blocked: ' + this.errorMessage(e) +
                '. Pull the trigger on a media item again.', false);
        });

        // DeoVR-style flags in the file name pick the projection
        // (case-insensitive, any order): _180 / _360 select equirect
        // sphere video. A stereo flag (SBS/LR/3DH or TB/3DV/OverUnder)
        // crops one eye for mono display. The "360" tag is a fallback.
        const flags = this.parseVideoFlags(item.name + ' ' + (item.relativePath || ''));
        const mode: 'flat' | '180' | '360' =
            flags.fov ?? (item.metaData?.tags?.includes('360') === true ? '360' : 'flat');
        this.videoStereo = flags.stereo;

        // Map the texture(s) for the projection. Sphere UVs: u=0 is -x,
        // u=0.25 is +z (behind), u=0.5 is +x, u=0.75 is -z (in front).
        // The shader samples texture at (uv * repeat + offset), so:
        // Stereo: the per-eye textures are SEPARATE objects and need the
        // same transform as the mono one.
        this.videoMode = mode;
        const texs = this.videoStereo
            ? [this.videoTexture, this.videoLeftTexture, this.videoRightTexture]
            : [this.videoTexture];
        for (const tex of texs) {
            if (!tex) continue;
            if (mode === '360') {
                // Full equirect: image center faces the user (-z), image
                // left of center on the viewer's left.
                tex.wrapS = THREE.RepeatWrapping;
                tex.wrapT = THREE.ClampToEdgeWrapping;
                tex.repeat.set(-1, 1);
                tex.offset.set(1.25, 0);
            } else {
                // Flat and 180: identity mapping - the 180 layout (black
                // back hemisphere, letterboxed band) is baked into the
                // canvas by the copy loop.
                tex.wrapS = THREE.ClampToEdgeWrapping;
                tex.wrapT = THREE.ClampToEdgeWrapping;
                tex.repeat.set(1, 1);
                tex.offset.set(0, 0);
            }
            // Anisotropic filtering keeps the sphere sharp at oblique angles.
            tex.anisotropy = this.renderer?.capabilities.getMaxAnisotropy() ?? 1;
            tex.needsUpdate = true;
        }

        // Defer showing the mesh until the video's dimensions are known so
        // the canvas can be sized to NATIVE resolution BEFORE the first
        // texture upload. A mid-playback canvas resize leaves the Quest
        // texture mapped at the stale size (video appears in a corner), so
        // the resize must happen before the mesh's first render.
        this.videoSphere!.visible = false;
        this.videoScreen!.visible = false;
        this.videoFrame!.visible = false;
        const v = this.videoEl!;
        if (v.readyState >= 1) {
            this.setupVideoMeshes();
        } else {
            v.addEventListener('loadedmetadata', () => this.setupVideoMeshes(), { once: true });
        }

        this.startStateTimer();
        this.setStatus(`Playing: ${item.name} [${mode === 'flat' ? '2D' : mode + ' sphere'}]` +
            (flags.stereo ? ` (stereo ${flags.stereo.toUpperCase()} 3D)` : ''));
    }

    // Parse DeoVR-style projection/stereo flags from a file name. Flags are
    // case-insensitive and may appear in any order, e.g.
    // "My_video_SBS_alpha_mkx200.mp4". The number flags match 180/360 as
    // standalone tokens (any separator, not part of a longer number).
    private parseVideoFlags(name: string): { fov: '180' | '360' | null; stereo: 'sbs' | 'tb' | null } {
        const n = name.toLowerCase();
        let fov: '180' | '360' | null = null;
        if (/(^|[^a-z0-9])180(?![0-9])/.test(n)) fov = '180';
        else if (/(^|[^a-z0-9])360(?![0-9])/.test(n)) fov = '360';
        let stereo: 'sbs' | 'tb' | null = null;
        if (/(^|[^a-z0-9])(l-r|lr|3dh|sbs)([^a-z]|$)/.test(n)) stereo = 'sbs';
        else if (/(^|[^a-z0-9])(tb|3dv|overunder)([^a-z]|$)/.test(n)) stereo = 'tb';
        return { fov, stereo };
    }

    // Target copy-canvas size for the current video at NATIVE resolution
    // (capped for the Quest). isEye selects the per-eye canvas (stereo);
    // otherwise the mono canvas. 180° uses a 2:1 canvas whose left half is
    // black (the back hemisphere) and right half is the content, so it is
    // 2x the content width.
    private computeCanvasSize(isEye: boolean): { w: number; h: number } | null {
        const v = this.videoEl;
        if (!v || !v.videoWidth) return null;
        const vw = v.videoWidth, vh = v.videoHeight;
        const MAXW = 4096, MAXH = 2048;
        const st = this.videoStereo;
        let w: number, h: number;
        if (this.videoMode === '360') {
            if (isEye) {
                if (st === 'sbs') { w = vw / 2; h = vh; }
                else if (st === 'tb') { w = vw; h = vh / 2; }
                else { w = vw; h = vh; }
            } else { w = vw; h = vh; }
        } else if (this.videoMode === '180') {
            if (isEye) {
                if (st === 'sbs') { w = vw; h = vh; }          // eye=vw/2 wide, canvas=2*eye=vw
                else if (st === 'tb') { w = vw * 2; h = vh / 2; } // eye=vw x vh/2, canvas=2*vw x vh/2
                else { w = vw * 2; h = vh; }
            } else { w = vw * 2; h = vh; }
        } else {
            w = vw; h = vh;
        }
        const s = Math.min(1, MAXW / w, MAXH / h);
        return { w: Math.max(2, Math.round(w * s)), h: Math.max(2, Math.round(h * s)) };
    }

    // Size the copy canvas(es) to the video's native resolution and show the
    // correct mesh. Called from 'loadedmetadata' (or immediately if metadata
    // is already present) so the resize happens BEFORE the first texture
    // upload - the mesh is kept hidden until then.
    private setupVideoMeshes(): void {
        const v = this.videoEl;
        if (!v || !v.videoWidth) return;
        const resizeCanvas = (c: HTMLCanvasElement | null, isEye: boolean) => {
            if (!c) return;
            const size = this.computeCanvasSize(isEye);
            if (size && (c.width !== size.w || c.height !== size.h)) {
                c.width = size.w;
                c.height = size.h;
            }
        };
        if (this.videoStereo) {
            resizeCanvas(this.videoLeftCanvas, true);
            resizeCanvas(this.videoRightCanvas, true);
            // Keep the mono-center canvas matched to the eye canvases.
            if (this.videoMonoCanvas && this.videoLeftCanvas) {
                this.videoMonoCanvas.width = this.videoLeftCanvas.width;
                this.videoMonoCanvas.height = this.videoLeftCanvas.height;
            }
        } else {
            resizeCanvas(this.videoCopyCanvas, false);
        }
        if (this.videoMode !== 'flat') {
            const m = this.videoSphere!.material as THREE.MeshBasicMaterial;
            // Stereo: onBeforeRender swaps this per eye pass.
            m.map = this.videoStereo ? this.videoLeftTexture : this.videoTexture;
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
    }

    private stopVideo(): void {
        if (!this.videoEl) return;
        this.videoEl.pause();
        this.stopStateTimer();
        this.videoService.clearVideo();
        this.hideVideoMeshes();
        this.setStatus('Stopped.');
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

    // Button 4: show/hide the control panel and the media wall together.
    // When shown, the control panel stays visible (no auto-hide).
    private toggleControlsAndMedia(): void {
        if (!this.mainPanel) return;
        this.controlsVisible = !this.controlsVisible;
        this.mainPanel.visible = this.controlsVisible;
        const media = this.panels.find(p => p.panelId === 'media');
        if (media) media.visible = this.controlsVisible;
        if (this.controlsVisible) this.markInteraction();
        this.syncPanelToggleButtons();
        this.saveLayout();
    }

    // Close the help panel (its Close button). Re-openable from the control
    // panel's Help toggle (togglePanel('help')).
    private closeHelp(): void {
        if (!this.helpPanel) return;
        this.helpPanel.visible = false;
        this.syncPanelToggleButtons();
        this.saveLayout();
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
    }

    private onSelectStart(controller: THREE.Object3D): void {
        // A button (trigger) brings up the control panel.
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
        if (this.grabbing || this.grabbingSkybox) return;
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
        } else if (target?.skybox) {
            // Grab the skybox: dragging up/down rotates it on the pitch axis.
            this.grabbingSkybox = { controller, lastY: controller.position.y };
        }
    }

    // Set a status message. Transient messages (the default) auto-clear after
    // a few seconds so they don't linger in the camera-attached HUD; pass
    // transient=false for messages that should stay (errors, session state).
    private setStatus(msg: string, transient = true): void {
        this.status$.next(msg);
        if (this.statusClearTimer) {
            clearTimeout(this.statusClearTimer);
            this.statusClearTimer = null;
        }
        if (transient) {
            this.statusClearTimer = window.setTimeout(() => {
                this.statusClearTimer = null;
                this.status$.next('');
            }, 4000);
        }
    }

    private recenterWorld(): void {
        const head = this.camera.position;
        const dist = Math.hypot(head.x, head.z);
        // Move the world TO the user: panels keep their local offsets, so
        // e.g. the media wall ends up 1.9m in front of the head again.
        this.worldRoot.position.set(head.x, 0, head.z);
        this.setStatus(dist > 0.5
            ? `Re-centered (you were ${dist.toFixed(1)}m from the scene).`
            : 'Scene re-centered.');
    }

    private onSqueezeEnd(controller: THREE.Object3D): void {
        if (this.grabbing?.controller === controller) {
            this.grabbing = null;
            this.saveLayout();
        }
        if (this.grabbingSkybox?.controller === controller) {
            this.grabbingSkybox = null;
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
            if (ud.skybox) return { object: current, skybox: ud.skybox };
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
        }
        // Sliders don't scale on hover: scaling a wide slider stretches the
        // track and knob, making it hard to grab. The cursor dot is the
        // hover feedback.
    }

    // ---- Layout persistence --------------------------------------------------
    // Panel positions/visibility are stored under a feature-private key so
    // removing the feature leaves the rest of the app's storage untouched.

    private static readonly LAYOUT_KEY = 'xtpXrPlayerLayout';

    private saveLayout(): void {
        try {
            const data: Record<string, { pos: number[]; rotY: number; rotX: number; visible: boolean }> = {};
            for (const p of this.panels) {
                data[p.panelId] = {
                    pos: [p.position.x, p.position.y, p.position.z],
                    rotY: p.rotation.y,
                    rotX: p.rotation.x,
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
                if (typeof entry.rotX === 'number') p.rotation.x = entry.rotX;
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

            // Controller button bindings (edge-detected per controller):
            //   button 3 -> re-center the world under the user
            //   button 4 -> show the control panel and toggle the media list
            for (const controller of this.controllers) {
                const index = this.ud(controller).index!;
                const gamepad = this.ud(controller).inputSource?.gamepad;
                if (gamepad && gamepad.buttons) {
                    const b3 = !!gamepad.buttons[3]?.pressed;
                    if (b3 && !this.btn3Down[index]) {
                        this.recenterWorld();
                    }
                    this.btn3Down[index] = b3;
                    const b4 = !!gamepad.buttons[4]?.pressed;
                    if (b4 && !this.btn4Down[index]) {
                        this.toggleControlsAndMedia();
                    }
                    this.btn4Down[index] = b4;
                }
            }

            // Skybox grab: dragging the controller up/down tilts the 360
            // sphere on the pitch axis (re-aims the video's vertical center).
            if (this.grabbingSkybox && this.videoSphere) {
                const dy = this.grabbingSkybox.controller.position.y - this.grabbingSkybox.lastY;
                this.grabbingSkybox.lastY = this.grabbingSkybox.controller.position.y;
                this.videoSphere.rotation.x += dy * 4.0;
            }

            // Copy the current video frame into the texture canvas (~30fps).
            // The Quest's XR path does not upload VideoTextures directly.
            // The canvas is sized to the video's NATIVE resolution (capped)
            // in setupVideoMeshes, before the first upload.
            {
                const v = this.videoEl;
                const cc = this.videoCopyCanvas;
                const cctx = this.videoCopyCtx;
                if (v && cc && cctx && this.videoTexture &&
                    (this.videoScreen?.visible || this.videoSphere?.visible) && !v.paused && v.videoWidth) {
                    const now = performance.now();
                    if (now - this.lastVideoCopy > 0) {
                        this.lastVideoCopy = now;
                        const W = cc.width, H = cc.height;
                        if (this.videoSphere?.visible) {
                            // Sphere mode: bake the projection into a 2:1
                            // canvas (matches the sphere UVs).
                            // 360: the frame fills the canvas.
                            // 180: left half black (behind the viewer),
                            // right half = the frame flipped horizontally
                            // (its left edge lands on the viewer's left),
                            // letterboxed to the equirectangular band so
                            // the top/bottom caps are black.
                            const is180 = this.videoMode === '180';
                            const drawSphere = (ctx: CanvasRenderingContext2D,
                                sx: number, sy: number, sw: number, sh: number,
                                shiftFrac = 0) => {
                                const w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = '#000';
                                ctx.fillRect(0, 0, w, h);
                                if (!is180) {
                                    const shift = Math.round(shiftFrac * w);
                                    if (shift === 0) {
                                        ctx.drawImage(v, sx, sy, sw, sh, 0, 0, w, h);
                                    } else {
                                        // Shifted equirect with wrap: draw the
                                        // image at the offset plus both wrap
                                        // positions so the seam stays seamless.
                                        ctx.drawImage(v, sx, sy, sw, sh, shift, 0, w, h);
                                        ctx.drawImage(v, sx, sy, sw, sh, shift - w, 0, w, h);
                                        ctx.drawImage(v, sx, sy, sw, sh, shift + w, 0, w, h);
                                    }
                                } else {
                                    // Band height from the frame's equirect
                                    // aspect: a 2:1 frame covers 90° (half
                                    // the height), taller frames cover more.
                                    const bandFrac = sw / sh >= 1 ? sh / sw : 1;
                                    const dh = h * bandFrac;
                                    ctx.save();
                                    ctx.translate(w, 0);
                                    ctx.scale(-1, 1);
                                    // Flipped space: dest x=0..w/2 lands on the
                                    // canvas RIGHT half (the front hemisphere);
                                    // the source's left edge lands at canvas x=w
                                    // (the viewer's left).
                                    ctx.drawImage(v, sx, sy, sw, sh, 0, (h - dh) / 2, w / 2, dh);
                                    ctx.restore();
                                }
                            };
                            if (this.videoStereo &&
                                this.videoLeftCanvas && this.videoRightCanvas &&
                                this.videoLeftTexture && this.videoRightTexture) {
                                // True stereo: one eye per canvas.
                                const lw = v.videoWidth, lh = v.videoHeight;
                                const lctx = this.videoLeftCanvas.getContext('2d');
                                const rctx = this.videoRightCanvas.getContext('2d');
                                if (lctx && rctx) {
                                    // 3D boost above full: pan each eye's 360
                                    // image apart (left eye left, right eye
                                    // right) to add parallax beyond full 3D.
                                    const boost = Math.max(0, this.stereoStrength - 1.0) * 0.06;
                                    if (this.videoStereo === 'sbs') {
                                        drawSphere(lctx, 0, 0, lw / 2, lh, -boost);
                                        drawSphere(rctx, lw / 2, 0, lw / 2, lh, boost);
                                    } else {
                                        drawSphere(lctx, 0, 0, lw, lh / 2, -boost);
                                        drawSphere(rctx, 0, lh / 2, lw, lh / 2, boost);
                                    }
                                    // Reduce 3D: blend each eye toward the mono
                                    // center (average of both eyes). Overlaying
                                    // the mono at alpha (1-strength) on top of
                                    // each eye gives lerp(eye, mono, 1-s) with
                                    // NO image shift - far more comfortable than
                                    // shifting the textures.
                                    const k = 1 - this.stereoStrength;
                                    if (k > 0 && this.videoMonoCanvas) {
                                        const mctx = this.videoMonoCanvas.getContext('2d');
                                        const mw = this.videoLeftCanvas.width;
                                        const mh = this.videoLeftCanvas.height;
                                        if (mctx && this.videoMonoCanvas.width === mw &&
                                            this.videoMonoCanvas.height === mh) {
                                            // mono = (left + right) / 2
                                            mctx.clearRect(0, 0, mw, mh);
                                            mctx.globalAlpha = 0.5;
                                            mctx.drawImage(this.videoLeftCanvas, 0, 0);
                                            mctx.drawImage(this.videoRightCanvas, 0, 0);
                                            mctx.globalAlpha = 1;
                                            // Overlay mono on each eye: lerp(eye, mono, k).
                                            lctx.globalAlpha = k;
                                            lctx.drawImage(this.videoMonoCanvas, 0, 0);
                                            lctx.globalAlpha = 1;
                                            rctx.globalAlpha = k;
                                            rctx.drawImage(this.videoMonoCanvas, 0, 0);
                                            rctx.globalAlpha = 1;
                                        }
                                    }
                                    this.videoLeftTexture.needsUpdate = true;
                                    this.videoRightTexture.needsUpdate = true;
                                }
                            } else {
                                drawSphere(cctx, 0, 0, v.videoWidth, v.videoHeight);
                                this.videoTexture.needsUpdate = true;
                            }
                        } else {
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

                // Rotate while dragging: face the user's HEAD (not the
                // hand) so the panel stays aimed at the user while the hand
                // moves the laser - facing the hand made the panel spin
                // around the user while dragging. Turning the head is the
                // rotation input. Pitch is damped to half strength: full
                // pitch made the panel swing hard when the hand crossed its
                // height, and it was hard to level again.
                const pw = this.grabbing.panel.getWorldPosition(this.tempVec);
                const dx = this.camera.position.x - pw.x;
                const dy = this.camera.position.y - pw.y;
                const dz = this.camera.position.z - pw.z;
                if (dx * dx + dy * dy + dz * dz > 0.001) {
                    const targetYaw = Math.atan2(dx, dz);
                    // Pitch is damped to half strength: full pitch made the
                    // panel swing hard whenever the hand crossed its height,
                    // and it was hard to level again.
                    const targetPitch = Math.atan2(-dy, dz) * 0.5;
                    let diff = targetYaw - this.grabbing.panel.rotation.y;
                    while (diff > Math.PI) diff -= 2 * Math.PI;
                    while (diff < -Math.PI) diff += 2 * Math.PI;
                    const ease = Math.min(1, dt * 8);
                    this.grabbing.panel.rotation.y += diff * ease;
                    this.grabbing.panel.rotation.x += (targetPitch - this.grabbing.panel.rotation.x) * ease;
                }
            }
        } catch (e) {
            // An exception inside the XR frame loop can kill the session on
            // some headsets - log it and surface it on the overlay.
            console.error('XR render error:', e);
            this.setStatus('Render error: ' + this.errorMessage(e), false);
        }
    };

    // ---- Teardown ----------------------------------------------------------------

    ngOnDestroy(): void {
        this.stopStateTimer();
        if (this.statusClearTimer) {
            clearTimeout(this.statusClearTimer);
            this.statusClearTimer = null;
        }
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
        this.settingsPanel = null;
        this.helpPanel = null;
        this.panels = [];
        this.controllers = [];
        this.supported$.next(false);
        this.sessionActive$.next(false);
        this.status$.next('');
    }
}