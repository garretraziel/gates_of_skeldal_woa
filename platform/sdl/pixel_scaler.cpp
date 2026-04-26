#include "pixel_scaler.h"

// Scale2x / EPX / AdvMAME2x algorithm
// Reference: https://en.wikipedia.org/wiki/Pixel-art_scaling_algorithms
//
// Neighbor layout:      Output 2x2 block:
//      A                   1 2
//    C P B                 3 4
//      D
//
// Using Wikipedia's AdvMAME2x naming where A=up, B=right, C=left, D=down:
//   1=P; 2=P; 3=P; 4=P;
//   IF C==A AND C!=D AND A!=B => 1=A
//   IF A==B AND A!=C AND B!=D => 2=B
//   IF D==C AND D!=B AND C!=A => 3=C
//   IF B==D AND B!=A AND D!=C => 4=D

void pixel_scale2x(const pixel_t *src, pixel_t *dst,
                   int srcW, int srcH, int srcPitch, int dstPitch)
{
    for (int y = 0; y < srcH; ++y) {
        const pixel_t *rowC = src + y * srcPitch;
        const pixel_t *rowA = (y > 0) ? rowC - srcPitch : rowC;
        const pixel_t *rowD = (y < srcH - 1) ? rowC + srcPitch : rowC;

        pixel_t *out0 = dst + (y * 2) * dstPitch;
        pixel_t *out1 = out0 + dstPitch;

        for (int x = 0; x < srcW; ++x) {
            pixel_t P = rowC[x];
            pixel_t A = rowA[x];          // up
            pixel_t D = rowD[x];          // down
            pixel_t C = (x > 0) ? rowC[x - 1] : P;        // left
            pixel_t B = (x < srcW - 1) ? rowC[x + 1] : P; // right

            int ox = x * 2;
            out0[ox]     = (C == A && C != D && A != B) ? A : P; // top-left
            out0[ox + 1] = (A == B && A != C && B != D) ? B : P; // top-right
            out1[ox]     = (D == C && D != B && C != A) ? C : P; // bottom-left
            out1[ox + 1] = (B == D && B != A && D != C) ? D : P; // bottom-right
        }
    }
}

// Scale3x / AdvMAME3x algorithm
// Reference: https://en.wikipedia.org/wiki/Pixel-art_scaling_algorithms
//
// Neighbor layout:       Output 3x3 block:
//   A B C                  1 2 3
//   D E F                  4 5 6
//   G H I                  7 8 9

void pixel_scale3x(const pixel_t *src, pixel_t *dst,
                   int srcW, int srcH, int srcPitch, int dstPitch)
{
    for (int y = 0; y < srcH; ++y) {
        const pixel_t *row1 = src + y * srcPitch;
        const pixel_t *row0 = (y > 0) ? row1 - srcPitch : row1;
        const pixel_t *row2 = (y < srcH - 1) ? row1 + srcPitch : row1;

        pixel_t *o0 = dst + (y * 3) * dstPitch;
        pixel_t *o1 = o0 + dstPitch;
        pixel_t *o2 = o1 + dstPitch;

        for (int x = 0; x < srcW; ++x) {
            pixel_t A = row0[x > 0 ? x - 1 : x];
            pixel_t B = row0[x];
            pixel_t C = row0[x < srcW - 1 ? x + 1 : x];
            pixel_t D = row1[x > 0 ? x - 1 : x];
            pixel_t E = row1[x];
            pixel_t F = row1[x < srcW - 1 ? x + 1 : x];
            pixel_t G = row2[x > 0 ? x - 1 : x];
            pixel_t H = row2[x];
            pixel_t I = row2[x < srcW - 1 ? x + 1 : x];

            int ox = x * 3;

            // Corners: same conditions as Scale2x
            o0[ox]     = (D == B && D != H && B != F) ? D : E;
            o0[ox + 2] = (B == F && B != D && F != H) ? F : E;
            o2[ox]     = (H == D && H != F && D != B) ? D : E;
            o2[ox + 2] = (F == H && F != B && H != D) ? F : E;

            // Edges
            o0[ox + 1] = ((D == B && D != H && B != F && E != C)
                       || (B == F && B != D && F != H && E != A)) ? B : E;

            o1[ox]     = ((H == D && H != F && D != B && E != A)
                       || (D == B && D != H && B != F && E != G)) ? D : E;

            o1[ox + 2] = ((B == F && B != D && F != H && E != I)
                       || (F == H && F != B && H != D && E != C)) ? F : E;

            o2[ox + 1] = ((F == H && F != B && H != D && E != G)
                       || (H == D && H != F && D != B && E != I)) ? H : E;

            // Center
            o1[ox + 1] = E;
        }
    }
}
