#ifndef TinyFrame_CRC_HPP
#define TinyFrame_CRC_HPP

#include "TinyFrame_Types.hpp"
#include "TF_Config.hpp"

namespace TinyFrame_n{
//region Checksums

// Custom checksum functions

/**
 * Initialize a checksum
 *
 * @return initial checksum value
 */
template<TF_CKSUM_t TF_CKSUM_TYPE>
TF_CKSUM<TF_CKSUM_TYPE> TF_CksumStart(void);

/**
 * Update a checksum with a byte
 *
 * @param cksum - previous checksum value
 * @param byte - byte to add
 * @return updated checksum value
 */
template<TF_CKSUM_t TF_CKSUM_TYPE>
TF_CKSUM<TF_CKSUM_TYPE> TF_CksumAdd(TF_CKSUM<TF_CKSUM_TYPE> cksum, uint8_t byte);

/**
 * Finalize the checksum calculation
 *
 * @param cksum - previous checksum value
 * @return final checksum value
 */
template<TF_CKSUM_t TF_CKSUM_TYPE>
TF_CKSUM<TF_CKSUM_TYPE> TF_CksumEnd(TF_CKSUM<TF_CKSUM_TYPE> cksum);

//endregion
}

#endif // TinyFrame_CRC_HPP