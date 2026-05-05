/// <reference types="node" />
export declare class TimeboxEvoMessage {
    private _START;
    private _END;
    private _message;
    constructor(msg?: string);
    private _calcCRC;
    readonly crc: number | undefined;
    readonly crcHS: string | undefined;
    readonly length: number | undefined;
    readonly lengthHS: string | undefined;
    payload: string;
    readonly message: string | undefined;
    append(msg: string): TimeboxEvoMessage;
    prepend(msg: string): TimeboxEvoMessage;
    toString(): string | undefined;
    asBinaryBuffer(): Buffer[];
}
