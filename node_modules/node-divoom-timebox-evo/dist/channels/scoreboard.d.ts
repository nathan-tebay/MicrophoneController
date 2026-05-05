import { TimeboxEvoRequest } from "../requests";
interface ScoreBoardOptions {
    red?: number;
    blue?: number;
}
export declare class ScoreBoardChannel extends TimeboxEvoRequest {
    private _opts;
    private _PACKAGE_PREFIX;
    constructor(opts?: ScoreBoardOptions);
    private _updateMessage;
    red: number;
    blue: number;
}
export {};
