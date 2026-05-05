import { TimeboxEvoRequest } from "../requests";
import { LightningType } from "../types";
import { ColorInput } from "@ctrl/tinycolor";
interface LightningOpts {
    type?: LightningType;
    color?: ColorInput;
    brightness?: number;
    power?: boolean;
}
export declare class LightningChannel extends TimeboxEvoRequest {
    private _opts;
    private _color;
    private _PACKAGE_PREFIX;
    private _PACKAGE_SUFFIX;
    constructor(opts?: LightningOpts);
    private _updateMessage;
    type: LightningType;
    color: ColorInput;
    power: boolean;
    brightness: number;
}
export {};
