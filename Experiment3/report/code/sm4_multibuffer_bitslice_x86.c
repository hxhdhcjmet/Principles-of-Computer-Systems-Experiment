/*
 * x86 host bitslice entry.
 *
 * This version uses AVX2 registers as bitslice bit-planes. Each bit-plane is
 * a 256-bit vector, so one SM4 round is evaluated for 256 independent CBC
 * streams in lockstep.
 */
#define BS_BACKEND_AVX2
#define BS_LANES 256
#define BS_VERSION "multi-buffer-bitslice-x86-avx2"

#include "sm4_multibuffer_bitslice.c"
