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

// A flat transparent label (no background). heightPx controls the canvas
// resolution (higher = crisper text, more memory).
export function makeLabel(text: string, widthM: number, heightM: number,
    color = '#ffffff', fontScale = 0.65, heightPx = 192): THREE.Mesh {
    const aspect = widthM / heightM;
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

// Draw a title at a FIXED font size, word-wrapped to fit the width and
// ellipsized if it exceeds maxLines. Unlike makeFaceTexture (which shrinks the
// font until it fits), this keeps every title the same size so long names stay
// legible.
export function makeTitleTexture(
    text: string,
    widthPx: number,
    heightPx: number,
    fontSizePx: number,
    color: string,
    maxLines: number
): THREE.CanvasTexture {
    const canvas = document.createElement('canvas');
    canvas.width = widthPx;
    canvas.height = heightPx;
    const ctx = canvas.getContext('2d')!;
    ctx.clearRect(0, 0, widthPx, heightPx);
    ctx.fillStyle = color;
    ctx.font = `bold ${fontSizePx}px sans-serif`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';

    const maxWidth = widthPx - 16;
    const lineHeight = Math.floor(fontSizePx * 1.2);

    // Word-wrap into lines that fit the width (hard-break over-long words).
    const words = text.split(/\s+/).filter(w => w.length > 0);
    const lines: string[] = [];
    let line = '';
    for (const word of words) {
        const test = line ? line + ' ' + word : word;
        if (ctx.measureText(test).width <= maxWidth) {
            line = test;
        } else {
            if (line) lines.push(line);
            if (ctx.measureText(word).width > maxWidth) {
                let chunk = '';
                for (const ch of word) {
                    if (ctx.measureText(chunk + ch).width > maxWidth) {
                        lines.push(chunk);
                        chunk = ch;
                    } else {
                        chunk += ch;
                    }
                }
                line = chunk;
            } else {
                line = word;
            }
        }
    }
    if (line) lines.push(line);

    // Ellipsize if it exceeds maxLines.
    if (lines.length > maxLines) {
        const kept = lines.slice(0, maxLines);
        let last = kept[maxLines - 1];
        while (last.length > 1 && ctx.measureText(last + '…').width > maxWidth) {
            last = last.slice(0, -1);
        }
        kept[maxLines - 1] = last.replace(/\s+$/, '') + '…';
        lines.length = 0;
        lines.push(...kept);
    }

    // Center vertically.
    const totalH = lines.length * lineHeight;
    const startY = (heightPx - totalH) / 2 + lineHeight / 2;
    lines.forEach((l, i) => {
        ctx.fillText(l, widthPx / 2, startY + i * lineHeight);
    });

    const texture = new THREE.CanvasTexture(canvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    return texture;
}

// A flat title label with a fixed font size, wrapped to maxLines and
// ellipsized beyond that.
export function makeTitleMesh(text: string, widthM: number, heightM: number,
    color = '#ffffff', maxLines = 2): THREE.Mesh {
    // High pixel density: the label is stretched to fill the 3D plane, so a
    // low-res canvas (e.g. 128px) gets upscaled and the text goes fuzzy.
    const heightPx = 384;
    const widthPx = Math.max(512, Math.floor(heightPx * (widthM / heightM)));
    const fontSizePx = Math.floor((heightPx / maxLines) * 0.95);
    const tex = makeTitleTexture(text, widthPx, heightPx, fontSizePx, color, maxLines);
    // Crisp text: the label is viewed near 1:1, so mipmapped minification
    // (the default) is what softens the glyphs. Linear filtering + anisotropy
    // keeps them sharp, especially at the media wall's ~31deg angle.
    tex.generateMipmaps = false;
    tex.minFilter = THREE.LinearFilter;
    tex.magFilter = THREE.LinearFilter;
    tex.anisotropy = 16;
    const mesh = new THREE.Mesh(
        new THREE.PlaneGeometry(widthM, heightM),
        new THREE.MeshBasicMaterial({
            map: tex,
            alphaTest: 0.5
        })
    );
    mesh.renderOrder = 10;
    return mesh;
}
// The background mesh is the "grab handle" (userData.panel points back here).
export class Panel extends THREE.Group {
    readonly panelId: string;
    readonly bg: THREE.Mesh;
    readonly content = new THREE.Group();
    private hovered = false;
    // Called after the panel's X button hides it, so the owning service can
    // sync the main-panel toggle buttons and persist the new layout.
    onClose?: () => void;

    constructor(id: string, title: string, widthM: number, heightM: number) {
        super();
        this.panelId = id;
        // YXZ Euler order: yaw (face the user horizontally) is applied before
        // pitch (tilt up/down). This keeps the panel facing the user while its
        // top stays aligned with world up (the skybox) - no roll/tilt while
        // dragging. The default XYZ order combined pitch+yaw into a roll.
        this.rotation.order = 'YXZ';

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

        const titleLabel = makeLabel(title, widthM * 0.9, heightM * 0.06, '#cdd6ea', 0.65);
        // Shifted left so it does not collide with the close (X) button.
        titleLabel.position.set(-widthM * 0.05, heightM / 2 - heightM * 0.05, 0.005);
        this.add(titleLabel);

        // Close (X) button, top-right: hides this panel. Styled to blend into
        // the panel - no background in the resting state (just a muted glyph),
        // with a subtle highlight on hover.
        const close = new Button3D('\u00d7', 0.12, 0.12, {
            bg: null,
            border: null,
            color: '#8b95ad',
            bgHover: 'rgba(90, 130, 220, 0.6)',
            borderHover: null,
            bgActive: 'rgba(90, 130, 220, 0.75)',
            borderActive: null
        });
        close.position.set(widthM / 2 - 0.1, heightM / 2 - 0.09, 0.02);
        close.onClick = () => {
            this.visible = false;
            this.onClose?.();
        };
        this.add(close);

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

// Per-state color overrides for a Button3D. Omitted values fall back to the
// default blue button colors; null explicitly removes the background/border.
export interface Button3DStyle {
    bg?: string | null;
    border?: string | null;
    color?: string;
    bgHover?: string | null;
    borderHover?: string | null;
    bgActive?: string | null;
    borderActive?: string | null;
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
    private style: Button3DStyle;

    constructor(label: string, widthM = 0.28, heightM = 0.09, style: Button3DStyle = {}) {
        super();
        this.label = label;
        this.widthM = widthM;
        this.heightM = heightM;
        this.style = style;
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
        let bg: string | null;
        let border: string | null;
        let color: string;
        if (!this.enabled) {
            bg = '#1c2436';
            border = null;
            color = '#7a869c';
        } else if (this.active) {
            bg = this.style.bgActive ?? '#3d6fd6';
            border = this.style.borderActive ?? 'rgba(180, 210, 255, 0.9)';
            color = this.style.color ?? '#ffffff';
        } else if (this.hovered) {
            bg = this.style.bgHover ?? '#2c3d63';
            border = this.style.borderHover ?? 'rgba(140, 180, 255, 0.9)';
            color = this.style.color ?? '#ffffff';
        } else {
            bg = this.style.bg ?? '#33456b';
            border = this.style.border ?? 'rgba(130, 170, 255, 0.85)';
            color = this.style.color ?? '#ffffff';
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

// A 3D combo box (dropdown). The face shows the current value plus a caret;
// clicking it opens a vertical list of options ABOVE the face. Selecting an
// option fires onValue and closes the list; clicking the face again (or the
// transparent backdrop behind the list) closes it. The backdrop makes the
// dropdown modal: while open, any click outside the options closes it.
//
// The list grows upward from the face, so it should be placed near the BOTTOM
// of its panel (the options have room to expand into the panel body).
export class ComboBox3D extends THREE.Group {
    onValue?: (value: string) => void;
    private button: Button3D;
    private dropdown: THREE.Group;
    private optionButtons: Button3D[] = [];
    private options: { value: string; label: string }[] = [];
    private value = '';
    private widthM: number;
    private heightM: number;
    private open = false;

    constructor(widthM = 0.6, heightM = 0.08, onValue?: (value: string) => void) {
        super();
        this.widthM = widthM;
        this.heightM = heightM;
        this.onValue = onValue;

        this.button = new Button3D('', widthM, heightM);
        this.button.onClick = () => this.toggle();
        this.add(this.button);

        this.dropdown = new THREE.Group();
        this.dropdown.visible = false;
        this.add(this.dropdown);

        // Modal backdrop: a fully-transparent button behind the options.
        // bg/border null + empty label => no pixels drawn, but the geometry is
        // still raycastable, so a click anywhere on it closes the list. Sized
        // to cover the options list (not the whole view) and with hover
        // disabled, so the laser pointer entering/leaving it does not rescale
        // or redraw the face (that constant rescale/redraw was the flicker).
        const backdrop = new Button3D('', 1.5, 1.0, { bg: null, border: null });
        backdrop.setHovered = () => { /* no hover effect: prevents flicker */ };
        backdrop.position.y = 0.4; // centered above the face, over the options
        backdrop.onClick = () => this.close();
        this.dropdown.add(backdrop);
    }

    setOptions(options: { value: string; label: string }[]): void {
        this.options = options;
        for (const btn of this.optionButtons) this.dropdown.remove(btn);
        this.optionButtons = [];
        const rowH = this.heightM * 0.78;
        options.forEach((opt, i) => {
            const btn = new Button3D(opt.label, this.widthM, rowH);
            btn.position.set(0, this.heightM + 0.015 + i * (rowH + 0.008), 0.01);
            btn.onClick = () => {
                this.value = opt.value;
                this.close();
                this.onValue?.(opt.value);
            };
            this.dropdown.add(btn);
            this.optionButtons.push(btn);
        });
        this.refresh();
    }

    setValue(value: string): void {
        this.value = value;
        this.refresh();
    }

    private refresh(): void {
        const opt = this.options.find(o => o.value === this.value);
        this.button.setLabel((opt ? opt.label : this.value) + ' \u25be');
    }

    toggle(): void {
        if (this.open) this.close();
        else this.openDropdown();
    }

    private openDropdown(): void {
        this.open = true;
        this.dropdown.visible = true;
        this.button.setActive(true);
    }

    close(): void {
        if (!this.open) return;
        this.open = false;
        this.dropdown.visible = false;
        this.button.setActive(false);
    }
}

// A 3D slider over [min, max] (default 0..1). Raycast hits are converted
// to values via setFromPoint.
export class Slider3D extends THREE.Group {
    value = 0;
    onValue?: (v: number) => void;
    enabled = true;
    private widthM: number;
    private min: number;
    private max: number;
    private track: THREE.Mesh;
    private fill: THREE.Mesh;
    private knob: THREE.Mesh;

    constructor(widthM = 1.0, onValue?: (v: number) => void, min = 0, max = 1) {
        super();
        this.widthM = widthM;
        this.onValue = onValue;
        this.min = min;
        this.max = max;

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
        // Anchor the fill at the track's LEFT edge so scaling grows it
        // rightward: translate puts the geometry's left edge at local x=0,
        // then position the mesh at the track's left edge (-widthM/2).
        fill.geometry.translate(widthM / 2, 0, 0);
        fill.position.x = -widthM / 2;
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
        this.value = THREE.MathUtils.clamp(v, this.min, this.max);
        const t = this.max > this.min ? (this.value - this.min) / (this.max - this.min) : 0;
        this.fill.scale.x = Math.max(t, 0.0001);
        this.knob.position.x = -this.widthM / 2 + t * this.widthM;
        if (emit && this.onValue) {
            this.onValue(this.value);
        }
    }

    // Convert a world-space raycast hit point to a slider value.
    setFromPoint(worldPoint: THREE.Vector3): void {
        const local = this.worldToLocal(worldPoint.clone());
        const t = (local.x + this.widthM / 2) / this.widthM;
        this.setValue(this.min + t * (this.max - this.min));
    }
}