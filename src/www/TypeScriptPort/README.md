# TypeScriptPort

An Angular + TypeScript application for porting the raw JavaScript XTP web app.

## Setup

```bash
npm install
```

## Development Server

```bash
npm start
```

Then open http://localhost:4200

## Build for Production

```bash
npm run build
```

Output goes to `dist/typescript-port/browser/`

## Watch Mode

```bash
npm run watch
```

## Project Structure

```
TypeScriptPort/
├── src/
│   ├── app/
│   │   ├── app.component.ts      # Root component
│   │   ├── app.component.html
│   │   ├── app.component.css
│   │   └── app.config.ts         # App configuration
│   ├── main.ts                   # Bootstrap entry
│   ├── index.html                # HTML shell
│   └── styles.css                # Global styles
├── angular.json                  # Angular CLI config
├── tsconfig.json                 # Base TS config
├── tsconfig.app.json             # App TS config
└── package.json
```