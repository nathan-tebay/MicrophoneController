import { TimeboxEvoRequest } from "../requests";
import { ColorInput } from "@ctrl/tinycolor";
import { TimeboxEvoMessage } from "../messages/message";
interface DisplayTextOpts {
    text?: string;
    paletteFn?: Function;
    animFn?: Function;
}
export declare class DisplayText extends TimeboxEvoRequest {
    private _animFrame;
    private _opts;
    PALETTE_TEXT_ON_BACKGROUND(text?: ColorInput, background?: ColorInput): string[];
    PALETTE_BLACK_ON_CMY_RAINBOW(): string[];
    PALETTE_BLACK_ON_RAINBOW(): string[];
    ANIM_STATIC_BACKGROUND(i?: number): number[];
    ANIM_UNI_GRADIANT_BACKGROUND(i: number): number[];
    ANIM_HORIZONTAL_GRADIANT_BACKGROUND(frame: number): number[];
    ANIM_VERTICAL_GRADIANT_BACKGROUND(frame: number): number[];
    constructor(opts?: DisplayTextOpts);
    private _encodeText;
    private _updateMessage;
    getNextAnimationFrame(): TimeboxEvoMessage;
    paletteFn: Function;
    readonly colorPalette: string[];
    animFn: Function;
    readonly pixels: number[];
    readonly frame: number;
    text: string;
}
export {};
