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
    MainControlPanel, SettingsPanel, HelpPanel, ConfirmDialog
} from './xr-panels';
import { XrSettingsService, XrPanelLayout } from './xr-settings.service';

// DeoVR-style projection detected from the file name (case-insensitive, any
// order). Equirectangular (180/360) and fisheye (180/190/200/220) are the
// supported meshes; "flat" is a regular 2D plane.
type VideoProjection =
    | 'flat'
    | 'equirect180'
    | 'equirect360'
    | 'fisheye180'
    | 'fisheye190'
    | 'fisheye200'
    | 'fisheye220';

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
    private camera = new THREE.PerspectiveCamera(70, 16 / 9, 0.1, 100);
    private raycaster = new THREE.Raycaster();
    private tempMatrix = new THREE.Matrix4();
    private tempVec = new THREE.Vector3();
    private clock = new THREE.Clock();
    private initialized = false;

    private controllers: THREE.Object3D[] = [];
    // Controller laser lines, kept so they can be faded in/out (see
    // updateLaserVisibility). They are hidden when no panel is open to reduce
    // clutter, and reappear on trigger press / while a panel is open.
    private laserLines: THREE.Line[] = [];
    // Cursor dots at the end of each laser. Faded in/out together with the
    // laser lines (see updateLaserVisibility) so they hide when the lasers do.
    private laserDots: THREE.Mesh[] = [];
    // performance.now() of the last trigger press. The lasers stay lit for a
    // short window after a trigger even when no panel is open, so the user can
    // point at whatever is on screen, then fade out again.
    private lastTriggerAt = -Infinity;
    private hovered: (InteractiveTarget | null)[] = [null, null];
    private draggingSlider: { controller: THREE.Object3D; slider: Slider3D } | null = null;
    private grabbing: { controller: THREE.Object3D; panel: Panel; rayDist: number } | null = null;
    private rotatingSkybox: { controller: THREE.Object3D; lastQuat: THREE.Quaternion } | null = null;
    // Skybox roll (radians). Applied to the CAMERA, not the sphere: rolling the
    // view keeps the side-by-side split line vertical so the eyes stay aligned
    // (rolling the sphere tilts the split line and cross-eyes). Heresphere-style.
    private skyboxRoll = 0;
    // Camera position offsets (meters, in the camera's local frame).
    private camZoom = 0;  // surge: forward/backward along view direction
    private camHeave = 0; // up/down (world Y)
    private camSway = 0;  // left/right (camera local X)
    private btn3Down: boolean[] = [false, false];
    private btn4Down: boolean[] = [false, false];
    private controlsVisible = false;
    // Panels the user has manually closed (via their X button). These are
    // excluded from the Button-4 toggle until the user re-opens them, so a
    // closed panel stays closed while the others toggle.
    private userClosedPanels: Set<string> = new Set();
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
    // Transient confirmation dialog (not part of this.panels, so it is never
    // persisted and never toggled by the main panel).
    private confirmDialog: ConfirmDialog | null = null;
    // Panel positions/visibility captured right after buildPanels() - the
    // defaults that a "Clear XR settings" reset returns to.
    private defaultLayout: XrPanelLayout = {};

    // Video
    private videoEl: HTMLVideoElement | null = null;
    // One-shot guard so the "video not decoding" diagnostic (below) is only
    // surfaced once per clip, not every XR frame.
    private videoDiagShown = false;
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
    // Projection mode of the current video (drives the copy-loop layout and
    // which material/mesh is shown).
    private videoMode: VideoProjection = 'flat';
    // Manual format override (the "Format"/"Stereo" combo boxes on the control
    // panel). 'auto' = detect from the file name; anything else forces that
    // projection/stereo for the current video. This fixes mis-named files
    // (e.g. a fisheye200 with no stereo flag, or a fisheye190 the name
    // doesn't describe). Each new video resets the override to Auto, so
    // detection is the default and the user re-applies a manual format per
    // video when needed.
    private formatProjection: VideoProjection | 'auto' = 'auto';
    private formatStereo: 'sbs' | 'tb' | 'mono' | 'auto' = 'auto';
    // Shader material that maps a pre-lens-corrected equidistant fisheye frame
    // onto the sphere (DeoVR _fisheye / _fisheye190 / _mkx200 / _vrca220).
    private videoFisheyeMaterial: THREE.ShaderMaterial | null = null;
    // The sphere's base material (equirect 180/360). Kept separately so it can
    // be swapped out for the fisheye shader and back again.
    private videoSphereMaterial: THREE.MeshBasicMaterial | null = null;
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
        private appState: AppStateService,
        private xrSettings: XrSettingsService
    ) {
        // Data subscriptions are established in init() (after the panels
        // exist), not here: the sources are BehaviorSubjects, so a
        // constructor-time subscription fires while mediaWall & co are
        // still null and the rebuilt panels would never receive data.
        //
        // WebXR support is checked HERE (not in init()) so the header's VR
        // button can show/hide based on support before the player is ever
        // opened. init() only runs once the player component is created, so
        // the check must not live there - that was a chicken-and-egg deadlock:
        // the button was hidden until supported$ was set, but supported$ was
        // only set after the button was clicked to open the player.
        this.checkXrSupport();
    }

    // Check WebXR immersive-vr support and publish it on supported$. Safe to
    // run at construction: it only reads navigator.xr.
    private checkXrSupport(): void {
        if (navigator.xr) {
            navigator.xr.isSessionSupported('immersive-vr')
                .then(supported => this.supported$.next(supported))
                .catch((e) => {
                    this.supported$.next(false);
                    this.status$.next('WebXR support check failed: ' + this.errorMessage(e));
                });
        } else {
            this.supported$.next(false);
        }
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
            this.serverService.serverChannels$.subscribe(settings => {
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
        this.laserLines = [];
        this.laserDots = [];
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
        this.videoFisheyeMaterial = null;
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
        // Capture the default panel layout (positions/visibility set in
        // buildPanels) before any persisted layout is applied, so a settings
        // reset can return the panels to these defaults.
        this.defaultLayout = this.collectLayout();
        this.subscribeToData();
        this.subs.push(this.status$.subscribe(t => this.statusText = t));
        this.subscribeToXrSettings();
        this.setupControllers();
        this.setupPreviewMouse(canvas);

        // WebXR support was already checked in the constructor (checkXrSupport)
        // so the header button can appear before the player opens. Here we just
        // reflect the result in the player's status line now that it's visible.
        if (this.supported$.value) {
            // Non-transient: this is the build/version confirmation the
            // user checks before putting on the headset - it must stay.
            this.setStatus('Ready [v52]. Put on your headset and press Enter VR.', false);
        } else {
            this.setStatus('WebXR immersive-vr is not supported on this device/browser. ' +
                'The page must be served over HTTPS (or localhost).', false);
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
        // Surface a media load/decode error on the headset. A non-zero code
        // here (especially 4 = FORMAT/codec not supported) means the browser
        // on this headset cannot decode the clip - the element never produces
        // frames, so the skybox stays black (the "only passthrough" symptom).
        video.addEventListener('error', () => {
            const e = video.error;
            const names: Record<number, string> = {
                1: 'ABORTED', 2: 'NETWORK', 3: 'DECODE', 4: 'FORMAT/codec not supported'
            };
            this.setStatus('Video will not play: ' +
                (e ? `${names[e.code] || e.code} (code ${e.code})` : 'unknown error') +
                '. This headset/browser may not support the clip\'s codec.', false);
        });

        // Video screen (regular media). Placed BEHIND the panel arc (radius
        // 5.0) so the wide 2D plane never sits in front of and occludes the
        // control panel / other panels. Pulled closer to 4.5m so the 2D video
        // feels nearer; the help panel sits just in front of it (radius 4.2).
        const screen = new THREE.Mesh(
            new THREE.PlaneGeometry(4.2, 2.36),
            new THREE.MeshBasicMaterial({ color: 0x000000 })
        );
        screen.position.set(0, 1.6, -4.5);
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
        frame.position.set(0, 1.6, -4.51);
        frame.visible = false;
        this.videoFrame = frame;
        this.worldRoot.add(frame);

        // Sphere for equirect (180/360) and fisheye media. The equirect path
        // uses a basic material; the fisheye path swaps in a shader material
        // (see setupVideoMeshes).
        this.videoSphereMaterial = new THREE.MeshBasicMaterial({ color: 0x000000, side: THREE.BackSide });
        const sphere = new THREE.Mesh(
            new THREE.SphereGeometry(12, 60, 40),
            this.videoSphereMaterial
        );
        sphere.visible = false;
        this.videoSphere = sphere;
        // Fisheye shader (equidistant fisheye -> equirectangular). DeoVR fisheye
        // files are pre-lens-corrected to a clean equidistant fisheye in
        // post-production, so the player only needs the standard equidistant
        // mapping with the FOV from the file-name flag (uThetaMax).
        this.videoFisheyeMaterial = new THREE.ShaderMaterial({
            side: THREE.BackSide,
            uniforms: {
                'uMap': { value: this.videoTexture },
                'uThetaMax': { value: 90 * Math.PI / 180 }
            },
            vertexShader: `
                varying vec3 vDir;
                void main() {
                    // Local position on the unit sphere = the view direction in
                    // the sphere's own space (so the image rotates with the
                    // skybox when it is pitched/yawed/rolled).
                    vDir = normalize(position);
                    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
                }
            `,
            fragmentShader: `
                uniform sampler2D uMap;
                uniform float uThetaMax;
                varying vec3 vDir;
                // sRGB encode: the texture is uploaded as sRGB (decoded to
                // linear on sample) and a ShaderMaterial does NOT get three.js'
                // output-colorspace chunk, so encode back to sRGB ourselves.
                vec3 srgbEncode(vec3 c) {
                    vec3 lo = c * 12.92;
                    vec3 hi = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
                    return mix(lo, hi, step(0.0031308, c));
                }
                void main() {
                    vec3 d = normalize(vDir);
                    // Optical axis points -z (straight ahead of the user).
                    float theta = acos(clamp(-d.z, -1.0, 1.0));
                    if (theta > uThetaMax) {
                        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
                        return;
                    }
                    // Equidistant fisheye: radial distance proportional to the
                    // angle from the optical axis, normalized to 1 at the edge.
                    float r = theta / uThetaMax;
                    float phi = atan(d.x, d.y);
                    float nx = r * sin(phi);
                    float ny = r * cos(phi);
                    vec2 uv = vec2(0.5 + nx * 0.5, 0.5 + ny * 0.5);
                    gl_FragColor = vec4(srgbEncode(texture2D(uMap, uv).rgb), 1.0);
                }
            `
        });
        // Grabbable: squeeze the skybox and drag up/down to tilt it on pitch.
        sphere.userData['skybox'] = sphere;
        // True stereo: onBeforeRender fires once per eye pass with that
        // eye's camera, so swap the sphere's texture per eye.
        sphere.onBeforeRender = (_renderer, _scene, camera) => {
            if (!this.videoStereo) return;
            const xrCam = this.renderer?.xr.getCamera();
            if (!xrCam) return;
            const isLeft = xrCam.cameras.length > 0 && camera === xrCam.cameras[0];
            const tex = isLeft ? this.videoLeftTexture : this.videoRightTexture;
            const mat = sphere.material;
            if (mat instanceof THREE.ShaderMaterial) {
                mat.uniforms['uMap'].value = tex;
            } else if (mat instanceof THREE.MeshBasicMaterial) {
                mat.map = tex;
            }
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
            onStop: () => this.stopVideo(),
            onPrev: () => this.mediaListService.playPreviousVideo(),
            onNext: () => this.mediaListService.playNextVideo(),
            onMoneyShot: () => this.websocketService.sendSkipToMoneyShot(),
            onNextAction: () => this.websocketService.sendSkipToNextAction(),
            onDevicePause: () => this.websocketService.togglePauseAllDeviceActions(),
            onSeek: (fraction) => this.onSeekFraction(fraction),
            onVolume: (v) => this.videoService.onSetVolume(v),
            onMute: () => this.videoService.toggleMute(),
            onRate: () => this.cyclePlayRate(),
            onSetProjection: (v) => this.setProjection(v),
            onSetStereo: (v) => this.setStereo(v),
            onTogglePanel: (id) => this.togglePanel(id),
            onPin: () => this.togglePin(),
            onExit: () => this.exitVR()
        });
        // Default panel layout: every panel sits on the SAME arc (radius 5.0m)
        // around the user, all at eye height and facing the user (aligned to
        // the skybox). The left-to-right order is preserved from the previous
        // layout: TCode, Media, Controls, Help, Settings, Script, Profiles.
        // The wide arc keeps the (wide) panels from overlapping each other.
        // The controls panel is pulled closer (radius 4.0) and lowered (y 1.3)
        // so it sits "under" / in front of the media wall as the primary
        // interaction surface.
        this.placeOnArc(this.mainPanel, -25, 4.0, 1.3);
        // Hidden until button 4 is pressed (no auto-show, no idle timeout).
        this.mainPanel.visible = false;
        this.worldRoot.add(this.mainPanel);
        this.panels.push(this.mainPanel);

        this.mediaWall = new MediaWallPanel(item => this.startVideo(item));
        this.placeOnArc(this.mediaWall, -55, 5.0, 1.5);
        this.worldRoot.add(this.mediaWall);
        this.panels.push(this.mediaWall);

        this.scriptPanel = new ScriptStatusPanel();
        this.placeOnArc(this.scriptPanel, 55, 5.0, 1.5);
        this.scriptPanel.visible = false;
        this.worldRoot.add(this.scriptPanel);
        this.panels.push(this.scriptPanel);

        this.profilesPanel = new MotionProfilesPanel(profile => this.selectProfile(profile));
        this.placeOnArc(this.profilesPanel, 75, 5.0, 1.5);
        this.profilesPanel.visible = false;
        this.worldRoot.add(this.profilesPanel);
        this.panels.push(this.profilesPanel);

        this.tcodePanel = new TCodePanel(command =>
            this.websocketService.sendTCode(command));
        this.placeOnArc(this.tcodePanel, -80, 5.0, 1.5);
        this.tcodePanel.visible = false;
        this.worldRoot.add(this.tcodePanel);
        this.panels.push(this.tcodePanel);

        this.settingsPanel = new SettingsPanel({
            onTogglePassthrough: () => this.togglePassthrough(),
            onStereoStrength: (v) => this.setStereoStrength(v),
            onResetStereo: () => this.setStereoStrength(1.0),
            onClearSettings: () => this.showClearSettingsDialog(),
            onSkyboxPitch: (v) => this.setSkyboxAxis('pitch', v),
            onSkyboxYaw: (v) => this.setSkyboxAxis('yaw', v),
            onSkyboxRoll: (v) => this.setSkyboxAxis('roll', v),
            onResetSkyboxPitch: () => this.resetSkyboxAxis('pitch'),
            onResetSkyboxYaw: () => this.resetSkyboxAxis('yaw'),
            onResetSkyboxRoll: () => this.resetSkyboxAxis('roll'),
            onZoom: (v) => { this.camZoom = v; },
            onHeight: (v) => { this.camHeave = -v; },
            onSide: (v) => { this.camSway = -v; },
            onResetPosition: () => { this.camZoom = 0; this.camHeave = 0; this.camSway = 0; }
        });
        this.placeOnArc(this.settingsPanel, 25, 5.0, 1.5);
        this.settingsPanel.visible = false;
        this.worldRoot.add(this.settingsPanel);
        this.panels.push(this.settingsPanel);

        // Help panel: the controller instructions. Hidden by default; open it
        // from the control panel's Help toggle, closed with its Close button.
        // Placed at radius 4.2 (z=-4.2) so it sits slightly IN FRONT of the 2D
        // video screen (z=-4.5) and is never hidden behind it.
        this.helpPanel = new HelpPanel(() => this.closeHelp());
        this.placeOnArc(this.helpPanel, 0, 4.2, 1.5);
        this.helpPanel.visible = false;
        this.worldRoot.add(this.helpPanel);
        this.panels.push(this.helpPanel);

        // Every panel's X button hides the panel; notify us so we can sync the
        // main-panel toggle buttons and persist the new layout. A panel closed
        // this way is also opted out of the Button-4 toggle (stays closed) -
        // EXCEPT the controls panel, which is the main UI and must always
        // toggle (otherwise the user loses access to the toggle buttons).
        for (const p of this.panels) {
            p.onClose = () => {
                if (p.panelId !== 'controls') {
                    this.userClosedPanels.add(p.panelId);
                }
                this.syncPanelToggleButtons();
                this.saveLayout();
            };
        }

        // Transient confirmation dialog for "Clear XR settings". Kept out of
        // this.panels so it is never persisted or toggled by the main panel.
        this.confirmDialog = new ConfirmDialog(
            'This will reset all XR settings to their defaults.',
            () => this.confirmClearSettings(),
            () => this.hideClearSettingsDialog()
        );
        this.confirmDialog.position.set(0, 1.5, -1.8);
        this.confirmDialog.visible = false;
        this.worldRoot.add(this.confirmDialog);

        this.syncPanelToggleButtons();
    }

    // Place a panel on an arc of the given radius around the user, at the given
    // angle (degrees) from the forward axis (left = negative, right = positive)
    // and height, facing the user (yaw) and kept upright. All panels sharing the
    // same radius sit at the same distance from the eyes, aligned to the skybox.
    private placeOnArc(panel: Panel, angleDeg: number, radius: number, y: number): void {
        const a = THREE.MathUtils.degToRad(angleDeg);
        panel.position.set(radius * Math.sin(a), y, -radius * Math.cos(a));
        panel.rotation.y = -a;
        panel.rotation.x = 0;
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
        // Apply the current passthrough state (persisted from the last session).
        this.applyPassthrough();
        // Reflect the (persisted) passthrough state on the Settings button so
        // it shows the right active/inactive state when the panel opens.
        this.settingsPanel?.setPassthroughActive(
            this.passthroughEnabled && this.passthroughAvailable);
        // The help panel is hidden by default (open it from the control
        // panel's Help toggle). A short status line remains so an empty line
        // is indistinguishable from a missing dom-overlay.
        const feats = (session as any).enabledFeatures as string[] | undefined;
        this.setStatus(
            'In VR. Press Button 4 for controls (Help toggle for instructions).' +
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
        const next = !this.passthroughEnabled;
        this.xrSettings.setPassthrough(next);
        this.setStatus(next
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
        this.xrSettings.setStereoStrength(THREE.MathUtils.clamp(value, 0, 2));
    }

    // ---- Skybox orientation (pitch / yaw / roll) -----------------------------
    // The 360 sphere's rotation is the skybox orientation. The Settings panel
    // exposes a slider + reset button per axis; dragging the skybox in VR
    // updates the same values (synced back to the sliders via syncSkyboxSettings).

    // Set one skybox axis from a slider value (degrees).
    private setSkyboxAxis(axis: 'pitch' | 'yaw' | 'roll', degrees: number): void {
        const rad = THREE.MathUtils.degToRad(degrees);
        if (axis === 'roll') {
            // Roll is applied to the camera in the render loop (heresphere-style).
            this.skyboxRoll = rad;
        } else {
            if (!this.videoSphere) return;
            if (axis === 'pitch') this.videoSphere.rotation.x = rad;
            else this.videoSphere.rotation.y = rad;
        }
        this.syncSkyboxSettings();
    }

    // Reset one skybox axis to 0 (default).
    private resetSkyboxAxis(axis: 'pitch' | 'yaw' | 'roll'): void {
        if (axis === 'roll') {
            this.skyboxRoll = 0;
        } else {
            if (!this.videoSphere) return;
            if (axis === 'pitch') this.videoSphere.rotation.x = 0;
            else this.videoSphere.rotation.y = 0;
        }
        this.syncSkyboxSettings();
    }

    // Push the current skybox angles (degrees) into the Settings panel sliders.
    // Roll lives on the camera (not the sphere), so it must sync even when no
    // video sphere exists yet.
    private syncSkyboxSettings(): void {
        if (!this.settingsPanel || !this.settingsPanel.visible) return;
        this.settingsPanel.setSkyboxAngles(
            THREE.MathUtils.radToDeg(this.videoSphere?.rotation.x ?? 0),
            THREE.MathUtils.radToDeg(this.videoSphere?.rotation.y ?? 0),
            THREE.MathUtils.radToDeg(this.skyboxRoll)
        );
    }

    private onSessionEnd(): void {
        this.sessionActive$.next(false);
        if (this.canvas) this.canvas.style.visibility = '';
        const hadVideo = this.videoService.isMediaItemPlaying();
        this.stopPlaybackForExit();
        if (!hadVideo) {
            // Session ended without playback - say so, so an unexpected
            // early exit is visible on the overlay.
            this.setStatus('VR session ended.', false);
        }
    }

    // When the VR session ends, STOP playback and return the page to the state
    // it was in when XR launched (no video playing, page player hidden). The
    // video is NOT handed back to the web player - the old handoff left the
    // clip audible/visible in the background after exit.
    private stopPlaybackForExit(): void {
        if (this.videoEl) {
            this.videoEl.pause();
            // Mute the element directly: its audio is routed through a WebAudio
            // MediaElementSourceNode, and in the Quest browser pausing/clearing
            // the element does not always silence that graph - muting the source
            // element guarantees no background audio.
            this.videoEl.muted = true;
            this.videoEl.volume = 0;
            // Fully stop and HIDE the element so it is neither visible nor
            // audible on the web page after exit.
            this.videoEl.removeAttribute('src');
            this.videoEl.load();
            this.videoEl.style.display = 'none';
        }
        // Silence the spatial-audio node as well (belt and suspenders).
        if (this.audio) this.audio.setVolume(0);
        this.stopStateTimer();
        this.hideVideoMeshes();
        // Point the video service back at the page player (so future web
        // playback works) but do NOT resume it.
        if (this.previousVideoNode) {
            this.videoService.setVideoNode(this.previousVideoNode);
        }
        if (this.videoService.isMediaItemPlaying()) {
            this.videoService.clearVideo();
            this.videoService.hideVideo();
        }
        this.setStatus('VR session ended.', false);
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
        this.videoDiagShown = false;

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
        // Re-show our element (it is hidden after an XR exit) so the Quest
        // compositor produces frames for it.
        this.videoEl.style.display = '';

        // Explicitly start playback: the autoplay attribute's failure is
        // silent (no promise, no event), so if the browser blocks it the
        // video would just sit there. play() gives us a catchable rejection.
        this.videoEl.play().catch(e => {
            this.setStatus('Video playback was blocked: ' + this.errorMessage(e) +
                '. Pull the trigger on a media item again.', false);
        });

        // DeoVR-style flags in the file name pick the projection
        // (case-insensitive, any order): equirect (_180/_360) and fisheye
        // (_fisheye/_fisheye190/_mkx200/_vrca220) select a sphere mesh; a
        // stereo flag (SBS/LR/3DH or TB/3DV/OverUnder) crops one eye per
        // texture. The "360" tag is a fallback. A manual override (the
        // Format/Stereo buttons) wins over detection for mis-named files.
        // Each new video resets the override to Auto so detection is the
        // default; the user can re-apply a manual format per video.
        this.formatProjection = 'auto';
        this.formatStereo = 'auto';
        const flags = this.parseVideoFlags(item.name + ' ' + (item.relativePath || ''));
        this.videoMode = this.resolveProjection(flags, item);
        this.videoStereo = this.resolveStereo(flags);
        this.applyVideoFormat();
        this.mainPanel?.setFormat(this.formatProjection, this.formatStereo,
            this.projectionLabel(this.videoMode), this.resolvedStereoLabel());

        this.startStateTimer();
        this.setStatus(`Playing: ${item.name} [${this.projectionLabel(this.videoMode)}]` +
            (this.videoStereo ? ` (stereo ${this.videoStereo.toUpperCase()} 3D)` : ''));
    }

    // Apply the current videoMode/videoStereo to the textures and mesh.
    // Shared by startVideo (after detection) and the manual format override
    // (cycleProjection/cycleStereo), so both paths configure identically.
    private applyVideoFormat(): void {
        const mode = this.videoMode;
        const isEquirect360 = mode === 'equirect360';
        const texs = this.videoStereo
            ? [this.videoTexture, this.videoLeftTexture, this.videoRightTexture]
            : [this.videoTexture];
        for (const tex of texs) {
            if (!tex) continue;
            if (isEquirect360) {
                // Full equirect: image center faces the user (-z), image
                // left of center on the viewer's left.
                tex.wrapS = THREE.RepeatWrapping;
                tex.wrapT = THREE.ClampToEdgeWrapping;
                tex.repeat.set(-1, 1);
                tex.offset.set(1.25, 0);
            } else {
                tex.wrapS = THREE.ClampToEdgeWrapping;
                tex.wrapT = THREE.ClampToEdgeWrapping;
                tex.repeat.set(1, 1);
                tex.offset.set(0, 0);
            }
            // Anisotropic filtering keeps the sphere sharp at oblique angles.
            tex.anisotropy = this.renderer?.capabilities.getMaxAnisotropy() ?? 1;
            tex.needsUpdate = true;
        }
        // Set the fisheye shader's max half-FOV for this projection (the
        // material is shared, so update its uniform each time the format
        // changes).
        if (this.videoFisheyeMaterial) {
            this.videoFisheyeMaterial.uniforms['uThetaMax'].value = this.fisheyeThetaMax(mode);
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
    }

    // Resolve the projection: the manual override wins, otherwise the
    // detected flag (with the "360" tag as a fallback, then flat).
    private resolveProjection(flags: { projection: VideoProjection | null }, item: MediaItem): VideoProjection {
        if (this.formatProjection !== 'auto') return this.formatProjection;
        return flags.projection ??
            (item.metaData?.tags?.includes('360') === true ? 'equirect360' : 'flat');
    }

    // Resolve the stereo mode: the manual override wins, otherwise the
    // detected flag. 'mono' forces no stereo.
    private resolveStereo(flags: { stereo: 'sbs' | 'tb' | null }): 'sbs' | 'tb' | null {
        if (this.formatStereo === 'auto') return flags.stereo;
        if (this.formatStereo === 'mono') return null;
        return this.formatStereo;
    }

    // Set the manual projection override (from the Format combo box) and
    // re-apply it to the current video (no restart - the texture mapping and
    // mesh update in place).
    setProjection(value: string): void {
        if (!this.videoEl) return;
        this.formatProjection = value as VideoProjection | 'auto';
        this.reapplyFormat();
    }

    // Set the manual stereo override (from the Stereo combo box) and re-apply
    // it to the current video.
    setStereo(value: string): void {
        if (!this.videoEl) return;
        this.formatStereo = value as 'auto' | 'sbs' | 'tb' | 'mono';
        this.reapplyFormat();
    }

    // Re-derive the effective projection/stereo from the override (or the
    // detected flags when auto) and re-apply it to the current video.
    private reapplyFormat(): void {
        const item = this.videoService.playingMediaItem;
        if (!item || !this.videoEl) return;
        const flags = this.parseVideoFlags(item.name + ' ' + (item.relativePath || ''));
        this.videoMode = this.resolveProjection(flags, item);
        this.videoStereo = this.resolveStereo(flags);
        this.applyVideoFormat();
        this.mainPanel?.setFormat(this.formatProjection, this.formatStereo,
            this.projectionLabel(this.videoMode), this.resolvedStereoLabel());
        this.setStatus(`Format: ${this.projectionLabel(this.videoMode)} / ${this.stereoWord()}`);
    }

    private stereoWord(): string {
        if (this.formatStereo === 'auto') return 'Auto';
        if (this.formatStereo === 'mono') return 'Mono';
        return this.formatStereo.toUpperCase();
    }

    // The stereo layout actually resolved for the current clip (used to label
    // the "Auto" option, e.g. "Auto (SBS)").
    private resolvedStereoLabel(): string {
        if (this.videoStereo === 'sbs') return 'SBS';
        if (this.videoStereo === 'tb') return 'TB';
        return 'Mono';
    }

    // Parse DeoVR-style projection/stereo/alpha flags from a file name. Flags
    // are case-insensitive and may appear in any order, e.g.
    // "My_video_SBS_alpha_mkx200.mp4".
    //
    // Projection (first match wins, most specific first):
    //   _vrca220 / _mkx220  -> fisheye 220
    //   _mkx200             -> fisheye 200
    //   _fisheye190         -> fisheye 190 (Canon VR lens)
    //   _fisheye            -> fisheye 180
    //   _180                -> equirectangular 180
    //   _360                -> equirectangular 360
    // The number flags match 180/360 as standalone tokens (any separator, not
    // part of a longer number). The fisheye word flags are matched BEFORE the
    // bare number flags so "fisheye190" is not mistaken for an equirect.
    private parseVideoFlags(name: string):
        { projection: VideoProjection | null; stereo: 'sbs' | 'tb' | null; alpha: boolean } {
        const n = name.toLowerCase();
        let projection: VideoProjection | null = null;
        // The fisheye word flags use a look-ahead (?![0-9]) on the RIGHT so a
        // stereo suffix glued on (e.g. "mkx200tb", "fisheye190sbs") still
        // matches - the old ([^a-z0-9]|$) boundary rejected the letter and the
        // file fell back to flat/2D. The left boundary keeps "smkx200" out.
        if (/(^|[^a-z0-9])(vrca220|mkx220)(?![0-9])/.test(n)) projection = 'fisheye220';
        else if (/(^|[^a-z0-9])mkx200(?![0-9])/.test(n)) projection = 'fisheye200';
        else if (/(^|[^a-z0-9])fisheye190(?![0-9])/.test(n)) projection = 'fisheye190';
        else if (/(^|[^a-z0-9])fisheye(?![0-9])/.test(n)) projection = 'fisheye180';
        else if (/(^|[^a-z0-9])180(?![0-9])/.test(n)) projection = 'equirect180';
        else if (/(^|[^a-z0-9])360(?![0-9])/.test(n)) projection = 'equirect360';
        let stereo: 'sbs' | 'tb' | null = null;
        if (/(^|[^a-z0-9])(l-r|lr|3dh|sbs)([^a-z]|$)/.test(n)) stereo = 'sbs';
        else if (/(^|[^a-z0-9])(tb|3dv|overunder)([^a-z]|$)/.test(n)) stereo = 'tb';
        const alpha = /(^|[^a-z0-9])alpha([^a-z0-9]|$)/.test(n);
        return { projection, stereo, alpha };
    }

    // Max half-field-of-view (radians) for a fisheye projection, used by the
    // equidistant fisheye shader. 180 -> 90°, 190 -> 95°, 200 -> 100°,
    // 220 -> 110°.
    private fisheyeThetaMax(mode: VideoProjection): number {
        switch (mode) {
            case 'fisheye190': return 95 * Math.PI / 180;
            case 'fisheye200': return 100 * Math.PI / 180;
            case 'fisheye220': return 110 * Math.PI / 180;
            case 'fisheye180':
            default: return 90 * Math.PI / 180;
        }
    }

    // Human-readable label for the status line.
    private projectionLabel(mode: VideoProjection): string {
        switch (mode) {
            case 'flat': return '2D';
            case 'equirect180': return '180 equirect';
            case 'equirect360': return '360 equirect';
            case 'fisheye180': return '180 fisheye';
            case 'fisheye190': return '190 fisheye';
            case 'fisheye200': return '200 fisheye';
            case 'fisheye220': return '220 fisheye';
            default: return mode;
        }
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
        const isFisheye = this.videoMode.startsWith('fisheye');
        if (this.videoMode === 'equirect360' || isFisheye) {
            // 360 equirect: the full frame IS the equirect. Fisheye: each eye
            // is a square (SBS frame 2:1, TB frame 1:2, mono 1:1) - the same
            // per-eye crop as 360, so it shares this branch.
            if (isEye) {
                if (st === 'sbs') { w = vw / 2; h = vh; }
                else if (st === 'tb') { w = vw; h = vh / 2; }
                else { w = vw; h = vh; }
            } else { w = vw; h = vh; }
            if (isFisheye) {
                // The fisheye shader maps a SQUARE image (the fisheye circle
                // fills the square). Force a square canvas so the circle maps
                // correctly even when the per-eye frame is not exactly square
                // (a stretched canvas is what squished the TB fisheye).
                const s = Math.min(w, h);
                w = s; h = s;
            }
        } else if (this.videoMode === 'equirect180') {
            if (isEye) {
                // The per-eye canvas MUST be 4:1 (same as mono) so the sphere
                // UV mapping is identical for every stereo layout. The per-eye
                // frame is drawn into the right half (2:1) and stretched to
                // fill it, so a non-2:1 per-eye (e.g. a 4:1 eye from a 2:1
                // landscape TB frame) is normalized to the same 2:1 equirect
                // the sphere expects. Sizing the canvas to the eye's own aspect
                // (the old vh/2) made an 8:1 canvas that stretched the sphere
                // mapping and squished the TB image.
                if (st === 'sbs') { w = vw; h = vw / 4; }         // eye=vw/2 wide, canvas=4:1
                else if (st === 'tb') { w = vw * 2; h = vw / 2; } // eye=vw wide, canvas=4:1
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
        // Clear the canvases so a stale frame from a PREVIOUS video does not
        // linger on the skybox. A same-sized canvas is not cleared by the
        // resize above (only a dimension change wipes it), so the old video's
        // last frame would otherwise show until the copy loop catches up -
        // this is what left the previous video "stuck" on the sphere. The copy
        // loop fills them with the new video on the next frame.
        const clearCanvas = (c: HTMLCanvasElement | null) => {
            if (!c) return;
            const ctx = c.getContext('2d');
            if (ctx) { ctx.fillStyle = '#000'; ctx.fillRect(0, 0, c.width, c.height); }
        };
        if (this.videoStereo) {
            clearCanvas(this.videoLeftCanvas);
            clearCanvas(this.videoRightCanvas);
        } else {
            clearCanvas(this.videoCopyCanvas);
        }
        // A canvas resize leaves the GPU texture at the stale size until it is
        // re-uploaded - force it here so the FIRST frame after a video switch
        // is correct (this is what caused the black screen when switching
        // between fisheye videos).
        if (this.videoTexture) this.videoTexture.needsUpdate = true;
        if (this.videoLeftTexture) this.videoLeftTexture.needsUpdate = true;
        if (this.videoRightTexture) this.videoRightTexture.needsUpdate = true;
        if (this.videoMode !== 'flat') {
            const isFisheye = this.videoMode.startsWith('fisheye');
            if (isFisheye && this.videoFisheyeMaterial) {
                // Fisheye: the shader maps the equidistant fisheye frame onto
                // the sphere. Stereo: onBeforeRender swaps uMap per eye pass.
                this.videoFisheyeMaterial.uniforms['uMap'].value =
                    this.videoStereo ? this.videoLeftTexture : this.videoTexture;
                this.videoFisheyeMaterial.uniforms['uThetaMax'].value =
                    this.fisheyeThetaMax(this.videoMode);
                this.videoSphere!.material = this.videoFisheyeMaterial;
            } else {
                const m = this.videoSphereMaterial!;
                // Stereo: onBeforeRender swaps this per eye pass.
                m.map = this.videoStereo ? this.videoLeftTexture : this.videoTexture;
                // The material starts black (blank screen) - the color
                // MULTIPLIES the texture, so it must be white once a map is set.
                m.color.set(0xffffff);
                m.needsUpdate = true;
                this.videoSphere!.material = m;
            }
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
        if (panel.visible) {
            // Re-opening a panel opts it back into the Button-4 toggle (it now
            // toggles with the others until the user closes it again).
            this.userClosedPanels.delete(id);
        }
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
        // Toggle every panel EXCEPT TCode, Profiles and Script Status (those are
        // opened individually from the control panel's toggle row), and EXCEPT
        // any panel the user has manually closed - a closed panel stays closed
        // until the user re-opens it (which re-opts it into this toggle).
        for (const p of this.panels) {
            if (p.panelId === 'tcode' || p.panelId === 'profiles' || p.panelId === 'script') {
                continue;
            }
            if (this.userClosedPanels.has(p.panelId)) {
                continue;
            }
            p.visible = this.controlsVisible;
        }
        if (this.controlsVisible) this.markInteraction();
        this.syncPanelToggleButtons();
        this.saveLayout();
    }

    // Close the help panel (its Close button). Re-openable from the control
    // panel's Help toggle (togglePanel('help')). Closing it opts it out of the
    // Button-4 toggle (stays closed until re-opened), like the other panels.
    private closeHelp(): void {
        if (!this.helpPanel) return;
        this.helpPanel.visible = false;
        this.userClosedPanels.add('help');
        this.syncPanelToggleButtons();
        this.saveLayout();
    }

    // Select a motion profile: send the changeChannelProfile message (the same
    // one the web Motion tab sends), optimistically mark the button active, and
    // surface a status line so the click is confirmable in the headset. If the
    // status line appears but the profile doesn't change on the device, the
    // message is reaching the socket and the issue is downstream.
    private selectProfile(profile: string): void {
        this.websocketService.setSelectedProfile(profile);
        this.profilesPanel?.setSelected(profile);
        this.setStatus(`Profile: ${profile}`);
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
            this.laserLines.push(line);

            // Cursor dot. Transparent so it can be faded in/out together with
            // the laser line (see updateLaserVisibility) - a hard on/off dot
            // stayed visible after the laser had already faded away.
            const cursor = new THREE.Mesh(
                new THREE.SphereGeometry(0.012, 12, 8),
                new THREE.MeshBasicMaterial({ color: 0xffffff, transparent: true, opacity: 0 }));
            cursor.visible = false;
            controller.add(cursor);
            this.ud(controller).cursor = cursor;
            this.laserDots.push(cursor);

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
        // A trigger press lights the lasers for a short window (see
        // updateLaserVisibility) so the user can point at any open panel.
        this.lastTriggerAt = performance.now();
        const target = this.hovered[this.ud(controller).index!];
        if (target?.slider) {
            this.draggingSlider = { controller, slider: target.slider };
        } else if (target?.skybox && !this.rotatingSkybox) {
            // Trigger-and-rotate: hold the trigger and rotate the controller
            // to tilt the skybox. The controller's orientation delta (pitch,
            // yaw, roll) is applied to the skybox each frame (see render loop).
            this.rotatingSkybox = {
                controller,
                lastQuat: controller.quaternion.clone()
            };
        }
    }

    private onSelectEnd(controller: THREE.Object3D): void {
        this.markInteraction();
        if (this.draggingSlider?.controller === controller) {
            this.draggingSlider = null;
            return;
        }
        if (this.rotatingSkybox?.controller === controller) {
            this.rotatingSkybox = null;
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
        if (this.grabbing || this.rotatingSkybox) return;
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
    }

    // Fade the controller laser lines in/out. They stay lit while the control
    // panel or the media wall is open (so the user can point at them); when
    // both are closed they fade out to reduce clutter. A trigger press keeps
    // them lit for a short window (LASER_TRIGGER_MS) so the user can interact
    // with whatever is on screen, and an active slider drag / panel grab keeps
    // them lit too. The opacity is eased each frame for a smooth fade.
    private static readonly LASER_TRIGGER_MS = 2000;
    private static readonly LASER_OPACITY = 0.6;

    private updateLaserVisibility(dt: number): void {
        if (this.laserLines.length === 0) return;
        const now = performance.now();
        const panelsOpen =
            (this.mainPanel?.visible === true) || (this.mediaWall?.visible === true);
        const triggerRecent = now - this.lastTriggerAt < XrPlayerService.LASER_TRIGGER_MS;
        const interacting = this.draggingSlider !== null || this.grabbing !== null;
        const target = (panelsOpen || triggerRecent || interacting)
            ? XrPlayerService.LASER_OPACITY : 0;
        const ease = Math.min(1, dt * 8);
        for (const line of this.laserLines) {
            const mat = line.material as THREE.LineBasicMaterial;
            mat.opacity = THREE.MathUtils.lerp(mat.opacity, target, ease);
        }
        // Fade the cursor dots in lockstep with the laser lines so they hide
        // together (the dot's .visible is still gated on a raycast hit).
        for (const dot of this.laserDots) {
            const mat = dot.material as THREE.MeshBasicMaterial;
            mat.opacity = THREE.MathUtils.lerp(mat.opacity, target, ease);
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

    // ---- XR settings (via XrSettingsService) ---------------------------------
    // All XR settings (panel layout, passthrough, stereo strength) live in the
    // self-contained XrSettingsService, which namespaces its keys under "xtpXr"
    // so they never touch the web app's local settings. We subscribe to its
    // observables so any change (including a reset) is applied to the live scene.

    private subscribeToXrSettings(): void {
        this.subs.push(
            this.xrSettings.passthrough$.subscribe(v => {
                this.passthroughEnabled = v;
                this.applyPassthrough();
                this.settingsPanel?.setPassthroughActive(v && this.passthroughAvailable);
            }),
            this.xrSettings.stereoStrength$.subscribe(v => {
                this.stereoStrength = v;
                this.settingsPanel?.stereoSlider.setValue(v, false);
            }),
            this.xrSettings.layout$.subscribe(layout => this.applyLayout(layout))
        );
    }

    // Gather the current position/rotation/visibility of every panel.
    private collectLayout(): XrPanelLayout {
        const data: XrPanelLayout = {};
        for (const p of this.panels) {
            data[p.panelId] = {
                pos: [p.position.x, p.position.y, p.position.z],
                rotY: p.rotation.y,
                rotX: p.rotation.x,
                visible: p.visible
            };
        }
        return data;
    }

    private saveLayout(): void {
        this.xrSettings.setLayout(this.collectLayout());
    }

    // Apply a persisted layout. Panels with no entry fall back to their default
    // (captured in buildPanels), so a reset restores the defaults.
    private applyLayout(layout: XrPanelLayout): void {
        for (const p of this.panels) {
            const entry = layout[p.panelId] ?? this.defaultLayout[p.panelId];
            if (!entry) continue;
            if (Array.isArray(entry.pos) && entry.pos.length === 3) {
                p.position.set(entry.pos[0], entry.pos[1], entry.pos[2]);
            }
            p.rotation.y = entry.rotY;
            p.rotation.x = entry.rotX;
            p.visible = entry.visible;
        }
        this.syncPanelToggleButtons();
    }

    // ---- Clear XR settings ---------------------------------------------------
    // "Clear XR settings" opens a confirmation dialog. On Confirm, every XR
    // setting is reset to its default (only XR settings - the web app's local
    // settings are untouched). The subscriptions above re-apply the defaults to
    // the live scene (panels return to their default layout, passthrough off,
    // stereo strength 1.0).

    private showClearSettingsDialog(): void {
        if (!this.confirmDialog) return;
        this.recenterDialog();
        this.confirmDialog.visible = true;
    }

    private hideClearSettingsDialog(): void {
        if (this.confirmDialog) this.confirmDialog.visible = false;
    }

    private confirmClearSettings(): void {
        this.hideClearSettingsDialog();
        this.xrSettings.resetXrSettings();
        this.setStatus('XR settings cleared.');
    }

    // Place the confirmation dialog ~1.8m in front of the user, at their height,
    // facing them (yaw only, kept upright).
    private recenterDialog(): void {
        if (!this.confirmDialog) return;
        const head = this.camera.position;
        this.tempMatrix.identity().extractRotation(this.camera.matrixWorld);
        const dir = new THREE.Vector3(0, 0, -1).applyMatrix4(this.tempMatrix);
        dir.y = 0;
        if (dir.lengthSq() < 0.001) dir.set(0, 0, -1);
        dir.normalize();
        const worldPos = head.clone().addScaledVector(dir, 1.8);
        worldPos.y = head.y - 0.1;
        const dx = head.x - worldPos.x;
        const dz = head.z - worldPos.z;
        this.confirmDialog.rotation.y = Math.atan2(dx, dz);
        this.confirmDialog.rotation.x = 0;
        this.worldRoot.worldToLocal(worldPos);
        this.confirmDialog.position.copy(worldPos);
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
            } else if (this.skyboxRoll !== 0) {
                // Apply the skybox roll to the XR camera's per-eye MATRICES
                // (heresphere-style).  The renderer computes matrixWorld
                // directly from camera.matrix in xr.updateCamera() — it never
                // reads camera.quaternion — so we must multiply the roll into
                // the matrix itself.
                //
                // CRITICAL: use premultiply (WORLD frame) around the camera's
                // forward axis, NOT multiply (LOCAL frame).  multiply applies
                // the roll AFTER the headset's pitch/yaw, so the roll axis
                // tilts with the head — at extreme roll, head pitch couples
                // into the view (gimbal lock).  premultiply applies the roll
                // BEFORE the headset pose, keeping the roll axis locked to the
                // camera's actual forward direction in world space.
                const xrCam = this.renderer.xr.getCamera();
                for (const cam of xrCam.cameras) {
                    const forward = new THREE.Vector3(0, 0, -1)
                        .transformDirection(cam.matrix).normalize();
                    const rollM = new THREE.Matrix4().makeRotationAxis(forward, this.skyboxRoll);
                    cam.matrix.premultiply(rollM);
                }
            }

            // Apply camera position offsets (zoom/heave/sway) to the SKYBOX
            // (video sphere), NOT the camera.  Moving the camera would shift
            // the panels, controllers, and slider raycasts relative to the
            // user's head, causing the sliders to jump and the UI to drift.
            // Moving the skybox instead only shifts the video, keeping all UI
            // elements anchored to the head.
            if ((this.camZoom !== 0 || this.camHeave !== 0 || this.camSway !== 0) && this.videoSphere) {
                const xrCam = this.renderer.xr.getCamera();
                const cam = xrCam.cameras[0];
                const rot = new THREE.Matrix4().extractRotation(cam.matrix);
                const offset = new THREE.Vector3(
                    this.camSway, this.camHeave, this.camZoom
                ).applyMatrix4(rot);
                this.videoSphere.position.copy(offset);
            } else if (this.videoSphere) {
                this.videoSphere.position.set(0, 0, 0);
            }

            this.updateInteraction(dt);
            this.updateLaserVisibility(dt);

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

            // Skybox trigger-and-rotate: while the trigger is held on the
            // skybox, the controller's orientation delta (pitch, yaw, roll)
            // is applied to the skybox. This follows the controller's actual
            // rotation (not its position), so there's no surge/sway drift.
            if (this.rotatingSkybox && this.videoSphere) {
                const { controller, lastQuat } = this.rotatingSkybox;
                // Delta rotation from last frame's controller orientation to
                // the current one, in the controller's local frame.
                const deltaQuat = new THREE.Quaternion()
                    .copy(controller.quaternion)
                    .multiply(lastQuat.clone().invert());
                const euler = new THREE.Euler().setFromQuaternion(deltaQuat, 'YXZ');
                // Sensitivity: 1.0 = 1:1 tracking; lower = more damped.
                const sens = 1.0;
                this.videoSphere.rotation.x += euler.x * sens;
                this.videoSphere.rotation.y += euler.y * sens;
                this.skyboxRoll += euler.z * sens;
                // Store this frame's orientation as the new baseline.
                lastQuat.copy(controller.quaternion);
                this.syncSkyboxSettings();
            }

            // Copy the current video frame into the texture canvas (~30fps).
            // The Quest's XR path does not upload VideoTextures directly.
            // The canvas is sized to the video's NATIVE resolution (capped)
            // in setupVideoMeshes, before the first upload.
            {
                const v = this.videoEl;
                const cc = this.videoCopyCanvas;
                const cctx = this.videoCopyCtx;
                // Recovery: if the video should be playing (per the video
                // service) but the element is paused (e.g. a TB frame stalled
                // the decoder), nudge it back to playing so the skybox doesn't
                // stay black for the rest of the session.
                if (v && this.videoService.isPlaying && v.paused && v.videoWidth) {
                    v.play().catch(() => { /* ignore */ });
                }
                // Diagnostic: the service thinks a clip is playing but the
                // element has no frames (videoWidth 0). Surface the element
                // state on the headset so a broken load (unsupported codec,
                // network, etc.) is visible - this is the "only passthrough"
                // symptom on headsets that can't decode the clip.
                if (v && this.videoService.isPlaying && v.videoWidth === 0 &&
                    !this.videoDiagShown) {
                    this.videoDiagShown = true;
                    const ec = v.error ? v.error.code : 0;
                    this.setStatus(`Video not decoding (readyState=${v.readyState} ` +
                        `network=${v.networkState} err=${ec} paused=${v.paused}). ` +
                        'This headset may not support the clip\'s codec/format.', false);
                }
                try {
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
                            const is180 = this.videoMode === 'equirect180';
                            const isFisheye = this.videoMode.startsWith('fisheye');
                            const drawSphere = (ctx: CanvasRenderingContext2D,
                                sx: number, sy: number, sw: number, sh: number,
                                shiftFrac = 0) => {
                                const w = ctx.canvas.width, h = ctx.canvas.height;
                                ctx.fillStyle = '#000';
                                ctx.fillRect(0, 0, w, h);
                                if (isFisheye) {
                                    // Fisheye: letterbox the per-eye image into
                                    // the square canvas, preserving aspect, so
                                    // the fisheye circle maps correctly. A
                                    // non-square per-eye frame stretched into
                                    // the canvas is what squished the TB fisheye.
                                    const va = sw / sh;
                                    const ca = w / h;
                                    let dw: number, dh: number, dx: number, dy: number;
                                    if (va > ca) {
                                        dw = w; dh = w / va; dx = 0; dy = (h - dh) / 2;
                                    } else {
                                        dh = h; dw = h * va; dx = (w - dw) / 2; dy = 0;
                                    }
                                    ctx.drawImage(v, sx, sy, sw, sh, dx, dy, dw, dh);
                                } else if (!is180) {
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
                                    // Disabled for fisheye: the shader assumes
                                    // the fisheye center sits at the image
                                    // center, so panning the frame would shift
                                    // the whole projection off-axis.
                                    const boost = isFisheye
                                        ? 0
                                        : Math.max(0, this.stereoStrength - 1.0) * 0.06;
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
                } catch (e) {
                    // A bad canvas/texture state (e.g. a TB frame that throws in
                    // drawImage) can leave the skybox black for the rest of the
                    // session. Log it and re-setup the meshes to recover.
                    console.error('Video copy error:', e);
                    this.setupVideoMeshes();
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
                    const distH = Math.sqrt(dx * dx + dz * dz);
                    const targetYaw = Math.atan2(dx, dz);
                    // Pitch about the horizontal distance (not dz) so the panel
                    // faces the user at the correct angle. With the panel's YXZ
                    // rotation order this tilts up/down without any roll, keeping
                    // the top aligned with the skybox / world up.
                    const targetPitch = Math.atan2(-dy, distH);
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