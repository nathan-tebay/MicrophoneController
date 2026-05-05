import { TimeboxEvoMessageArray } from "./messages/message_array";
export declare class TimeboxEvoRequest {
    private _messages;
    push(msg: string): number;
    clear(): void;
    readonly messages: TimeboxEvoMessageArray;
}
