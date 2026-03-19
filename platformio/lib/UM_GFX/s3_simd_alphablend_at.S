//
// ESP32-S3 SIMD optimized code - positioned/strided alpha blending
// By Seon Rozenblum, Unexpected Maker for UM_GFX
//
// Based on s3_simd_alphablend.S by by Larry Bank, BitBank Software, Inc.
// https://github.com/bitbank2/BB_SPI_LCD
//

#if defined (ARDUINO_ARCH_ESP32) && !defined(NO_SIMD)
#if __has_include ("dsps_fft2r_platform.h")
#include "dsps_fft2r_platform.h"
#if (dsps_fft2r_sc16_aes3_enabled == 1)
	.text
	.align 4

    .type   s3_alpha_blend_at_be, @function
    .align 4
    .global s3_alpha_blend_at_be

# Alpha blend RGB565 big-endian pixels with stride support for positioned sprites
# The alpha value can be from 0 to 32
# Width must be a multiple of 8
# Handles row iteration internally - blends entire sprite in one call
#
#                                          A2            A3              A4              A5              A6            A7
# Call as void s3_alpha_blend_at_be(uint16_t *pFG, uint16_t *pDest, uint32_t fg_pitch, uint32_t dest_pitch, uint32_t width, uint32_t height,
#                                   uint8_t alpha, const uint16_t *pMasks);
#                                   stack[0]       stack[1]
  s3_alpha_blend_at_be:
    .align 4
    entry    a1,32

    # Load stack parameters
    l32i.n   a8,a1,32         # alpha (7th param)
    l32i.n   a9,a1,36         # pMasks (8th param)

    # Convert pitches from pixels to bytes (multiply by 2)
    slli     a4,a4,1          # fg_pitch in bytes
    slli     a5,a5,1          # dest_pitch in bytes

    # Make the alpha value as a pair of 16-bit values for later
    slli     a10,a8,16
    or       a10,a10,a8       # now we have the alpha in 16-bit slots

    ee.movi.32.q q5,a10,0     # set up alpha value in 8 slots of Q5
    ee.movi.32.q q5,a10,1
    ee.movi.32.q q5,a10,2
    ee.movi.32.q q5,a10,3

    movi.n   a10,32           # inverted alpha for destination
    sub      a8,a10,a8
    slli     a10,a8,16        # prepare it for 16-bit slots
    or       a10,a10,a8
    ee.movi.32.q q6,a10,0     # set up inverted alpha in 8 slots of Q6
    ee.movi.32.q q6,a10,1
    ee.movi.32.q q6,a10,2
    ee.movi.32.q q6,a10,3

    movi.n   a10,0            # prepare shift amounts
    movi.n   a11,5

.row_loop_at:
    # Save row start pointers
    mov      a12,a2           # fg row start
    mov      a13,a3           # dest row start
    mov      a14,a6           # width counter for this row

.pixel_loop_at:
    wsr.sar  a10              # SAR (shift amount register) = 0

    ee.vld.128.ip q1,a2,16    # load 8 RGB565 src pixels into q1
    ee.vld.128.ip q2,a3,0     # load 8 RGB565 dest pixels into q2 (no increment - will write back)

    mv.qr    q7,q1            # big endian data, need to swap the byte order
    ee.vunzip.8 q7,q1         # swap the byte order to be big-endian
    ee.vzip.8 q1,q7

    mv.qr    q7,q2
    ee.vunzip.8 q7,q2
    ee.vzip.8 q2,q7

    ee.vldbc.16.ip q0,a9,2    # load the blue mask into all 8 slots
    ee.andq  q3,q1,q0         # apply the mask to all pixels
    ee.andq  q4,q2,q0
    ee.vmul.u16 q3,q3,q5      # multiply source pixels by source alpha
    ee.vmul.u16 q4,q4,q6      # multiply dest pixels by dest alpha
    ee.vadds.s16 q7,q3,q4     # combine src+dest blue
    wsr.sar  a11              # set up right shift by 5
    ee.vsr.32 q7,q7           # shift blue back to normal position
    ee.andq  q7,q7,q0

    ee.vldbc.16.ip q0,a9,2    # load the green mask
    ee.andq  q3,q1,q0
    ee.andq  q4,q2,q0
    ee.vmul.u16 q3,q3,q5      # mult green by src+dest alpha
    ee.vmul.u16 q4,q4,q6      # leave the right shift of 5
    ee.vadds.s16 q4,q3,q4     # combine src+dest green
    ee.andq  q4,q4,q0
    ee.orq   q7,q7,q4         # combine green+blue

    ee.vldbc.16.ip q0,a9,2    # load red mask for arith shift problem
    ee.vsr.32 q1,q1           # shift red down 5 bits to not overflow
    ee.vsr.32 q2,q2
    ee.andq  q1,q1,q0         # mask at the new position
    ee.andq  q2,q2,q0
    ee.vmul.u16 q1,q1,q5      # mult red by src+dest alpha
    ee.vmul.u16 q2,q2,q6
    ee.vadds.s16 q1,q1,q2     # combine src+dst red
    ee.vsl.32 q1,q1           # now shift left 5 to fix the position
    ee.vldbc.16 q0,a9
    ee.andq  q1,q1,q0
    ee.orq   q1,q7,q1         # combine R + G + B

    # we now have 16-bit slots with the results we want
    mv.qr    q7,q1            # reverse byte order from little to big endian
    ee.vunzip.8 q7,q1
    ee.vzip.8 q1,q7

    ee.vst.128.ip q1,a3,16    # store 8 finished destination pixels
    addi.n   a9,a9,-6         # reset color masks pointer
    addi.n   a14,a14,-8       # decrement pixel count by 8
    bnez.n   a14,.pixel_loop_at

    # Advance to next row using pitch (already in bytes)
    add      a2,a12,a4        # next fg row = row_start + fg_pitch
    add      a3,a13,a5        # next dest row = row_start + dest_pitch

    addi.n   a7,a7,-1         # decrement height (row count)
    bnez.n   a7,.row_loop_at

    retw.n

#endif // dsps_fft2r_sc16_aes3_enabled
#endif // __has_include
#endif // ESP32
