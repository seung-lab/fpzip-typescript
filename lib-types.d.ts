import { Buffer } from "nbind/dist/shim";

export class NBindBase { free?(): void }

export class Fpzip extends NBindBase {
	/** Fpzip(Buffer); */
	constructor(p0: number[] | ArrayBuffer | DataView | Uint8Array | Buffer);

	/** uint64_t nbytes(); */
	nbytes(): number;

	/** uint64_t nvoxels(); */
	nvoxels(): number;

	/** uint64_t get_type(); */
	get_type(): number;

	/** void decompress(Buffer &); */
	decompress(p0: any): void;

	/** void dekempress(Buffer &); */
	dekempress(p0: any): void;
}
