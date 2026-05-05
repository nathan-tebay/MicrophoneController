import { TimeboxEvoRequest } from "../requests";
interface BrightnessOpts {
    brightness?: number;
    in_min?: number;
    in_max?: number;
}
export declare class BrightnessCommand extends TimeboxEvoRequest {
    private _opts;
    private _PACKAGE_PREFIX;
    constructor(opts?: BrightnessOpts);
    brightness: number;
    in_min: number;
    in_max: number;
    opts: BrightnessOpts;
    private _updateMessage;
}
export {};
