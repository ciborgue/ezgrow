#ifndef __QRENCODECALLBACK_H__
#define __QRENCODECALLBACK_H__

#define RESULT_OK               0
#define RESULT_BUFFER_TOO_SMALL 1
#define RESULT_TOO_MUCH_DATA    2

/*
 * 'hint' is normally set to 0. It is used to enforce the following:
 * +--------------------------------+
 * !                                !
 * !10987654321098765432109876543210!
 * +-------------------+---+--+-----+
 *                     !   !  !
 *                     !   !  +----- (minimum) Version [1..40]
 *                     !   +-------- ECC [1..4] for eccM..eccQ
 *                     +------------ XOR mask [1..8] for XOR mask 0..7
 *
*/
int qrEncodeCallback(int hint, char *url, int srcLen, int bufLen, char *buf);

/*******************************************************************************
 * Static functions with their descriptions
*******************************************************************************/
/*
 *      int qrDetectVersionECC(int hint, int src, int bufLen, char *buf);
 *
 * Detects the smallest possible version that fits given buffer.
 * Also, within the same version, detects the BEST possible ECC that doesn't
 * require version bump. Starts with the minimal requirement in version/ecc
 * and increases them until minimums are satisfied (of QRMAX is reached that
 * flags "TOO MUCH DATA" condition.
 *
 * Input:
 *  hint    - minimal requirements for the version/ECC
 *  srcIdx  - first byte of data
 *  bufLen  - total buffer length
 *  buf     - data buffer
 *
 * Return:
 *  ecc/version packed as in 'hint'
 *
 * Note0: on success buffer is filled with the bit sequence (no ECC, no padding)
 * Note1: It always encodes bitstring at the beginning of the buffer
 */
#endif
