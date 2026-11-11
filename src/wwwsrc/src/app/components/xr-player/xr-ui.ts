// 3D UI primitives for the WebXR media player.
//
// This file is part of the self-contained xr-player feature: it has no
// dependencies on the rest of the app (only on three.js) so the whole
// feature can be removed by deleting the xr-player folder.
import * as THREE from 'three';

// Draw text (optionally on a rounded-rect button face) to a canvas and
// return it as a texture.
export function makeFaceTexture(
    text: string,
    opts: {
        width?: number;
        height?: number;
        font?: string;
        color?: string;
        bg?: string | null;
        border?: string | null;
        rounded?: number;
    } = {}
): THREE.CanvasTexture {
    const width = opts.width ?? 512;
    const height = opts.height ?? 128;
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext('2d')!;
    ctx.clearRect(0, 0, width, height);

    if (opts.bg) {
        const r = opts.rounded ?? 24;
        ctx.fillStyle = opts.bg;
        ctx.beginPath();
        ctx.roundRect(4, 4, width - 8, height - 8, r);
        ctx.fill();
        if (opts.border) {
            ctx.strokeStyle = opts.border;
            ctx.lineWidth = 6;
            ctx.stroke();
        }
    }

    if (text) {
        ctx.fillStyle = opts.color ?? '#ffffff';
        ctx.font = opts.font ?? `bold ${Math.floor(height * 0.42)}px sans-serif`;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        // Shrink the font until the text fits the face.
        let fontSize = Math.floor(height * 0.42);
        while (fontSize > 12 && ctx.measureText(text).width > width - 40) {
            fontSize -= 2;
            ctx.font = `bold ${fontSize}px sans-serif`;
        }
        ctx.fillText(text, width / 2, height / 2 + 2);
    }

    const texture = new THREE.CanvasTexture(canvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    return texture;
}

// A flat transparent label (no background).
export function makeLabel(text: string, widthM: number, heightM: number,
    color = '#ffffff', fontScale = 0.5): THREE.Mesh {
    const aspect = widthM / heightM;
    const heightPx = 128;
    const mesh = new THREE.Mesh(
        new THREE.PlaneGeometry(widthM, heightM),
        new THREE.MeshBasicMaterial({
            map: makeFaceTexture(text, {
                width: Math.max(256, Math.floor(heightPx * aspect)),
                height: heightPx,
                color,
                font: `bold ${Math.floor(heightPx * fontScale)}px sans-serif`
            }),
            // Opaque with alpha cutoff (NOT transparent blending): the Quest's
            // XR render path drops transparent content that sits in front of
            // the panel background. alphaTest keeps the material in the
            // opaque pass, so the depth buffer alone handles layering.
            alphaTest: 0.5
        })
    );
    mesh.renderOrder = 10;
    return mesh;
}

// A draggable/toggleable panel: semi-transparent rounded background + title.
// The background mesh is the "grab handle" (userData.panel points back here).
export class Panel extends THREE.Group {
    readonly panelId: string;
    readonly bg: THREE.Mesh;
    readonly content = new THREE.Group();
    private hovered = false;

    constructor(id: string, title: string, widthM: number, heightM: number) {
        super();
        this.panelId = id;

        const bg = new THREE.Mesh(
            new THREE.PlaneGeometry(widthM, heightM),
            new THREE.MeshBasicMaterial({
                map: this.makeBgTexture(widthM, heightM, false),
                // Opaque with alpha cutoff (not transparent blending) so the
                // depth buffer, not transparency sorting, layers content.
                alphaTest: 0.5
            })
        );
        bg.userData['panel'] = this;
        this.bg = bg;
        this.add(bg);

        const titleLabel = makeLabel(title, widthM * 0.9, heightM * 0.09);
        titleLabel.position.set(0, heightM / 2 - heightM * 0.06, 0.005);
        this.add(titleLabel);

        this.content.position.set(0, 0, 0.01);
        this.add(this.content);
    }

    private makeBgTexture(widthM: number, heightM: number, hovered: boolean): THREE.CanvasTexture {
        const aspect = widthM / heightM;
        const heightPx = 512;
        return makeFaceTexture('', {
            width: Math.max(256, Math.floor(heightPx * aspect)),
            height: heightPx,
            bg: 'rgba(10, 15, 28, 0.88)',
            border: hovered ? 'rgba(140, 180, 255, 0.9)' : 'rgba(90, 130, 220, 0.55)',
            rounded: 32
        });
    }

    setHovered(v: boolean): void {
        if (this.hovered === v) return;
        this.hovered = v;
        const target = v ? 1.03 : 1.0;
        this.scale.setScalar(THREE.MathUtils.lerp(this.scale.x, target, 0.4));
        const mat = this.bg.material as THREE.MeshBasicMaterial;
        const old = mat.map;
        mat.map = this.makeBgTexture(
            (this.bg.geometry as THREE.PlaneGeometry).parameters.width,
            (this.bg.geometry as THREE.PlaneGeometry).parameters.height,
            v);
        old?.dispose();
    }
}

// A 3D push button with hover/pressed/active states.
export class Button3D extends THREE.Group {
    label = '';
    onClick?: () => void;
    enabled = true;
    private active = false;
    private hovered = false;
    private face: THREE.Mesh;
    private widthM: number;
    private heightM: number;

    constructor(label: string, widthM = 0.28, heightM = 0.09) {
        super();
        this.label = label;
        this.widthM = widthM;
        this.heightM = heightM;
        this.face = new THREE.Mesh(
            new THREE.PlaneGeometry(widthM, heightM),
            new THREE.MeshBasicMaterial({ alphaTest: 0.5 })
        );
        this.face.renderOrder = 10;
        this.face.userData['button'] = this;
        this.add(this.face);
        this.refresh();
    }

    get isActive(): boolean { return this.active; }

    setLabel(text: string): void {
        this.label = text;
        this.refresh();
    }

    setActive(v: boolean): void {
        if (this.active === v) return;
        this.active = v;
        this.refresh();
    }

    setEnabled(v: boolean): void {
        this.enabled = v;
        this.refresh();
    }

    setHovered(v: boolean): void {
        this.hovered = v;
        const target = (v && this.enabled) ? 1.08 : 1.0;
        this.scale.setScalar(THREE.MathUtils.lerp(this.scale.x, target, 0.5));
        this.refresh();
    }

    private refresh(): void {
        let bg = '#33456b';
        let border: string | null = 'rgba(130, 170, 255, 0.85)';
        let color = '#ffffff';
        if (!this.enabled) {
            bg = '#1c2436';
            border = null;
            color = '#7a869c';
        } else if (this.active) {
            bg = '#3d6fd6';
            border = 'rgba(180, 210, 255, 0.9)';
        } else if (this.hovered) {
            bg = '#2c3d63';
            border = 'rgba(140, 180, 255, 0.9)';
        }
        const aspect = this.widthM / this.heightM;
        const heightPx = 96;
        const mat = this.face.material as THREE.MeshBasicMaterial;
        const old = mat.map;
        mat.map = makeFaceTexture(this.label, {
            width: Math.max(128, Math.floor(heightPx * aspect)),
            height: heightPx,
            bg,
            border,
            color,
            rounded: 20
        });
        old?.dispose();
    }
}

// A 3D slider (0..1). Raycast hits are converted to values via setFromPoint.
export class Slider3D extends THREE.Group {
    value = 0;
    onValue?: (v: number) => void;
    enabled = true;
    private widthM: number;
    private track: THREE.Mesh;
    private fill: THREE.Mesh;
    private knob: THREE.Mesh;

    constructor(widthM = 1.0, onValue?: (v: number) => void) {
        super();
        this.widthM = widthM;
        this.onValue = onValue;

        const track = new THREE.Mesh(
            new THREE.PlaneGeometry(widthM, 0.025),
            new THREE.MeshBasicMaterial({ color: 0x3a4a6e })
        );
        track.renderOrder = 10;
        track.userData['slider'] = this;
        this.track = track;
        this.add(track);

        const fill = new THREE.Mesh(
            new THREE.PlaneGeometry(widthM, 0.025),
            new THREE.MeshBasicMaterial({ color: 0x4d8dff })
        );
        fill.renderOrder = 11;
        // Anchor the fill at the left edge so scaling grows it rightward.
        fill.geometry.translate(widthM / 2, 0, 0);
        fill.position.z = 0.002;
        this.fill = fill;
        this.add(fill);

        const knob = new THREE.Mesh(
            new THREE.SphereGeometry(0.028, 16, 12),
            new THREE.MeshBasicMaterial({ color: 0xdfe9ff })
        );
        knob.renderOrder = 12;
        knob.userData['slider'] = this;
        this.knob = knob;
        this.add(knob);

        this.setValue(0, false);
    }

    setValue(v: number, emit = true): void {
        this.value = THREE.MathUtils.clamp(v, 0, 1);
        this.fill.scale.x = Math.max(this.value, 0.0001);
        this.knob.position.x = -this.widthM / 2 + this.value * this.widthM;
        if (emit && this.onValue) {
            this.onValue(this.value);
        }
    }

    // Convert a world-space raycast hit point to a slider value.
    setFromPoint(worldPoint: THREE.Vector3): void {
        const local = this.worldToLocal(worldPoint.clone());
        this.setValue((local.x + this.widthM / 2) / this.widthM);
    }
}