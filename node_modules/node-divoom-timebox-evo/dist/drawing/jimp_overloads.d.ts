/// <reference types="node" />
import { TimeboxEvoMessageArray } from "../messages/message_array";
import { TimeboxEvoMessage } from "../messages/message";
import Jimp from 'jimp';
export declare class JimpArray extends Array<DivoomJimpAnim | DivoomJimpStatic> {
    private constructor();
    static create(): JimpArray;
    private _animAsDivoomMessages;
    private _staticAsDivoomMessages;
    asDivoomMessages(): TimeboxEvoMessageArray;
    asBinaryBuffer(): Buffer[];
}
export declare class DivoomJimp extends Jimp {
    getColorsAndPixels(): {
        colors: string[];
        pixels: number[];
    };
    getPixelString(pixelArray: number[], nbColors: number): string;
    asBinaryBuffer(): Buffer[];
}
export declare class DivoomJimpAnim extends DivoomJimp {
    private _frame;
    private _delay;
    frame: number;
    delay: number;
    asDivoomMessage(): TimeboxEvoMessage;
}
export declare class DivoomJimpStatic extends DivoomJimp {
    asDivoomMessage(): TimeboxEvoMessage;
}
