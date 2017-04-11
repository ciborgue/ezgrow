/* $Id: test_qrSequence.c 657 2013-04-13 03:42:40Z  $ */
#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <strings.h>
#include <locale.h>
#include <sys/types.h>
#include <assert.h>

#include "qrEnum.h"
#include "qrMatrix.h"
#include "qrInterface.h"

extern u_int16_t   *getLog();
struct { /* {{{ */
    int ver;
    u_int16_t     qrLog[QRSIZE(40) * QRSIZE(40)];
} data[] = {
    {0,{ /* {{{2 Version 1 */
        0x1414, 0x1314, 0x1413, 0x1313, 0x1412, 0x1312, 0x1411, 0x1311,
        0x1410, 0x1310, 0x140f, 0x130f, 0x140e, 0x130e, 0x140d, 0x130d,
        0x140c, 0x130c, 0x140b, 0x130b, 0x140a, 0x130a, 0x1409, 0x1309,

        0x1209, 0x1109, 0x120a, 0x110a, 0x120b, 0x110b, 0x120c, 0x110c,
        0x120d, 0x110d, 0x120e, 0x110e, 0x120f, 0x110f, 0x1210, 0x1110,
        0x1211, 0x1111, 0x1212, 0x1112, 0x1213, 0x1113, 0x1214, 0x1114,

        0x1014, 0x0f14, 0x1013, 0x0f13, 0x1012, 0x0f12, 0x1011, 0x0f11,
        0x1010, 0x0f10, 0x100f, 0x0f0f, 0x100e, 0x0f0e, 0x100d, 0x0f0d,
        0x100c, 0x0f0c, 0x100b, 0x0f0b, 0x100a, 0x0f0a, 0x1009, 0x0f09,

        0x0e09, 0x0d09, 0x0e0a, 0x0d0a, 0x0e0b, 0x0d0b, 0x0e0c, 0x0d0c,
        0x0e0d, 0x0d0d, 0x0e0e, 0x0d0e, 0x0e0f, 0x0d0f, 0x0e10, 0x0d10,
        0x0e11, 0x0d11, 0x0e12, 0x0d12, 0x0e13, 0x0d13, 0x0e14, 0x0d14,

        0x0c14, 0x0b14, 0x0c13, 0x0b13, 0x0c12, 0x0b12, 0x0c11, 0x0b11,
        0x0c10, 0x0b10, 0x0c0f, 0x0b0f, 0x0c0e, 0x0b0e, 0x0c0d, 0x0b0d,
        0x0c0c, 0x0b0c, 0x0c0b, 0x0b0b, 0x0c0a, 0x0b0a, 0x0c09, 0x0b09,

        /* here is the special case ----! (note xx07 -> xx05) */
        0x0c08, 0x0b08, 0x0c07, 0x0b07, 0x0c05, 0x0b05, 0x0c04, 0x0b04,

        0x0c03, 0x0b03, 0x0c02, 0x0b02, 0x0c01, 0x0b01, 0x0c00, 0x0b00,
        0x0a00, 0x0900, 0x0a01, 0x0901, 0x0a02, 0x0902, 0x0a03, 0x0903,

        /* here is the special case ----! (note xx05 -> xx07) */
        0x0a04, 0x0904, 0x0a05, 0x0905, 0x0a07, 0x0907, 0x0a08, 0x0908,

        0x0a09, 0x0909, 0x0a0a, 0x090a, 0x0a0b, 0x090b, 0x0a0c, 0x090c,
        0x0a0d, 0x090d, 0x0a0e, 0x090e, 0x0a0f, 0x090f, 0x0a10, 0x0910,
        0x0a11, 0x0911, 0x0a12, 0x0912, 0x0a13, 0x0913, 0x0a14, 0x0914,

        0x080c, 0x070c, 0x080b, 0x070b, 0x080a, 0x070a, 0x0809, 0x0709,
        0x0509, 0x0409, 0x050a, 0x040a, 0x050b, 0x040b, 0x050c, 0x040c,
        0x030c, 0x020c, 0x030b, 0x020b, 0x030a, 0x020a, 0x0309, 0x0209,
        0x0109, 0x0009, 0x010a, 0x000a, 0x010b, 0x000b, 0x010c, 0x000c,

        0xffff /* v1 is complete */
    }}, /* }}}2 */
    {1,{ /* {{{2 Version 2 */
        0x1818, 0x1718, 0x1817, 0x1717, 0x1816, 0x1716, 0x1815, 0x1715,/*D01*/
        0x1814, 0x1714, 0x1813, 0x1713, 0x1812, 0x1712, 0x1811, 0x1711,
        0x1810, 0x1710, 0x180f, 0x170f, 0x180e, 0x170e, 0x180d, 0x170d,
        0x180c, 0x170c, 0x180b, 0x170b, 0x180a, 0x170a, 0x1809, 0x1709,

        0x1609, 0x1509, 0x160a, 0x150a, 0x160b, 0x150b, 0x160c, 0x150c,/*D05*/
        0x160d, 0x150d, 0x160e, 0x150e, 0x160f, 0x150f, 0x1610, 0x1510,
        0x1611, 0x1511, 0x1612, 0x1512, 0x1613, 0x1513, 0x1614, 0x1514,
        0x1615, 0x1515, 0x1616, 0x1516, 0x1617, 0x1517, 0x1618, 0x1518,

        0x1418, 0x1318, 0x1417, 0x1317, 0x1416, 0x1316, 0x1415, 0x1315,/*D09*/

        /* alignment skipped */
        0x140f, 0x130f, 0x140e, 0x130e, 0x140d, 0x130d, 0x140c, 0x130c,

        /* special block -------------------------------! */
        0x140b, 0x130b, 0x140a, 0x130a, 0x1409, 0x1309, 0x1209, 0x1109,/*D11*/

        0x120a, 0x110a, 0x120b, 0x110b, 0x120c, 0x110c, 0x120d, 0x110d,

        /* alignment skipped */
        0x120e, 0x110e, 0x120f, 0x110f, 0x1215, 0x1115, 0x1216, 0x1116,

        /* 'horizontal' block at 18,23 */
        0x1217, 0x1117, 0x1218, 0x1118, 0x1018, 0x0f18, 0x1017, 0x0f17,/*D14*/

        /* 'glide' the alignment pattern */
        0x1016, 0x0f16, 0x1015, 0x0f15, 0x0f14, 0x0f13, 0x0f12, 0x0f11,/*D15*/

        0x0f10, 0x100f, 0x0f0f, 0x100e, 0x0f0e, 0x100d, 0x0f0d, 0x100c,/*D16*/
        0x0f0c, 0x100b, 0x0f0b, 0x100a, 0x0f0a, 0x1009, 0x0f09, 0x1008,/*D17*/
        
        /* D18 traverses the timing pattern */
        0x0f08, 0x1007, 0x0f07, 0x1005, 0x0f05, 0x1004, 0x0f04, 0x1003,/*D18*/

        /* D19 is the ugly one on top */
        0x0f03, 0x1002, 0x0f02, 0x1001, 0x0f01, 0x1000, 0x0f00, 0x0e00,/*D19*/

        0x0d00, 0x0e01, 0x0d01, 0x0e02, 0x0d02, 0x0e03, 0x0d03, 0x0e04,/*D20*/

        /* D21 traverses the timing */
        0x0d04, 0x0e05, 0x0d05, 0x0e07, 0x0d07, 0x0e08, 0x0d08, 0x0e09,/*D21*/

        0x0d09, 0x0e0a, 0x0d0a, 0x0e0b, 0x0d0b, 0x0e0c, 0x0d0c, 0x0e0d,
        0x0d0d, 0x0e0e, 0x0d0e, 0x0e0f, 0x0d0f, 0x0e10, 0x0d10, 0x0e11,
        0x0d11, 0x0e12, 0x0d12, 0x0e13, 0x0d13, 0x0e14, 0x0d14, 0x0e15,

        /* D25 is on the bottom with 1px extends to the left */
        0x0d15, 0x0e16, 0x0d16, 0x0e17, 0x0d17, 0x0e18, 0x0d18, 0x0c18,/*D25*/

        0x0b18, 0x0c17, 0x0b17, 0x0c16, 0x0b16, 0x0c15, 0x0b15, 0x0c14,
        0x0b14, 0x0c13, 0x0b13, 0x0c12, 0x0b12, 0x0c11, 0x0b11, 0x0c10,
        0x0b10, 0x0c0f, 0x0b0f, 0x0c0e, 0x0b0e, 0x0c0d, 0x0b0d, 0x0c0c,
        0x0b0c, 0x0c0b, 0x0b0b, 0x0c0a, 0x0b0a, 0x0c09, 0x0b09, 0x0c08,/*E01*/

        /* E02 traverses timing pattern */
        0x0b08, 0x0c07, 0x0b07, 0x0c05, 0x0b05, 0x0c04, 0x0b04, 0x0c03,

        0x0b03, 0x0c02, 0x0b02, 0x0c01, 0x0b01, 0x0c00, 0x0b00, 0x0a00,/*E03*/
        0x0900, 0x0a01, 0x0901, 0x0a02, 0x0902, 0x0a03, 0x0903, 0x0a04,/*E04*/

        /* E05 traverses timing pattern */
        0x0904, 0x0a05, 0x0905, 0x0a07, 0x0907, 0x0a08, 0x0908, 0x0a09,/*E05*/

        0x0909, 0x0a0a, 0x090a, 0x0a0b, 0x090b, 0x0a0c, 0x090c, 0x0a0d,
        0x090d, 0x0a0e, 0x090e, 0x0a0f, 0x090f, 0x0a10, 0x0910, 0x0a11,
        0x0911, 0x0a12, 0x0912, 0x0a13, 0x0913, 0x0a14, 0x0914, 0x0a15,

        /* E09 has a weird split */
        0x0915, 0x0a16, 0x0916, 0x0a17, 0x0917, 0x0a18, 0x0918, 0x0810,/*E09*/

        0x0710, 0x080f, 0x070f, 0x080e, 0x070e, 0x080d, 0x070d, 0x080c,/*E10*/

        /* E11 has a weird split (different from E09) */
        0x070c, 0x080b, 0x070b, 0x080a, 0x070a, 0x0809, 0x0709, 0x0509,/*E11*/

        0x0409, 0x050a, 0x040a, 0x050b, 0x040b, 0x050c, 0x040c, 0x050d,/*E12*/
        0x040d, 0x050e, 0x040e, 0x050f, 0x040f, 0x0510, 0x0410, 0x0310,/*E13*/

        0x0210, 0x030f, 0x020f, 0x030e, 0x020e, 0x030d, 0x020d, 0x030c,
        0x020c, 0x030b, 0x020b, 0x030a, 0x020a, 0x0309, 0x0209, 0x0109,

        0x0009, 0x010a, 0x000a, 0x010b, 0x000b, 0x010c, 0x000c, 0x010d,/*E16*/

        0xffff /* v2 is complete */
    }}, /* }}}2 */
    {6,{ /* {{{2 version 7 */
        0x2c2c, 0x2b2c, 0x2c2b, 0x2b2b, 0x2c2a, 0x2b2a, 0x2c29, 0x2b29,/*D01*/
        0x2c28, 0x2b28, 0x2c27, 0x2b27, 0x2c26, 0x2b26, 0x2c25, 0x2b25,/*D14*/
        0x2c24, 0x2b24, 0x2c23, 0x2b23, 0x2c22, 0x2b22, 0x2c21, 0x2b21,
        0x2c20, 0x2b20, 0x2c1f, 0x2b1f, 0x2c1e, 0x2b1e, 0x2c1d, 0x2b1d,
        0x2c1c, 0x2b1c, 0x2c1b, 0x2b1b, 0x2c1a, 0x2b1a, 0x2c19, 0x2b19,
        0x2c18, 0x2b18, 0x2c17, 0x2b17, 0x2c16, 0x2b16, 0x2c15, 0x2b15,
        0x2c14, 0x2b14, 0x2c13, 0x2b13, 0x2c12, 0x2b12, 0x2c11, 0x2b11,
        0x2c10, 0x2b10, 0x2c0f, 0x2b0f, 0x2c0e, 0x2b0e, 0x2c0d, 0x2b0d,
        0x2c0c, 0x2b0c, 0x2c0b, 0x2b0b, 0x2c0a, 0x2b0a, 0x2c09, 0x2b09,/*D41*/

        0x2a09, 0x2909, 0x2a0a, 0x290a, 0x2a0b, 0x290b, 0x2a0c, 0x290c,/*D54*/
        0x2a0d, 0x290d, 0x2a0e, 0x290e, 0x2a0f, 0x290f, 0x2a10, 0x2910,
        0x2a11, 0x2911, 0x2a12, 0x2912, 0x2a13, 0x2913, 0x2a14, 0x2914,
        0x2a15, 0x2915, 0x2a16, 0x2916, 0x2a17, 0x2917, 0x2a18, 0x2918,
        0x2a19, 0x2919, 0x2a1a, 0x291a, 0x2a1b, 0x291b, 0x2a1c, 0x291c,
        0x2a1d, 0x291d, 0x2a1e, 0x291e, 0x2a1f, 0x291f, 0x2a20, 0x2920,
        0x2a21, 0x2921, 0x2a22, 0x2922, 0x2a23, 0x2923, 0x2a24, 0x2924,
        0x2a25, 0x2925, 0x2a26, 0x2926, 0x2a27, 0x2927, 0x2a28, 0x2928,
        0x2a29, 0x2929, 0x2a2a, 0x292a, 0x2a2b, 0x292b, 0x2a2c, 0x292c,/*D30*/

        0x282c, 0x272c, 0x282b, 0x272b, 0x282a, 0x272a, 0x2829, 0x2729,/*D43*/

        0x2823, 0x2723, 0x2822, 0x2722, 0x2821, 0x2721, 0x2820, 0x2720,/*D56*/
        0x281f, 0x271f, 0x281e, 0x271e, 0x281d, 0x271d, 0x281c, 0x271c,
        0x281b, 0x271b, 0x281a, 0x271a, 0x2819, 0x2719, 0x2813, 0x2713,

        0x2812, 0x2712, 0x2811, 0x2711, 0x2810, 0x2710, 0x280f, 0x270f,/*D31*/
        0x280e, 0x270e, 0x280d, 0x270d, 0x280c, 0x270c, 0x280b, 0x270b,

        0x280a, 0x270a, 0x2809, 0x2709, 0x2609, 0x2509, 0x260a, 0x250a,/*D57*/

        0x260b, 0x250b, 0x260c, 0x250c, 0x260d, 0x250d, 0x260e, 0x250e,/*D06*/
        0x260f, 0x250f, 0x2610, 0x2510, 0x2611, 0x2511, 0x2612, 0x2512,

        0x2613, 0x2513, 0x2619, 0x2519, 0x261a, 0x251a, 0x261b, 0x251b,/*D32*/
        0x261c, 0x251c, 0x261d, 0x251d, 0x261e, 0x251e, 0x261f, 0x251f,
        0x2620, 0x2520, 0x2621, 0x2521, 0x2622, 0x2522, 0x2623, 0x2523,

        0x2629, 0x2529, 0x262a, 0x252a, 0x262b, 0x252b, 0x262c, 0x252c,/*D07*/
        0x242c, 0x232c, 0x242b, 0x232b, 0x242a, 0x232a, 0x2429, 0x2329,/*D20*/

        0x2328, 0x2327, 0x2326, 0x2325, 0x2324, 0x2423, 0x2323, 0x2422,/*D33*/

        0x2322, 0x2421, 0x2321, 0x2420, 0x2320, 0x241f, 0x231f, 0x241e,
        0x231e, 0x241d, 0x231d, 0x241c, 0x231c, 0x241b, 0x231b, 0x241a,/*D59*/

        0x231a, 0x2419, 0x2319, 0x2318, 0x2317, 0x2316, 0x2315, 0x2314,/*D08*/

        0x2413, 0x2313, 0x2412, 0x2312, 0x2411, 0x2311, 0x2410, 0x2310,
        0x240f, 0x230f, 0x240e, 0x230e, 0x240d, 0x230d, 0x240c, 0x230c,
        0x240b, 0x230b, 0x240a, 0x230a, 0x2409, 0x2309, 0x2408, 0x2308,/*D47*/

        /* D60 is split TODO verify the last 6 pixels direction */
        0x2407, 0x2307, 0x2100, 0x2101, 0x2102, 0x2103, 0x2104, 0x2105,/*D60*/

        /* D09-D49 */
        0x2207, 0x2107, 0x2208, 0x2108, 0x2209, 0x2109, 0x220a, 0x210a,
        0x220b, 0x210b, 0x220c, 0x210c, 0x220d, 0x210d, 0x220e, 0x210e,
        0x220f, 0x210f, 0x2210, 0x2110, 0x2211, 0x2111, 0x2212, 0x2112,
        0x2213, 0x2113, 0x2214, 0x2114, 0x2215, 0x2115, 0x2216, 0x2116,
        0x2217, 0x2117, 0x2218, 0x2118, 0x2219, 0x2119, 0x221a, 0x211a,
        0x221b, 0x211b, 0x221c, 0x211c, 0x221d, 0x211d, 0x221e, 0x211e,
        0x221f, 0x211f, 0x2220, 0x2120, 0x2221, 0x2121, 0x2222, 0x2122,
        0x2223, 0x2123, 0x2224, 0x2124, 0x2225, 0x2125, 0x2226, 0x2126,
        0x2227, 0x2127, 0x2228, 0x2128, 0x2229, 0x2129, 0x222a, 0x212a,

        0x222b, 0x212b, 0x222c, 0x212c, 0x202c, 0x1f2c, 0x202b, 0x1f2b,/*D62*/

        /* D11-D51 */
        0x202a, 0x1f2a, 0x2029, 0x1f29, 0x2028, 0x1f28, 0x2027, 0x1f27,
        0x2026, 0x1f26, 0x2025, 0x1f25, 0x2024, 0x1f24, 0x2023, 0x1f23,
        0x2022, 0x1f22, 0x2021, 0x1f21, 0x2020, 0x1f20, 0x201f, 0x1f1f,
        0x201e, 0x1f1e, 0x201d, 0x1f1d, 0x201c, 0x1f1c, 0x201b, 0x1f1b,
        0x201a, 0x1f1a, 0x2019, 0x1f19, 0x2018, 0x1f18, 0x2017, 0x1f17,
        0x2016, 0x1f16, 0x2015, 0x1f15, 0x2014, 0x1f14, 0x2013, 0x1f13,
        0x2012, 0x1f12, 0x2011, 0x1f11, 0x2010, 0x1f10, 0x200f, 0x1f0f,
        0x200e, 0x1f0e, 0x200d, 0x1f0d, 0x200c, 0x1f0c, 0x200b, 0x1f0b,
        0x200a, 0x1f0a, 0x2009, 0x1f09, 0x2008, 0x1f08, 0x2007, 0x1f07,/*D51*/

        0x2005, 0x1f05, 0x2004, 0x1f04, 0x2003, 0x1f03, 0x2002, 0x1f02,/*D64*/
        0x2001, 0x1f01, 0x2000, 0x1f00, 0x1e00, 0x1d00, 0x1e01, 0x1d01,/*D13*/
        0x1e02, 0x1d02, 0x1e03, 0x1d03, 0x1e04, 0x1d04, 0x1e05, 0x1d05,/*D26*/

        0x1e07, 0x1d07, 0x1e08, 0x1d08, 0x1e09, 0x1d09, 0x1e0a, 0x1d0a,/*D39*/
        0x1e0b, 0x1d0b, 0x1e0c, 0x1d0c, 0x1e0d, 0x1d0d, 0x1e0e, 0x1d0e,
        0x1e0f, 0x1d0f, 0x1e10, 0x1d10, 0x1e11, 0x1d11, 0x1e12, 0x1d12,
        0x1e13, 0x1d13, 0x1e14, 0x1d14, 0x1e15, 0x1d15, 0x1e16, 0x1d16,
        0x1e17, 0x1d17, 0x1e18, 0x1d18, 0x1e19, 0x1d19, 0x1e1a, 0x1d1a,
        0x1e1b, 0x1d1b, 0x1e1c, 0x1d1c, 0x1e1d, 0x1d1d, 0x1e1e, 0x1d1e,
        0x1e1f, 0x1d1f, 0x1e20, 0x1d20, 0x1e21, 0x1d21, 0x1e22, 0x1d22,
        0x1e23, 0x1d23, 0x1e24, 0x1d24, 0x1e25, 0x1d25, 0x1e26, 0x1d26,
        0x1e27, 0x1d27, 0x1e28, 0x1d28, 0x1e29, 0x1d29, 0x1e2a, 0x1d2a,/*E105*/

        0x1e2b, 0x1d2b, 0x1e2c, 0x1d2c, 0x1c2c, 0x1b2c, 0x1c2b, 0x1b2b,/*E02*/

        0x1c2a, 0x1b2a, 0x1c29, 0x1b29, 0x1c28, 0x1b28, 0x1c27, 0x1b27,/*E28*/
        0x1c26, 0x1b26, 0x1c25, 0x1b25, 0x1c24, 0x1b24, 0x1c23, 0x1b23,
        0x1c22, 0x1b22, 0x1c21, 0x1b21, 0x1c20, 0x1b20, 0x1c1f, 0x1b1f,
        0x1c1e, 0x1b1e, 0x1c1d, 0x1b1d, 0x1c1c, 0x1b1c, 0x1c1b, 0x1b1b,
        0x1c1a, 0x1b1a, 0x1c19, 0x1b19, 0x1c18, 0x1b18, 0x1c17, 0x1b17,
        0x1c16, 0x1b16, 0x1c15, 0x1b15, 0x1c14, 0x1b14, 0x1c13, 0x1b13,
        0x1c12, 0x1b12, 0x1c11, 0x1b11, 0x1c10, 0x1b10, 0x1c0f, 0x1b0f,
        0x1c0e, 0x1b0e, 0x1c0d, 0x1b0d, 0x1c0c, 0x1b0c, 0x1c0b, 0x1b0b,
        0x1c0a, 0x1b0a, 0x1c09, 0x1b09, 0x1c08, 0x1b08, 0x1c07, 0x1b07,/*E107*/

        0x1c05, 0x1b05, 0x1c04, 0x1b04, 0x1c03, 0x1b03, 0x1c02, 0x1b02,/*E04*/

        0x1c01, 0x1b01, 0x1c00, 0x1b00, 0x1a00, 0x1900, 0x1a01, 0x1901,/*E30*/

        0x1a02, 0x1902, 0x1a03, 0x1903, 0x1a04, 0x1904, 0x1a05, 0x1905,/*E56*/

        0x1a07, 0x1907, 0x1a08, 0x1908, 0x1a09, 0x1909, 0x1a0a, 0x190a,/*E82*/
        0x1a0b, 0x190b, 0x1a0c, 0x190c, 0x1a0d, 0x190d, 0x1a0e, 0x190e,
        0x1a0f, 0x190f, 0x1a10, 0x1910, 0x1a11, 0x1911, 0x1a12, 0x1912,
        0x1a13, 0x1913, 0x1a14, 0x1914, 0x1a15, 0x1915, 0x1a16, 0x1916,
        0x1a17, 0x1917, 0x1a18, 0x1918, 0x1a19, 0x1919, 0x1a1a, 0x191a,
        0x1a1b, 0x191b, 0x1a1c, 0x191c, 0x1a1d, 0x191d, 0x1a1e, 0x191e,
        0x1a1f, 0x191f, 0x1a20, 0x1920, 0x1a21, 0x1921, 0x1a22, 0x1922,
        0x1a23, 0x1923, 0x1a24, 0x1924, 0x1a25, 0x1925, 0x1a26, 0x1926,
        0x1a27, 0x1927, 0x1a28, 0x1928, 0x1a29, 0x1929, 0x1a2a, 0x192a,
        0x1a2b, 0x192b, 0x1a2c, 0x192c, 0x182c, 0x172c, 0x182b, 0x172b,/*E58*/

        /* E84 traverses alignment */
        0x182a, 0x172a, 0x1829, 0x1729, 0x1823, 0x1723, 0x1822, 0x1722,

        0x1821, 0x1721, 0x1820, 0x1720, 0x181f, 0x171f, 0x181e, 0x171e,/*E110*/
        0x181d, 0x171d, 0x181c, 0x171c, 0x181b, 0x171b, 0x181a, 0x171a,/*E07*/

        /* E33 traverses alignment */
        0x1819, 0x1719, 0x1813, 0x1713, 0x1812, 0x1712, 0x1811, 0x1711,/*E33*/

        0x1810, 0x1710, 0x180f, 0x170f, 0x180e, 0x170e, 0x180d, 0x170d,
        0x180c, 0x170c, 0x180b, 0x170b, 0x180a, 0x170a, 0x1809, 0x1709,

        0x1803, 0x1703, 0x1802, 0x1702, 0x1801, 0x1701, 0x1800, 0x1700,/*E11*/

        0x1600, 0x1500, 0x1601, 0x1501, 0x1602, 0x1502, 0x1603, 0x1503,/*E08*/

        0x1609, 0x1509, 0x160a, 0x150a, 0x160b, 0x150b, 0x160c, 0x150c,
        0x160d, 0x150d, 0x160e, 0x150e, 0x160f, 0x150f, 0x1610, 0x1510,

        /* E86 traverses alignment */
        0x1611, 0x1511, 0x1612, 0x1512, 0x1613, 0x1513, 0x1619, 0x1519,/*E86*/

        0x161a, 0x151a, 0x161b, 0x151b, 0x161c, 0x151c, 0x161d, 0x151d,
        0x161e, 0x151e, 0x161f, 0x151f, 0x1620, 0x1520, 0x1621, 0x1521,

        /* E35 traverses alignment */
        0x1622, 0x1522, 0x1623, 0x1523, 0x1629, 0x1529, 0x162a, 0x152a,/*E35*/

        0x162b, 0x152b, 0x162c, 0x152c, 0x142c, 0x132c, 0x142b, 0x132b,/*E61*/

        0x142a, 0x132a, 0x1429, 0x1329, 0x1328, 0x1327, 0x1326, 0x1325,/*E87*/

        0x1324, 0x1423, 0x1323, 0x1422, 0x1322, 0x1421, 0x1321, 0x1420,
        0x1320, 0x141f, 0x131f, 0x141e, 0x131e, 0x141d, 0x131d, 0x141c,

        /* E36 is irregular + collides with alignment */
        0x131c, 0x141b, 0x131b, 0x141a, 0x131a, 0x1419, 0x1319, 0x1318,/*E36*/

        /* E62 goes around the alignment */
        0x1317, 0x1316, 0x1315, 0x1314, 0x1413, 0x1313, 0x1412, 0x1312,/*E62*/

        0x1411, 0x1311, 0x1410, 0x1310, 0x140f, 0x130f, 0x140e, 0x130e,/*E88*/
        0x140d, 0x130d, 0x140c, 0x130c, 0x140b, 0x130b, 0x140a, 0x130a,/*E114*/

        0x1409, 0x1309, 0x1308, 0x1307, 0x1305, 0x1304, 0x1403, 0x1303,/*E11*/

        0x1402, 0x1302, 0x1401, 0x1301, 0x1400, 0x1300, 0x1200, 0x1100,/*E37*/

        0x1201, 0x1101, 0x1202, 0x1102, 0x1203, 0x1103, 0x1204, 0x1104,/*E63*/
        0x1205, 0x1105, 0x1207, 0x1107, 0x1208, 0x1108, 0x1209, 0x1109,/*E89*/

        0x120a, 0x110a, 0x120b, 0x110b, 0x120c, 0x110c, 0x120d, 0x110d,/*E115*/
        0x120e, 0x110e, 0x120f, 0x110f, 0x1210, 0x1110, 0x1211, 0x1111,
        0x1212, 0x1112, 0x1213, 0x1113, 0x1214, 0x1114, 0x1215, 0x1115,
        0x1216, 0x1116, 0x1217, 0x1117, 0x1218, 0x1118, 0x1219, 0x1119,
        0x121a, 0x111a, 0x121b, 0x111b, 0x121c, 0x111c, 0x121d, 0x111d,
        0x121e, 0x111e, 0x121f, 0x111f, 0x1220, 0x1120, 0x1221, 0x1121,
        0x1222, 0x1122, 0x1223, 0x1123, 0x1224, 0x1124, 0x1225, 0x1125,
        0x1226, 0x1126, 0x1227, 0x1127, 0x1228, 0x1128, 0x1229, 0x1129,
        0x122a, 0x112a, 0x122b, 0x112b, 0x122c, 0x112c, 0x102c, 0x0f2c,/*E65*/

        0x102b, 0x0f2b, 0x102a, 0x0f2a, 0x1029, 0x0f29, 0x1028, 0x0f28,/*E91*/
        0x1027, 0x0f27, 0x1026, 0x0f26, 0x1025, 0x0f25, 0x1024, 0x0f24,
        0x1023, 0x0f23, 0x1022, 0x0f22, 0x1021, 0x0f21, 0x1020, 0x0f20,
        0x101f, 0x0f1f, 0x101e, 0x0f1e, 0x101d, 0x0f1d, 0x101c, 0x0f1c,
        0x101b, 0x0f1b, 0x101a, 0x0f1a, 0x1019, 0x0f19, 0x1018, 0x0f18,
        0x1017, 0x0f17, 0x1016, 0x0f16, 0x1015, 0x0f15, 0x1014, 0x0f14,
        0x1013, 0x0f13, 0x1012, 0x0f12, 0x1011, 0x0f11, 0x1010, 0x0f10,
        0x100f, 0x0f0f, 0x100e, 0x0f0e, 0x100d, 0x0f0d, 0x100c, 0x0f0c,
        0x100b, 0x0f0b, 0x100a, 0x0f0a, 0x1009, 0x0f09, 0x1008, 0x0f08,
        0x1007, 0x0f07, 0x1005, 0x0f05, 0x1004, 0x0f04, 0x1003, 0x0f03,/*E67*/

        0x1002, 0x0f02, 0x1001, 0x0f01, 0x1000, 0x0f00, 0x0e00, 0x0d00,/*E93*/

        0x0e01, 0x0d01, 0x0e02, 0x0d02, 0x0e03, 0x0d03, 0x0e04, 0x0d04,
        0x0e05, 0x0d05, 0x0e07, 0x0d07, 0x0e08, 0x0d08, 0x0e09, 0x0d09,/*E16*/

        0x0e0a, 0x0d0a, 0x0e0b, 0x0d0b, 0x0e0c, 0x0d0c, 0x0e0d, 0x0d0d,/*E42*/
        0x0e0e, 0x0d0e, 0x0e0f, 0x0d0f, 0x0e10, 0x0d10, 0x0e11, 0x0d11,
        0x0e12, 0x0d12, 0x0e13, 0x0d13, 0x0e14, 0x0d14, 0x0e15, 0x0d15,
        0x0e16, 0x0d16, 0x0e17, 0x0d17, 0x0e18, 0x0d18, 0x0e19, 0x0d19,
        0x0e1a, 0x0d1a, 0x0e1b, 0x0d1b, 0x0e1c, 0x0d1c, 0x0e1d, 0x0d1d,
        0x0e1e, 0x0d1e, 0x0e1f, 0x0d1f, 0x0e20, 0x0d20, 0x0e21, 0x0d21,
        0x0e22, 0x0d22, 0x0e23, 0x0d23, 0x0e24, 0x0d24, 0x0e25, 0x0d25,
        0x0e26, 0x0d26, 0x0e27, 0x0d27, 0x0e28, 0x0d28, 0x0e29, 0x0d29,
        0x0e2a, 0x0d2a, 0x0e2b, 0x0d2b, 0x0e2c, 0x0d2c, 0x0c2c, 0x0b2c,/*E121*/

        0x0c2b, 0x0b2b, 0x0c2a, 0x0b2a, 0x0c29, 0x0b29, 0x0c28, 0x0b28,/*E18*/
        0x0c27, 0x0b27, 0x0c26, 0x0b26, 0x0c25, 0x0b25, 0x0c24, 0x0b24,
        0x0c23, 0x0b23, 0x0c22, 0x0b22, 0x0c21, 0x0b21, 0x0c20, 0x0b20,
        0x0c1f, 0x0b1f, 0x0c1e, 0x0b1e, 0x0c1d, 0x0b1d, 0x0c1c, 0x0b1c,
        0x0c1b, 0x0b1b, 0x0c1a, 0x0b1a, 0x0c19, 0x0b19, 0x0c18, 0x0b18,
        0x0c17, 0x0b17, 0x0c16, 0x0b16, 0x0c15, 0x0b15, 0x0c14, 0x0b14,
        0x0c13, 0x0b13, 0x0c12, 0x0b12, 0x0c11, 0x0b11, 0x0c10, 0x0b10,
        0x0c0f, 0x0b0f, 0x0c0e, 0x0b0e, 0x0c0d, 0x0b0d, 0x0c0c, 0x0b0c,
        0x0c0b, 0x0b0b, 0x0c0a, 0x0b0a, 0x0c09, 0x0b09, 0x0c08, 0x0b08,
        0x0c07, 0x0b07, 0x0c05, 0x0b05, 0x0c04, 0x0b04, 0x0c03, 0x0b03,/*E123*/

        0x0c02, 0x0b02, 0x0c01, 0x0b01, 0x0c00, 0x0b00, 0x0a00, 0x0900,/*E20*/

        0x0a01, 0x0901, 0x0a02, 0x0902, 0x0a03, 0x0903, 0x0a04, 0x0904,
        0x0a05, 0x0905, 0x0a07, 0x0907, 0x0a08, 0x0908, 0x0a09, 0x0909,/*E72*/

        0x0a0a, 0x090a, 0x0a0b, 0x090b, 0x0a0c, 0x090c, 0x0a0d, 0x090d,/*E98*/
        0x0a0e, 0x090e, 0x0a0f, 0x090f, 0x0a10, 0x0910, 0x0a11, 0x0911,
        0x0a12, 0x0912, 0x0a13, 0x0913, 0x0a14, 0x0914, 0x0a15, 0x0915,
        0x0a16, 0x0916, 0x0a17, 0x0917, 0x0a18, 0x0918, 0x0a19, 0x0919,
        0x0a1a, 0x091a, 0x0a1b, 0x091b, 0x0a1c, 0x091c, 0x0a1d, 0x091d,
        0x0a1e, 0x091e, 0x0a1f, 0x091f, 0x0a20, 0x0920, 0x0a21, 0x0921,
        0x0a22, 0x0922, 0x0a23, 0x0923, 0x0a24, 0x0924, 0x0a25, 0x0925,
        0x0a26, 0x0926, 0x0a27, 0x0927, 0x0a28, 0x0928, 0x0a29, 0x0929,

        /* E48 splits around the version ---------------!                   */
        0x0a2a, 0x092a, 0x0a2b, 0x092b, 0x0a2c, 0x092c, 0x0824, 0x0724,/*E48*/

        0x0823, 0x0723, 0x0822, 0x0722, 0x0821, 0x0721, 0x0820, 0x0720,
        0x081f, 0x071f, 0x081e, 0x071e, 0x081d, 0x071d, 0x081c, 0x071c,
        0x081b, 0x071b, 0x081a, 0x071a, 0x0819, 0x0719, 0x0813, 0x0713,/*E126*/

        0x0812, 0x0712, 0x0811, 0x0711, 0x0810, 0x0710, 0x080f, 0x070f,
        0x080e, 0x070e, 0x080d, 0x070d, 0x080c, 0x070c, 0x080b, 0x070b,

        /* E75 is split horizontally across the timing pattern */
        0x080a, 0x070a, 0x0809, 0x0709, 0x0509, 0x0409, 0x050a, 0x040a,

        0x050b, 0x040b, 0x050c, 0x040c, 0x050d, 0x040d, 0x050e, 0x040e,
        0x050f, 0x040f, 0x0510, 0x0410, 0x0511, 0x0411, 0x0512, 0x0412,

        /* E24 is split by alignment */
        0x0513, 0x0413, 0x0519, 0x0419, 0x051a, 0x041a, 0x051b, 0x041b,

        0x051c, 0x041c, 0x051d, 0x041d, 0x051e, 0x041e, 0x051f, 0x041f,
        0x0520, 0x0420, 0x0521, 0x0421, 0x0321, 0x0221, 0x0320, 0x0220,/*E76*/

        0x031f, 0x021f, 0x031e, 0x021e, 0x031d, 0x021d, 0x031c, 0x021c,
        0x031b, 0x021b, 0x031a, 0x021a, 0x0319, 0x0219, 0x0318, 0x0218,
        0x0317, 0x0217, 0x0316, 0x0216, 0x0315, 0x0215, 0x0314, 0x0214,
        0x0313, 0x0213, 0x0312, 0x0212, 0x0311, 0x0211, 0x0310, 0x0210,
        0x030f, 0x020f, 0x030e, 0x020e, 0x030d, 0x020d, 0x030c, 0x020c,
        0x030b, 0x020b, 0x030a, 0x020a, 0x0309, 0x0209, 0x0109, 0x0009,/*E103*/

        0x010a, 0x000a, 0x010b, 0x000b, 0x010c, 0x000c, 0x010d, 0x000d,
        0x010e, 0x000e, 0x010f, 0x000f, 0x0110, 0x0010, 0x0111, 0x0011,
        0x0112, 0x0012, 0x0113, 0x0013, 0x0114, 0x0014, 0x0115, 0x0015,
        0x0116, 0x0016, 0x0117, 0x0017, 0x0118, 0x0018, 0x0119, 0x0019,
        0x011a, 0x001a, 0x011b, 0x001b, 0x011c, 0x001c, 0x011d, 0x001d,
        0x011e, 0x001e, 0x011f, 0x001f, 0x0120, 0x0020, 0x0121, 0x0021,/*E130*/

        0xffff
    }}, /* }}}2 */
    {-1,{0}},
}; /* }}} */

int main(int argc, char *argv[]) {
    u_int8_t	buffer[65536];
    int     i,y;
    hint_t   ver;

    for (i=0; data[i].ver != -1; i++) {
        ver.ver = data[i].ver;

        fprintf(stderr, "v%d ", ver.ver);
        qrSequence(ver,65535,buffer);
        for (y=0; data[i].qrLog[y] != 0xffff; y++) {
            if ((y & 7) == 0) {
                printf(".");
            }
            if (0) {
                printf("%3d,%3d got: %3d,%3d\n",
                    data[i].qrLog[y] >> 8, data[i].qrLog[y] & 255,
                    getLog()[y] >> 8, getLog()[y] & 255);
            }
            if (data[i].qrLog[y] != getLog()[y]) {
                printf("%04x != %04x\n", data[i].qrLog[y], getLog()[y]);
                assert(0);
            }
        }
    }
    printf("\n");

    return 0;
}