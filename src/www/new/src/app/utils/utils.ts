export function scale(number: number, inMin: number, inMax: number, outMin: number, outMax: number): number {
    return (((number - inMin) * (outMax - outMin)) / (inMax - inMin)) + outMin;
}

export function getChannelColor(channel: string): string {
    switch (channel) {
        case 'L0': return 'red';
        case 'L1': return 'yellow';
        case 'L2': return 'orange';
        case 'R0': return 'blue';
        case 'R1': return 'green';
        case 'R2': return 'purple';
        case 'V0': return 'pink';
        case 'V1': return 'aliceblue';
        case 'A0': return 'cyan';
        case 'A1': return 'magenta';
        default: return 'white';
    }
}

export function lerp(startValue: number, targetValue: number, timeScale: number): number {
    return startValue + (targetValue - startValue) * timeScale;
}

export function millis(lastTime = 0): number {
    return Date.now() - lastTime;
}

export function round(value: number, precision?: number): number {
    const multiplier = Math.pow(10, precision || 0);
    return Math.round(value * multiplier) / multiplier;
}

export function capitalizeFirstLetter(val: string): string {
    return val.charAt(0).toUpperCase() + val.slice(1);
}

export function removeAllChildNodes(parent: HTMLElement): void {
    while (parent.firstChild) {
        parent.removeChild(parent.firstChild);
    }
}

export function isDirty(form: HTMLFormElement): boolean {
    for (let i = 0; i < form.elements.length; i++) {
        const field = form.elements[i] as HTMLInputElement | HTMLSelectElement;
        switch (field.type) {
            case 'select-multiple':
            case 'select-one': {
                const select = field as HTMLSelectElement;
                const options = select.options;
                for (let j = 0; j < options.length; j++) {
                    if (options[j].selected !== options[j].defaultSelected) return true;
                }
                break;
            }
            case 'text':
            case 'file':
            case 'password': {
                const input = field as HTMLInputElement;
                if (input.value !== input.defaultValue) return true;
                break;
            }
            case 'checkbox':
            case 'radio': {
                const input = field as HTMLInputElement;
                if (input.checked !== input.defaultChecked) return true;
                break;
            }
        }
    }
    return false;
}

export function parseCookie(str: string): { [key: string]: string } {
    return str
        .split(';')
        .map(v => v.split('='))
        .reduce((acc, v) => {
            if (v && v[0].trim().length)
                acc[decodeURIComponent(v[0].trim())] = decodeURIComponent(v[1].trim());
            return acc;
        }, {} as { [key: string]: string });
}

export function deleteCookie(name: string, path?: string, domain?: string): void {
    if (getCookie(name)) {
        document.cookie = name + '=' +
            ((path) ? ';path=' + path : '') +
            ((domain) ? ';domain=' + domain : '') +
            ';expires=Thu, 01 Jan 1970 00:00:01 GMT';
    }
}

export function getCookie(name: string): boolean {
    return document.cookie.split(';').some(c => {
        return c.trim().startsWith(name + '=');
    });
}