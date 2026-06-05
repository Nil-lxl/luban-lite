# Low Complexity Subband Codec (SBC)
SBC is an audio coding system specially designed for Bluetooth AV applications to obtain
high-quality audio at medium bit rates, while having a low computational complexity.
SBC uses 4 or 8 subbands, an adaptive bit allocation algorithm, and simple adaptive block PCM quantizers.

## Overview
In order to support A2DP in Zephyr, we need to have the SBC encoder and decoder because it is mandatory for A2DP.
Android bluetooth maintain one SBC implementation and it's license is Apache2.0, so Zephyr can use it.

The codes of this repository are from Android bluetooth SBC https://android.googlesource.com/platform/packages/modules/Bluetooth/+/refs/heads/main/system/embdrv/sbc/
