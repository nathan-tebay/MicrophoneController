import { TimeboxEvoRequest } from "../requests";
import { WeatherType } from "../types";
interface TempWeatherOpts {
    temperature?: number;
    weather?: WeatherType;
}
export declare class TempWeatherCommand extends TimeboxEvoRequest {
    private _PACKAGE_PREFIX;
    private _opts;
    constructor(opts?: TempWeatherOpts);
    temperature: number;
    weather: WeatherType;
    private _updateMessage;
}
export {};
