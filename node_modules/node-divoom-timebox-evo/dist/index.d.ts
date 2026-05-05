import { RequestTypes } from "./types";
import { TimeboxEvoRequest } from "./requests";
import { TimeChannel, LightningChannel, VJEffectChannel, ScoreBoardChannel, CloudChannel, CustomChannel } from "./channels/exports";
import { BrightnessCommand, TempWeatherCommand, DateTimeCommand } from "./commands/exports";
import { DisplayText } from "./drawing/text";
import { DisplayAnimation } from "./drawing/drawing";
export * from "./channels/exports";
export * from "./commands/exports";
export * from "./drawing/exports";
export * from "./requests";
export { TIMEBOX_CONST } from "./types";
export declare class TimeboxEvo {
    createRequest(type: Object): RequestTypes;
    createRequest(type: "cloud"): CloudChannel;
    createRequest(type: "custom"): CustomChannel;
    createRequest(type: "lightning"): LightningChannel;
    createRequest(type: "scoreboard"): ScoreBoardChannel;
    createRequest(type: "time"): TimeChannel;
    createRequest(type: "vjeffect" | "vj-effect"): VJEffectChannel;
    createRequest(type: "brightness"): BrightnessCommand;
    createRequest(type: "temp_weather"): TempWeatherCommand;
    createRequest(type: "text"): DisplayText;
    createRequest(type: "picture" | "animation"): DisplayAnimation;
    createRequest(type: "datetime"): DateTimeCommand;
    createRequest(type: "raw"): TimeboxEvoRequest;
}
