#pragma once
/**
 * This is here only as a reference/summary of the standard.
 */

#include <assert.h>
#include <stdint.h>

/* Test for GCC >= 12.1.0. */
#if __GNUC__ < 12 || (__GNUC__ == 12 && (__GNUC_MINOR__ < 0) ||                \
                      (__GNUC_MINOR__ == 0 && __GNUC_PATCHLEVEL__ < 1))
#error                                                                         \
    "Compilcation requires GCC 12.0.1 or newer because previous versions don't have the 'unavailable' enumerator attribute."
#endif

/**
 * The overview of the IFD interface is specified in
 * http://pcscworkgroup.com/Download/Specifications/pcsc1_v2.01.01.pdf
 * and the other parts of the "Interoperability Specification for ICCs and
 * Personal Computer Systems" document (10 parts in total) specify the details.
 *
 * Glossary of terms (IFD-specific):
 * IFD  = Interface Device
 * IFS  = Information filed size associated with T=1 protocol.
 * IFSD = IFS for the terminal
 * IFSC = IFS for the ICC
 * BWT  = Block waiting time
 * CWT  = Charadcter waiting time
 * EDC  = Error detection code
 * PTS  = Protocol type selection
 *
 * Glossary of terms (ICC-specific):
 * ICC = Integrated circuit card
 * ATR = Answer-to-reset. "The transmission sent by an ICC to the reader in
 *       response to a RESET condition."
 */

#error                                                                         \
    "The generic IFD handler shall only serve as a reference, not be used in an implementation. This is because PC/SC resource manager implementations may alter the functions and their parameters."

/**
 * Used in method that sets protocol parameters. Defined in PCSC-3 v2.01.09
 * pg.36 sec.4.3.1.3.
 */
#define IFD_DEFAULT_PROTOCOL ((uint32_t)0x80000000)

/* These types are defined in PCSC-9 v2.01.01 pg.6 sec.4.1.2. */
typedef uint8_t ifd_byte_kt;
typedef uint16_t ifd_ushort_kt;
typedef int16_t ifd_bool_kt;
typedef uint32_t ifd_dword_kt;
typedef char *ifd_str_kt;
typedef char const *ifd_cstr_kt;
typedef char (*ifd_str32_kt)[32U];
typedef char const (*ifd_cstr32_kt)[32U];
typedef uint8_t ifd_guid_kt[16U];
typedef uint8_t ifd_iid_kt[16U];
typedef uint8_t ifd_acid_kt[16U];
typedef int32_t ifd_responsecode_kt;
typedef uint32_t ifd_handle_kt;
/*  */
typedef uint16_t ifd_word_kt;

/**
 * Interface device capabilities are defined in PCSC-3 v2.01.09 pg.6 sec.3.1.1.2
 * table.3-1.
 * @note The string pointers don't all contain 32 bytes, this is their maximum
 * size. Better this than nothing.
 * @note The 'tag' is just the BER-TLV tag.
 */
typedef struct ifd_dev_cap_s
{
    ifd_cstr32_kt vendor_name;          /**
                                         * Tag     = 0x0100
                                         * Len Max = 32B
                                         * Data    = ASCII string
                                         */
    ifd_cstr32_kt ifd_type;             /**
                                         * Tag     = 0x0101
                                         * Len Max = 32B
                                         * Data    = ASCII string
                                         */
    ifd_dword_kt ifd_ver;               /**
                                         * Tag     = 0x0102
                                         * Len Max = 4B
                                         * Data    = Vendor-specified IFD version number.
                                         * Format  = 0xMMmmbbbb
                                         *   0xMM   = Major ver
                                         *   0xmm   = Minor ver
                                         *   0xbbbb = Build number
                                         */
    ifd_cstr32_kt ifd_serial;           /**
                                         * Tag     = 0x0103
                                         * Len Max = 32B
                                         * Data    = ASCII string
                                         */
    ifd_dword_kt chan_id;               /**
                                         * Tag     = 0x0110
                                         * Len Max = 4B
                                         * Data    = Channel ID.
                                         * Format  = 0xDDDDCCCC
                                         *   0xDDDD = Data channel type
                                         *   0xCCCC = Channel number
                                         *
                                         * Possible data:
                                         *   0x0001 + port = Serial I/O on port.
                                         *   0x0002 + port = Parallel I/O on port.
                                         *   0x0004 + 0x0000 = PS/2 keyboard.
                                         *   0x0008 + SCSI ID number = SCSI with given ID.
                                         *   0x0010 + Device number = IDE with device.
                                         *   0x0020 + Device number = USB with device.
                                         *   0x00F0 + y = Vendor-defined interface with y 0
                                         *     to 15. y also vendor-defined.
                                         */
    ifd_dword_kt support_card_async;    /**
                                         * Tag     = 0x0120
                                         * Len Max = 4B
                                         * Data    = ISO 7816 (async) / 14443 /
                                         * 15694 cards. Indicates potential range
                                         * (not range of connected ICC).
                                         * Format  = 0x00RRPPPP
                                         *   RR   = RFU (shall be 0x00)
                                         *   PPPP = Encodes supported protocol.
                                         *     A 1 in a bit indicates the
                                         *     associated ISO protocol is
                                         *     supported. 0x00000003 indicates T=0
                                         *     and T=1 are supported. This is the
                                         *     only compliant value that may be
                                         *     returned by contact devices.
                                         *     Contactless devices may return
                                         *     0x00010000.
                                         *   All other values are outside of the
                                         *   specification.
                                         *
                                         * @note For an IFD for ISO 7816-compatible
                                         * ICCs, support for T=0 AND T=1 is only
                                         * required at a default clock frequency.
                                         * (PCSC3 v2.01.09 pg.12 sec.3.1.1.4)
                                         */
    ifd_dword_kt clock_def;             /**
                                         * Tag     = 0x0121
                                         * Len Max = 4B
                                         * Data    = Default ICC clock frequency in KHz
                                         *   encoded as little endian integer value.
                                         */
    ifd_dword_kt clock_max;             /**
                                         * Tag     = 0x0122
                                         * Len Max = 4B
                                         * Data    = Maximum supported ICC clock frequency
                                         *   in KHz encoded as little endian integer value.
                                         */
    ifd_dword_kt data_rate_def;         /**
                                         * Tag     = 0x0123
                                         * Len Max = 4B
                                         * Data    = Default ICC I/O data rate in bps
                                         *   encoded as little endian integer.
                                         */
    ifd_dword_kt data_rate_max;         /**
                                         * Tag     = 0x0124
                                         * Len Max = 4B
                                         * Data    = Maximum supported ICC I/O data rate
                                         *   in bps.
                                         */
    ifd_dword_kt ifsd_max;              /**
                                         * Tag     = 0x0125
                                         * Len Max = 4B
                                         * Data    = Maximum IFSD supported by IFD.
                                         */
    ifd_dword_kt support_proto_sync;    /**
                                         * Tag     = 0x0126
                                         * Len Max = 4B
                                         * Data    = Synchronous protocol types
                                         *   supported for ISO 7816-compatible
                                         *   ICCs. Indicates potential range (not
                                         *   range of connected ICC).
                                         * Format  = 0x40RRPPPP
                                         *   RR   = RFU (shall be 0x00)
                                         *   PPPP = Encodes the supported protocol
                                         *     types. A 1 in a given bit position
                                         *     indicates support for the associated
                                         *     protocol.
                                         *
                                         * Possible data:
                                         *   0x40000001 = Support for 2-wire
                                         *     protocol.
                                         *   0x40000002 = Support for 3-wire
                                         *     protocol.
                                         *   0x40000004 = Support for I2C-bus
                                         *     protocol.
                                         *
                                         * @note For an IFD for ISO 7816-compatible
                                         * ICCs, support for T=0 AND T=1 is only
                                         * required at a default clock frequency
                                         * (PCSC3 v2.01.09 pg.12 sec.3.1.1.4).
                                         */
    ifd_dword_kt support_power_mgmt;    /**
                                         * Tag     = 0x0131
                                         * Len Max = 4B
                                         * Data    = Power management supported.
                                         *
                                         * Format  = If 0, device does not support
                                         *   power down while ICC inserted. If
                                         *   non-zero, device does support it.
                                         */
    ifd_dword_kt user_to_card_auth_dev; /**
                                         * Tag     = 0x0140
                                         * Len Max = 4B
                                         * Data    = User to card authentication
                                         *   devices.
                                         * Format  = OR of flags.
                                         *
                                         * Flags:
                                         *   0x00000000 = No devices
                                         *   0x00000001 = RFU
                                         *   0x00000002 = Numeric pad
                                         *   0x00000004 = Keyboard
                                         *   0x00000008 = Fingerprint
                                         *   0x00000010 = Retinal scanner
                                         *   0x00000020 = Image scanner
                                         *   0x00000040 = Voice print scanner
                                         *   0x00000080 = Display device
                                         *   0x0000DD00 = 0xDD is vendor
                                         * selected for vendor-defined device.
                                         */
    ifd_dword_kt user_auth_input_dev;   /**
                                         * Tag     = 0x0142
                                         * Len Max = 4B
                                         * Data    = User authentication input
                                         *   device.
                                         * Format  = OR of flags.
                                         *
                                         * Flags:
                                         *   0x00000000 = No devices
                                         *   0x00000001 = RFU
                                         *   0x00000002 = Numeric pad
                                         *   0x00000004 = Keyboard
                                         *   0x00000008 = Fingerprint
                                         *   0x00000010 = Retinal scanner
                                         *   0x00000020 = Image scanner
                                         *   0x00000040 = Voice print scanner
                                         *   0x00000080 = Display device
                                         *   0x0000DD00 = 0xDD is vendor selected
                                         *     for vendor-defined device. It's in
                                         *     the range 0x01 to 0x40.
                                         *   0x00008000 = Used to indicate that
                                         *     encrypted input is supported.
                                         */
    ifd_dword_kt support_mechanics;     /**
                                         * Tag     = 0x0150
                                         * Len Max = 4B
                                         * Data    = Mechanical characteristics
                                         *   supported.
                                         * Format  = OR of flags.
                                         *
                                         * Flags:
                                         *   0x00000000 = No special characteristics
                                         *   0x00000001 = Card swallowing mechanism
                                         *   0x00000002 = Card ejection mechanism
                                         *   0x00000004 = Card capture mechanism
                                         *   0x00000008 = Contactless
                                         *   All other values are RFU.
                                         */
    ifd_dword_kt vendor_features;       /**
                                         * Tag     = 0x0180 to 0x01F0 (user defined)
                                         * Len Max = ?
                                         * Data    = Vendor defined features.
                                         */
} ifd_dev_cap_st;

/**
 * Codes for enumnerating ICC state defined in PCSC-3 v2.01.09 p.12 sec.3.1.1.4
 * table.3-2.
 */
typedef struct ifd_icc_state_s
{
    ifd_byte_kt icc_presence;         /**
                                       * Tag     = 0x0300
                                       * Len Max = 1B
                                       * Data    = ICC presence.
                                       *
                                       * Possible data:
                                       *   0 = Not present
                                       *   1 = Card present but not swallowed (only
                                       *     applies if IFD support ICC swallowing)
                                       *   2 = Card present (and swallowed if the IFD
                                       *     supports ICC swallowing)
                                       *   4 = Card confiscated
                                       */
    ifd_byte_kt icc_interface_status; /**
                                       * Tag     = 0x0301
                                       * Len Max = 1B (boolean)
                                       * Data    = ICC interface status for
                                       ISO
                                       *   7816-compatible cards only.
                                       *
                                       * Possible data:
                                       *   0 = Contact inactive
                                       *   1 = Contact active
                                       *
                                       * @note IFD subsystem will provide a mean
                                       * to reinitialize an ICC using a warm
                                       * reset on application to ensure ICC is
                                       * in a known state (only for ISO
                                       * 7816-compatible cards).
                                       */
    ifd_cstr32_kt atr;                /**
                                       * Tag     = 0x0303
                                       * Len Max = 32B
                                       * Data    = ATR string.
                                       */
    ifd_byte_kt icc_type;             /**
                                       * Tag     = 0x0304
                                       * Len Max = 1B
                                       * Data    = ICC type.
                                       *
                                       * Possible data:
                                       *   0 = Unknown ICC type
                                       *   1 = ISO 7816 asynchronous
                                       *   2 = ISO 7816 synchronous
                                       *   3 = ISO 7816-10 synchronous type 1
                                       *   4 = ISO 7816-10 synchronous type 2
                                       *   5 = ISO 14443 type A
                                       *   6 = ISO 14443 type B
                                       *   7 = ISO 15693
                                       *   Other values are RFU.
                                       */
} ifd_icc_state_st;

/**
 * Codes for enumerating interface device protocol options from PCSC-3 v2.01.09
 * pg.15 sec.3.1.2.1.1 table.3-3.
 */
typedef struct ifd_proto_opt_s
{
    ifd_dword_kt cur_proto_type; /**
                                  * Tag       = 0x0201
                                  * Len Max   = 4B
                                  * Data      = Current protocol type.
                                  * Read-only = Yes
                                  * Format    = Same encoding as "available
                                  *   protocol types". It's illegal to specify
                                  *   more than a single protocol in this value.
                                  */
    ifd_dword_kt cur_clk;        /**
                                  * Tag       = 0x0202
                                  * Len Max   = 4B
                                  * Data      = Current ICC clock frequency in KHz
                                  *   encoded as a little endian integer value.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_f;          /**
                                  * Tag       = 0x0203
                                  * Len Max   = 4B
                                  * Data      = Current clock conversion factor. Encoded
                                  *   as a little endian integer. It may be modified
                                  * through PPS.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_d;          /**
                                  * Tag       = 0x0204
                                  * Len Max   = 4B
                                  * Data      = Current bit rate conversion factor.
                                  *   Encoded as a little endian integer. It may be
                                  *   modified through PPS.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_n;          /**
                                  * Tag       = 0x0205
                                  * Len Max   = 4B
                                  * Data      = Current guard time factor. Encoded as a
                                  *   little endian integer. It may be modified through
                                  * PPS.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_w;          /**
                                  * Tag       = 0x0206
                                  * Len Max   = 4B
                                  * Data      = Current work waiting time. Encoded as a
                                  *   little endian integer. Only valid if current
                                  *   protocol is set to T=0.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_ifsc;       /**
                                  * Tag       = 0x0207
                                  * Len Max   = 4B
                                  * Data      = Current information field size card.
                                  *   Encoded as a little endian integer. Only valid
                                  *   if current protocol is T=1.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_ifsd;       /**
                                  * Tag       = 0x0208
                                  * Len Max   = 4B
                                  * Data      = Current information field size reader.
                                  *   Only valid if current protocol is T=1.
                                  * Read-only = No
                                  */
    ifd_dword_kt cur_bwt;        /**
                                  * Tag       = 0x0209
                                  * Len Max   = 4B
                                  * Data      = Current block waiting time. Only valid
                                  *   if current protocol is T=1.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_cwt;        /**
                                  * Tag       = 0x020A
                                  * Len Max   = 4B
                                  * Data      = Current character waiting time. Only
                                  *   valid if current protocol is T=1.
                                  * Read-only = Yes
                                  */
    ifd_dword_kt cur_edc;        /**
                                  * Tag       = 0x020B
                                  * Len Max   = 4B
                                  * Data      = Current EDC encoding. Only valid if
                                  *   current protocol is T=1.
                                  * Read-only = Yes
                                  *
                                  * Possible data:
                                  *   0 = LRC
                                  *   1 = CRC
                                  *
                                  * @note There is an errata in the specification.
                                  * This field is incorrectly called 'EBC' in PCSC-3
                                  * v2.01.09.
                                  */
} ifd_proto_opt_st;

typedef enum ifd_dev_type_e
{
    IFD_DEV_TYPE_SLOT = 1U,
    IFD_DEV_TYPE_FUNCTIONAL = 2U,

    /* Force enum to be 32 bits wide. */
    IFD_DEV_TYPE_MAX __attribute__((unavailable)) = UINT32_MAX
} ifd_dev_type_et;
static_assert(sizeof(ifd_dev_type_et) == sizeof(ifd_dword_kt),
              "IFD dev type enum is not equal to width of DWORD.");

/**
 * Device descriptor from PCSC-3 v2.01.09 pg.34 sec.4.2.
 */
typedef struct ifd_dev_descr_s
{
    ifd_str_kt dev_name;
    ifd_dword_kt dev_type;
} __attribute__((packed)) ifd_dev_descr_st;

/**
 * Smart card I/O header from PCSC-3 v2.01.09 pg.41 sec.4.3.2.1.
 */
typedef struct ifd_scard_io_hdr_s
{
    ifd_dword_kt proto;
    /* The length parameter is the size in bytes of this structure.  */
    ifd_dword_kt len;
} __attribute__((packed)) ifd_scard_io_hdr_st;

/**
 * Command/response data from PCSC-3 v2.01.09 pg.41 sec.4.3.2.1.
 */
typedef struct ifd_data_s
{
    ifd_scard_io_hdr_st scard_io_hdr;
    /* Contains the data to be (sent to)/(received from) the card. */
    ifd_byte_kt proto_data[];
} __attribute__((packed)) ifd_data_st;

/**
 * Power management actions on an ISO 7816 card with contacts from PCSC-3
 * v2.01.09 pg.38 sec.4.3.1.4 table.3-22.
 */
typedef enum ifd_icc_act_e
{
    /* Requests activation of the contact. */
    IFD_ICC_ACT_POWER_ON __attribute__((deprecated)),
    /* Requests deactivation of the contact. */
    IFD_ICC_ACT_POWER_DOWN,
    /* Requests a warm reset of the ICC. */
    IFD_ICC_ACT_RESET,
    /* Requests a cold reset of the ICC. */
    IFC_ICC_ACT_COLD_RESET,
} ifd_icc_act_et;

typedef enum ifd_pts_nego_e
{
    IFD_PTS_NEGO_1 = 1 << 0,
    IFD_PTS_NEGO_2 = 1 << 1,
    IFD_PTS_NEGO_3 = 1 << 2,
} ifd_pts_nego_et;

/**
 * Equivalent to the RESPONSECODE from PCSC-3 v2.01.09.
 */
typedef enum ifd_ret_e
{
    IFD_RET_IFD_SUCCESS,
    IFD_RET_IFD_ERROR_TAG,
    IFD_RET_IFD_ERROR_SET_FAILURE,
    IFD_RET_IFD_ERROR_VALUE_READ_ONLY,
    IFD_RET_IFD_ERROR_PTS_FAILURE,
    IFD_RET_IFD_ERROR_NOT_SUPPORTED,
    IFD_RET_IFD_PROTOCOL_NOT_SUPPORTED,
    IFD_RET_IFD_ERROR_POWER_ACTION,
    IFD_RET_IFD_ERROR_SWALLOW,
    IFD_RET_IFD_ERROR_EJECT,
    IFD_RET_IFD_ERROR_CONFISCATE,
    IFD_RET_IFD_ERROR_COMMUNICATION,
    IFD_RET_IFD_RESPONSE_TIMEOUT,
    IFD_RET_DEVICE_SUCCESS,
    IFD_RET_DEVICE_ERROR_NOT_SUPPORTED,

    /* Force enum to be 32 bits wide. */
    IFD_RET_MAX __attribute__((unavailable)) = UINT32_MAX
} ifd_ret_et;
static_assert(sizeof(ifd_ret_et) == sizeof(ifd_responsecode_kt),
              "IFD ret enum is not equal to RESPONSECODE width.");

/**
 * Below are function declarations for the standard set of IFD handler functions
 * from PCSC-3 v2.01.09.
 */

/**
 * @brief The addressed logical device (slot or functional) lists all the
 * descriptors of its sibling logical devices (slot and functional) exposed for
 * the same physical device (IFD), including itself. This method is useful for
 * an IFD Service Provider when present. It is via this method that the Resource
 * Manager can get access to all the functional logical devices.
 * @param[out] dev_list List of all the device descriptors exposed by the given
 * IFD.
 * @return DEVICE_SUCCESS (the request was successful) or
 * DEVICE_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.2.
 */
ifd_responsecode_kt Device_List_Devices(ifd_dev_descr_st dev_list[]);

/**
 * @brief This method supports a direct and generic communication channel with
 * any slot or functional logical device. It is the responsibility of the vendor
 * to define control code values and input/ouput data associated with these
 * features.
 * @param[in] control_code Vendor-defined control code.
 * @param[in] buf_rx Input data buffer.
 * @param[out] buf_tx Output data buffer.
 * @return DEVICE_SUCCESS (the request was successful),
 * DEVICE_ERROR_NOT_SUPPORTED (function not supported), or
 * DEVICE_WRONG_PARAMETER (a parameter is not supported or invalid).
 * @note PCSC-3 v2.01.09 sec.4.2.
 */
ifd_responsecode_kt Device_Control(ifd_dword_kt const control_code,
                                   ifd_byte_kt const *const buf_rx,
                                   ifd_byte_kt *const buf_tx);

/**
 * @brief This instructs the IFD Handler to retrieve the value corresponding to
 * the specified Tag parameter. This enables the calling application to retrieve
 * any of the information described from the following TLV structures:
 * - Reader Capabilities (table 3-1).
 * - ICC Interface State (table 3-2).
 * - Protocol Parameters (tables 3-3, 3-4, 3-5, 3-6).
 * @param[in] tag
 * @param[out] val
 * @return IFD_SUCCESS (value was successfully retrieved), or IFD_ERROR_TAG (tag
 * does not exist).
 * @note PCSC-3 v2.01.09 sec.4.3.1.1.
 */
ifd_responsecode_kt IFD_Get_Capabilities(ifd_dword_kt const tag,
                                         ifd_byte_kt *const val);

/**
 * @brief The IFD Handler will attempt to set the parameter specified by Tag to
 * Value. This function can be used by a calling service provider to set
 * parameters such as the current IFSD, or to request an extension of the Block
 * Waiting Time.
 * @param[in] tag
 * @param[out] val
 * @return IFD_SUCCESS (parameter was successfully set), IFD_ERROR_SET_FAILURE
 * (operation failed), IFD_ERROR_TAG (tag does not exist), or
 * IFD_ERROR_VALUE_READ_ONLY (the value cannot be modified).
 * @note PCSC-3 v2.01.09 sec.4.3.1.2.
 */
ifd_responsecode_kt IFD_Set_Capabilities(ifd_dword_kt const tag,
                                         ifd_byte_kt *const val);

/**
 * @brief This function should only be called by the ICC Resource Manager layer.
 * A Service Provider simply specifies its preferred protocols and protocol
 * parameters, if any, when connecting to an ICC, and lets the ICC Resource
 * Manager check if it is able to negotiate one of the preferred protocols based
 * on the ATR received from the ICC, and the IFD capabilities.
 * @param[in] proto_type Can be a list of protocol types (like for tags 0x0120
 * and 0x0126) or special value IFD_DEFAULT_PROTOCOL.
 * @param[in] flags_sel Indicates which of the optional parameters (PPS1, PPS2
 * and PPS3), if any, have to be negotiated and included in the PPS request. It
 * is obtained by performing a bitwise OR operation on members of the PTS
 * negotiation enumeration.
 * @param[in] pps1 For ISO 7816, encodes clock conversion and bit duration
 *   factors.
 * @param[in] pps2
 * @param[in] pps3
 * @return IFD_SUCCESS (PTS succeeded), IFD_ERROR_PTS_FAILURE (PTS failed),
 * IFD_ERROR_NOT_SUPPORTED (PTS not supported), or IFD_PROTOCOL_NOT_SUPPORTED
 * (protocol not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.1.3.
 */
ifd_responsecode_kt IFD_Set_Protocol_Parameters(ifd_dword_kt const proto_type,
                                                ifd_byte_kt const flags_sel,
                                                ifd_byte_kt const pps1,
                                                ifd_byte_kt const pps2,
                                                ifd_byte_kt const pps3);

/**
 * @brief This function is used to power up, power down, or reset the ICC. The
 * desired action is specified by the action parameter.
 * @param[in] act The requested action. Shall be a member of the IFd ICC action
 * enumeration.
 * @return IFD_SUCCESS (performed action), IFD_ERROR_POWER_ACTION (the requested
 * action could not be carried out), or IFD_ERROR_NOT_SUPPORTED (one of the
 * requested actions is not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.1.4.
 * @note If the function reports success and the action requested was either a
 * reset or a power-up, then the ATR returned by the card and the Protocol
 * Parameters can be accessed through the IFD_Get_Capabilities function.
 */
ifd_responsecode_kt IFD_Power_ICC(ifd_word_kt const act);

/**
 * @brief This function causes a mechanical swallow of the ICC, if the IFD
 * supports such a feature.
 * @return IFD_SUCCESS (card successfully swallowed), IFD_ERROR_SWALLOW (card
 * not swallowed), or IFD_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.1.5.
 */
ifd_responsecode_kt IFD_Swallow_ICC();

/**
 * @brief This function causes a mechanical ejection of the ICC, if the IFD
 * supports such a feature.
 * @return IFD_SUCCESS (card successfully ejected), IFD_ERROR_EJECT (card not
 * ejected), or IFD_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.1.5.
 */
ifd_responsecode_kt IFD_Eject_ICC();

/**
 * @brief This function causes the IFD to confiscate the ICC, if the IFD
 * supports such a feature.
 * @return IFD_SUCCESS (card successfully confiscated), IFD_ERROR_CONFISCATE
 * (card not confiscated), or IFD_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.1.5.
 */
ifd_responsecode_kt IFD_Confiscate_ICC();

/**
 * @brief This instructs the IFD Handler to send to the ICC the command
 * specified in the RX data parameter and return the response of the ICC in
 * the TX data parameter.
 * @param[in] data_rx Command data. Contains a command APDU.
 * @param[out] data_tx Response data. Contains response APDU.
 * @return IFD_SUCCESS (the request was successfully sent to the ICC),
 * IFD_ERROR_COMMUNICATION (the request could not be sent to the ICC), or
 * IFD_RESPONSE_TIMEOUT (the IFD timed-out waiting the response from the ICC).
 * @note PCSC-3 v2.01.09 sec.4.3.2.1.
 * @note If the parameter does not match a valid entry in the table, the IFD
 * handler must fail the call with the return code IFD_ERROR_COMMUNICATION.
 * @note The protocol can be:
 * ISO 7816-3 T=0: SCARD_PROTOCOL_T0.
 * ISO 7816-3 T=1: SCARD_PROTOCOL_T1 or SCARD_PROTOCOL_APPDATA.
 * ISO 7816-10: SCARD_PROTOCOL_APPDATA.
 * @note For T=0, there are 3 cases of data encoding:
 * 1. CLA INS P1 P2.
 * 2. CLA INS P1 P2 Le.
 * 3. CLA INS P1 P2 Lc Data.
 * For all cases, Lc and Le are one byte. Incoming APDUs with no data byte (P3 =
 * 0) should be sent using case 1. A case 4 APDU with both Lc and Le is not
 * supported and a GET RESPONSE must be used if needed.
 * @note For the other protocols, look at PCSC-3 v2.01.09 pg.42 sec.4.3.2.1.
 */
ifd_responsecode_kt IFD_Transmit_to_ICC(ifd_data_st const *const data_rx,
                                        ifd_data_st *const data_tx);

/**
 * @brief Asynchronously signals the insertion of an ICC in the Interface
 * Device.
 * @return Not specified.
 * @note PCSC-3 v2.01.09 sec.4.3.2.2.
 */
ifd_responsecode_kt IFD_Is_ICC_Present();

/**
 * @brief Asynchronously signals the removal of the ICC from the Interface
 * Device.
 * @return Not specified.
 * @note PCSC-3 v2.01.09 sec.4.3.2.2.
 */
ifd_responsecode_kt IFD_Is_ICC_Absent();

/**
 * @brief Returns the list of the application contexts by their ACIDs that a IFD
 * supports.
 * @param[out] app_ctx_list Application context list.
 * @return DEVICE_SUCCESS (the request was successful) or
 * DEVICE_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.3.
 */
ifd_responsecode_kt IFD_List_Contexts(ifd_acid_kt *const app_ctx_list);

/**
 * @brief Returns if a particular application context referenced by its ACID is
 * supported or not by the IFD.
 * @param[in] app_ctx Application context.
 * @param[out] res_support Result supported. True if result is supported, false
 * otherwise.
 * @return DEVICE_SUCCESS (the request was successful) or
 * DEVICE_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.3.
 * @note There is an errata in the standard. The result support param shall be a
 * pointer, otherwise it can't be an out param.
 */
ifd_responsecode_kt IFD_Is_Context_Supported(ifd_acid_kt const app_ctx,
                                             ifd_bool_kt *const res_support);

/**
 * @brief Returns the reference to the IFD Service Provider if the reader
 * supports it.
 * @param[out] ifdsp_id Unique identifier for the IFD Service Provider.
 * @return DEVICE_SUCCESS (the request was successful) or
 * DEVICE_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.3.
 * @note There is an errata in the standard. The IFDSP ID param shall be a
 * pointer, otherwise it can't be an out param.
 */
ifd_responsecode_kt IFD_GetIFDSP(ifd_guid_kt const ifdsp_id);

/**
 * @brief Lists the interfaces (referenced by GUID) which are supported by the
 * IFD and exposed by its IFD Service Provider.
 * @param[out] interfaces List of interfaces supported by the IFDSP.
 * @return DEVICE_SUCCESS (the request was successful) or
 * DEVICE_ERROR_NOT_SUPPORTED (function not supported).
 * @note PCSC-3 v2.01.09 sec.4.3.3.
 */
ifd_responsecode_kt IFD_ListInterfaces(ifd_iid_kt *const interfaces);
