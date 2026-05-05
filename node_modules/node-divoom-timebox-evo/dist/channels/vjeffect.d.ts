import { TimeboxEvoRequest } from "../requests";
import { VJEffectType } from "../types";
interface VJEffectOptions {
    type?: VJEffectType;
}
export declare class VJEffectChannel extends TimeboxEvoRequest {
    private _opts;
    private _PACKAGE_PREFIX;
    constructor(opts?: VJEffectOptions);
    private _updateMessage;
    type: number;
}
export {};
