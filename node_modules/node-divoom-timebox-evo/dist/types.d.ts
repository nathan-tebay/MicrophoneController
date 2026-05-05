import { TimeChannel, LightningChannel, VJEffectChannel, ScoreBoardChannel, CloudChannel, CustomChannel } from "./channels/exports";
import { DisplayAnimation, DisplayText } from "./drawing/exports";
import { BrightnessCommand, TempWeatherCommand } from "./commands/exports";
export declare enum TimeDisplayType {
    FullScreen = 0,
    Rainbow = 1,
    WithBox = 2,
    AnalogSquare = 3,
    FullScreenNegative = 4,
    AnalogRound = 5
}
export declare enum LightningType {
    PlainColor = 0,
    Love = 1,
    Plants = 2,
    NoMosquitto = 3,
    Sleeping = 4
}
export declare enum WeatherType {
    Clear = 1,
    CloudySky = 3,
    Thunderstorm = 5,
    Rain = 6,
    Snow = 8,
    Fog = 9
}
export declare enum VJEffectType {
    Sparkles = 0,
    Lava = 1,
    VerticalRainbowLines = 2,
    Drops = 3,
    RainbowSwirl = 4,
    CMYFade = 5,
    RainbowLava = 6,
    PastelPatterns = 7,
    CMYWave = 8,
    Fire = 9,
    Countdown = 10,
    PinkBlueFade = 11,
    RainbowPolygons = 12,
    PinkBlueWave = 13,
    RainbowCross = 14,
    RainbowShapes = 15
}
export declare const TIMEBOX_CONST: {
    TimeType: typeof TimeDisplayType;
    LightningType: typeof LightningType;
    WeatherType: typeof WeatherType;
    VJEffectType: typeof VJEffectType;
};
export declare type RequestTypes = CloudChannel | CustomChannel | LightningChannel | ScoreBoardChannel | TimeChannel | VJEffectChannel | DisplayAnimation | DisplayText | BrightnessCommand | TempWeatherCommand;
