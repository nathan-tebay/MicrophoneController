import { TimeboxEvoRequest } from "../requests";
import { TimeDisplayType } from "../types";
import { ColorInput } from "@ctrl/tinycolor";
interface TimeOptions {
    type?: TimeDisplayType;
    color?: ColorInput;
    showTime?: boolean;
    showWeather?: boolean;
    showTemp?: boolean;
    showCalendar?: boolean;
}
export declare class TimeChannel extends TimeboxEvoRequest {
    private _opts;
    private _color;
    private _PACKAGE_PREFIX;
    constructor(opts?: TimeOptions);
    private _updateMessage;
    type: TimeDisplayType;
    color: ColorInput;
    showTime: boolean;
    showWeather: boolean;
    showTemp: boolean;
    showCalendar: boolean;
}
export {};
