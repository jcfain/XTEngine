// Source - https://stackoverflow.com/a/49279951
// Posted by Lerner, modified by community. See post 'Timeline' for change history
// Retrieved 2026-10-10, License - CC BY-SA 4.0

import { HttpInterceptorFn, HttpResponse } from '@angular/common/http';
import { tap } from 'rxjs';

const dateRegex =
    /^\d{4}(-\d{2}){2}T\d{2}:\d{2}(?::\d{2}(?:\.\d{1,7}(?:\+\d{2}:\d{2})?)?)?$/;

function convertDates(
        object: unknown,
        parent?: Record<string, unknown> | unknown[],
        key?: number | string,
    ) {
    if (object === null || object === undefined) return;
        if (typeof object === 'string') {
        if (dateRegex.test(object)) {
                /**
                 * @see https://stackoverflow.com/a/54733846/1306679
                 */
                const dateValue = object.replace(/-/g, '/').replace('T', ' ');
                // @ts-ignore
                parent[key] = new Date(dateValue);
            }
        } else if (Array.isArray(object)) {
            for (let i = 0; i < object.length; i++)
            convertDates(object[i], object, i);
    } else if (typeof object === 'object') {
            for (const key of Object.keys(object as Record<string, unknown>)) {
            convertDates(
                (object as Record<string, unknown>)[key],
                object as Record<string, unknown>,
                key,
                );
            }
        }
    }

export const apiInterceptor: HttpInterceptorFn = (request, next) => {
    return next(request).pipe(
        tap((event) => {
            if (event instanceof HttpResponse) {
                convertDates(event.body);
}
        })
    );
};

