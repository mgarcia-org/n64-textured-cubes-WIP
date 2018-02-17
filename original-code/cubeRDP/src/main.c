//
// cubeRDP/src/main.c: RDP test cart (entry point).
//
// n64chain: A (free) open-source N64 development toolchain.
// Copyright 2014-16 Tyler J. Stachecki <stachecki.tyler@gmail.com>
//
// This file is subject to the terms and conditions defined in
// 'LICENSE', which is part of this source code package.
//

#include <rcp/vi.h>
#include <stdint.h>
#include <syscall.h>
#include "rdp.c"
#include "3d.c"
#include "3dscene.c"

// These pre-defined values are suitable for NTSC.
// TODO: Add support for PAL and PAL-M televisions.
static vi_state_t vi_state = {
  0x0000324E, // status
  0x00200000, // origin
  0x00000140, // width
  0x00000002, // intr
  0x00000000, // current
  0x03E52239, // burst
  0x0000020D, // v_sync
  0x00000C15, // h_sync
  0x0C150C15, // leap
  0x006C02EC, // h_start
  0x002501FF, // v_start
  0x000E0204, // v_burst
  0x00000200, // x_scale
  0x00000400, // y_scale
};

void main(void *unused __attribute__((unused))) {
  // Register VI interrupts on this thread. When registering a thread w/
  // interrupts, it causes the threads message queue to get populated w/
  // a message each time an interrupt fires.
  libn64_thread_reg_intr(libn64_thread_self(), LIBN64_INTERRUPT_VI);

  // Variables
  uint16_t XRot = 0; // X Rotation Value (0..1023)
  uint16_t YRot = 0; // Y Rotation Value (0..1023)
  uint16_t ZRot = 0; // Z Rotation Value (0..1023)

  // Setup RDP buffer
  uint32_t rdp_start = memory_pos;
  rdp_set_scissor(0.0,0.0, 320.0,240.0, SCISSOR_FIELD_DISABLE,SCISSOR_EVEN); // Set Scissor: XH,YH, XL,YL, Scissor Field Enable,Field
  rdp_set_other_modes(CYCLE_TYPE_FILL); // Set_Other_Modes: CYCLE_TYPE_FILL

  // Setup RDP frame buffer
  uint32_t rdp_fb_origin = memory_pos + 4;
  rdp_set_color_image(IMAGE_DATA_FORMAT_RGBA,SIZE_OF_PIXEL_16B, 320, 0x00000000); // Set Color Image: Format,Size, Width, DRAM Address
  rdp_set_fill_color(24,128,212,255); // Set Fill Color: R,G,B,A (Blue)
  rdp_fill_rectangle(0.0,0.0, 319.0,239.0); // Fill Rectangle: XH,YH, XL,YL
  rdp_sync_pipe(); // Stall Pipeline, Until Preceeding Primitives Completely Finish

  rdp_set_other_modes(SAMPLE_TYPE|BI_LERP_0|ALPHA_DITHER_SEL_NO_DITHER|B_M1A_0_2); // Set Other Modes
  rdp_set_combine_mode(0x0,0x00, 0,0, 0x6,0x01, 0x0,0xF, 1,0, 0,0,0, 7,7,7); // Set Combine Mode: SubA RGB0,MulRGB0, SubA Alpha0,MulAlpha0, SubA RGB1,MulRGB1, SubB RGB0,SubB RGB1, SubA Alpha1,MulAlpha1, AddRGB0,SubB Alpha0,AddAlpha0, AddRGB1,SubB Alpha1,AddAlpha1

  uint32_t rdp_buffer = memory_pos; // Set RDP Buffer Address

  // For each frame...
  while (1) {
    // Point to VI to the last fb, swap the front and back fbs.
    vi_flush_state(&vi_state);
    vi_state.origin ^= 0x100000; // 1MB

    // RDP set color image to last fb, clear the framebuffer to fill color
    *(uintptr_t *)(0xA0100000 | rdp_fb_origin) = vi_state.origin; // Set Color Image: DRAM ADDRESS vi_state.origin

    // Draw scene
    // translate_x(Matrix3D, 50.0); // Translate: Matrix, X
    // translate_y(Matrix3D, 50.0); // Translate: Matrix, Y
    // translate_z(Matrix3D, 50.0); // Translate: Matrix, Z
    // translate_xyz(Matrix3D, CubeAPos[0], CubeAPos[1], CubeAPos[2]); // Translate: Matrix, X, Y, Z
    // rotate_x(Matrix3D, SinCos1024, XRot); // Rotate: Matrix, Precalc Table, X
    // rotate_y(Matrix3D, SinCos1024, YRot); // Rotate: Matrix, Precalc Table, Y
    // rotate_z(Matrix3D, SinCos1024, ZRot); // Rotate: Matrix, Precalc Table, Z
    // rotate_xy(Matrix3D, SinCos1024, XRot, YRot); // Rotate: Matrix, Precalc Table, X, Y
    // rotate_xz(Matrix3D, SinCos1024, XRot, ZRot); // Rotate: Matrix, Precalc Table, X, Z
    // rotate_yz(Matrix3D, SinCos1024, YRot, ZRot); // Rotate: Matrix, Precalc Table, Y, Z
    // rotate_xyz(Matrix3D, SinCos1024, XRot, YRot, ZRot); // Rotate: Matrix, Precalc Table, X, Y, Z

    memory_pos = rdp_buffer; // Set Start Of RDP Buffer

    matrix_identity(Matrix3D); // Reset Matrix To Identity
    translate_xyz(Matrix3D, CubeRedPos[0], CubeRedPos[1], CubeRedPos[2]); // Translate: Matrix, X, Y, Z
    rotate_x(Matrix3D, Sin1024, XRot); // Rotate: Matrix, Precalc Table, X
    fill_triangle_array(CubeTri, CubeRedCol, CULL_BACK, 0, 108); // Fill Triangle Array: Vert Array, Color Array, RDP Buffer, Culling, Base, Length

    matrix_identity(Matrix3D); // Reset Matrix To Identity
    translate_xyz(Matrix3D, CubeGreenPos[0], CubeGreenPos[1], CubeGreenPos[2]); // Translate: Matrix, X, Y, Z
    rotate_y(Matrix3D, Sin1024, YRot); // Rotate: Matrix, Precalc Table, Y
    fill_triangle_array(CubeTri, CubeGreenCol, CULL_BACK, 0, 108); // Fill Triangle Array: Vert Array, Color Array, RDP Buffer, Culling, Base, Length

    matrix_identity(Matrix3D); // Reset Matrix To Identity
    translate_xyz(Matrix3D, CubeBluePos[0], CubeBluePos[1], CubeBluePos[2]); // Translate: Matrix, X, Y, Z
    rotate_z(Matrix3D, Sin1024, ZRot); // Rotate: Matrix, Precalc Table, Z
    fill_triangle_array(CubeTri, CubeBlueCol, CULL_BACK, 0, 108); // Fill Triangle Array: Vert Array, Color Array, RDP Buffer, Culling, Base, Length

    matrix_identity(Matrix3D); // Reset Matrix To Identity
    translate_xyz(Matrix3D, CubeYellowPos[0], CubeYellowPos[1], CubeYellowPos[2]); // Translate: Matrix, X, Y, Z
    rotate_xy(Matrix3D, Sin1024, XRot, YRot); // Rotate: Matrix, Precalc Table, X, Y
    fill_triangle_array(CubeTri, CubeYellowCol, CULL_BACK, 0, 108); // Fill Triangle Array: Vert Array, Color Array, RDP Buffer, Culling, Base, Length

    matrix_identity(Matrix3D); // Reset Matrix To Identity
    translate_xyz(Matrix3D, CubePurplePos[0], CubePurplePos[1], CubePurplePos[2]); // Translate: Matrix, X, Y, Z
    rotate_xz(Matrix3D, Sin1024, XRot, ZRot); // Rotate: Matrix, Precalc Table, X, Z
    fill_triangle_array(CubeTri, CubePurpleCol, CULL_BACK, 0, 108); // Fill Triangle Array: Vert Array, Color Array, RDP Buffer, Culling, Base, Length

    matrix_identity(Matrix3D); // Reset Matrix To Identity
    translate_xyz(Matrix3D, CubeCyanPos[0], CubeCyanPos[1], CubeCyanPos[2]); // Translate: Matrix, X, Y, Z
    rotate_xyz(Matrix3D, Sin1024, XRot, YRot, ZRot); // Rotate: Matrix, Precalc Table, X, Y, Z
    fill_triangle_array(CubeTri, CubeCyanCol, CULL_BACK, 0, 108); // Fill Triangle Array: Vert Array, Color Array, RDP Buffer, Culling, Base, Length

    rdp_sync_full(); // Ensure Entire Scene Is Fully Drawn

    rdp_run(rdp_start, memory_pos); // Run RDP buffer: Start, End

    // Update triangle rotation variables
    XRot = (XRot + 1) & 1023;
    YRot = (YRot + 1) & 1023;
    ZRot = (ZRot + 1) & 1023;

    // Block until the next VI interrupt comes in.
    libn64_recvt_message();
  }
}
