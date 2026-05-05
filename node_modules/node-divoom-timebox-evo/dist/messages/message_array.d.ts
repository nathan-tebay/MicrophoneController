/// <reference types="node" />
import { TimeboxEvoMessage } from "./message";
export declare class TimeboxEvoMessageArray extends Array<TimeboxEvoMessage> {
    private constructor();
    static create(): TimeboxEvoMessageArray;
    asBinaryBuffer(): Buffer[];
}
