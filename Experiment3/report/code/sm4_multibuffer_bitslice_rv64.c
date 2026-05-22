/*
 * RV64 vector-oriented bitslice entry.
 *
 * The core uses RVV vector registers as bitslice bit-planes. With LMUL=8 and
 * 64-bit elements this asks the compiler/runtime for four 64-bit lanes, giving
 * 256 bits of bitslice state per bit-plane.
 */
#define BS_BACKEND_RVV
#define BS_LANES 256
#define BS_VERSION "multi-buffer-bitslice-rv64-rvv"

#include "sm4_multibuffer_bitslice.c"
