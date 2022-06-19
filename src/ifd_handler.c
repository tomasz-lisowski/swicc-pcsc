/**
 * IFD handler for PCSC-lite.
 */

#include "io.h"
#include <debuglog.h>
#include <ifdhandler.h>
#include <stdio.h>
#include <string.h>

#define IFD_SLOT_COUNT_MAX IO_CLIENT_COUNT_LEN
#define IFD_SERVER_PORT 37324U

static io_ctx_st io_ctx = {
    .sock_listen = -1,
    .sock_client = {-1},
};

/**
 * @brief Parse the Lun into a reader number and slot number and check that
 * these values are in sane ranges.
 * @param Lun Yhe Lun to parse.
 * @param reader_num Where to write the number of the reader.
 * @param slot_num Where to write the number of the slot.
 * @return 0 on valid Lun (success), -1 on invalid Lun (failure).
 */
static int32_t lun_parse(DWORD const Lun, DWORD *const reader_num,
                         DWORD *const slot_num)
{
    *reader_num = Lun & 0xFFFF0000;
    *slot_num = Lun & 0x0000FFFF;

    if (*slot_num >= IFD_SLOT_COUNT_MAX)
    {
        Log2(PCSC_LOG_ERROR,
             "Tried to create an unsupported slot: slot_num=%lu", *slot_num);
        return IFD_COMMUNICATION_ERROR;
    }
    if (*reader_num != 0U)
    {
        Log2(PCSC_LOG_ERROR,
             "Tried to create an unsupported reader: reader_num=%lu",
             *reader_num);
        return IFD_COMMUNICATION_ERROR;
    }
    return 0;
}

RESPONSECODE IFDHCreateChannelByName(DWORD const Lun, LPSTR const DeviceName)
{
    Log3(PCSC_LOG_DEBUG, "Lun=0x%04lX, DeviceName='%s'", Lun, DeviceName);

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    if (strncmp(DeviceName, DIR_PCSC_DEV, strlen(DIR_PCSC_DEV)) != 0)
    {
        Log3(PCSC_LOG_ERROR, "Expected device '%s' but got DeviceName='%s'",
             DIR_PCSC_DEV, DeviceName);
        return IFD_COMMUNICATION_ERROR;
    }
    if (io_init(&io_ctx, IFD_SERVER_PORT) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }
    return IFD_SUCCESS;
}

RESPONSECODE IFDHControl(DWORD Lun, DWORD dwControlCode, PUCHAR TxBuffer,
                         DWORD TxLength, PUCHAR RxBuffer, DWORD RxLength,
                         LPDWORD pdwBytesReturned)
{
    Log9(PCSC_LOG_DEBUG,
         "Lun=0x%04lX, dwControlCode=%lu, TxBuffer=0x%p, TxLength=%lu, "
         "RxBuffer=0x%p, RxLength=%lu, pdwBytesReturned=%p%c",
         Lun, dwControlCode, TxBuffer, TxLength, RxBuffer, RxLength,
         pdwBytesReturned, '\0');

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHCreateChannel(DWORD Lun, DWORD Channel)
{
    Log3(PCSC_LOG_DEBUG, "Lun=0x%04lX, Channel=%lu", Lun, Channel);
    /**
     * This method has been superseded by 'IFDHCreateChannelByName' so this will
     * remain unimplemented.
     */
    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHCloseChannel(DWORD Lun)
{
    Log2(PCSC_LOG_DEBUG, "Lun=0x%04lX", Lun);

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    if (io_fini(&io_ctx) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }
    return IFD_SUCCESS;
}

RESPONSECODE IFDHGetCapabilities(DWORD Lun, DWORD Tag, PDWORD Length,
                                 PUCHAR Value)
{
    Log5(PCSC_LOG_DEBUG, "Lun=0x%04lX, Tag=0x%04lX, Length=0x%p, Value=%p", Lun,
         Tag, Length, Value);

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    switch (Tag)
    {
    default:
        break;
    }
    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHSetCapabilities(DWORD Lun, DWORD Tag, DWORD Length,
                                 PUCHAR Value)
{
    Log5(PCSC_LOG_DEBUG, "Lun=0x%04lX, Tag=0x%04lX, Length=%lu, Value=0x%p",
         Lun, Tag, Length, Value);

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHSetProtocolParameters(DWORD Lun, DWORD Protocol, UCHAR Flags,
                                       UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
    Log9(PCSC_LOG_DEBUG,
         "Lun=0x%04lX, Protocol=%lu, Flags=%u, PTS1=%u, PTS2=%u, PTS3=%u%c%c",
         Lun, Protocol, Flags, PTS1, PTS2, PTS3, '\0', '\0');

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHPowerICC(DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
    Log5(PCSC_LOG_DEBUG, "Lun=0x%04lX, Action=%lu, Atr=0x%p, AtrLength=0x%p",
         Lun, Action, Atr, AtrLength);

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHTransmitToICC(DWORD Lun, SCARD_IO_HEADER SendPci,
                               PUCHAR TxBuffer, DWORD TxLength, PUCHAR RxBuffer,
                               PDWORD RxLength, PSCARD_IO_HEADER RecvPci)
{
    Log9(PCSC_LOG_DEBUG,
         "Lun=0x%04lX, SendPci=0x%p, TxBuffer=0x%p, TxLength=%lu, "
         "RxBuffer=0x%p, "
         "RxLength=0x%p, RecvPci=0x%p%c",
         Lun, &SendPci, TxBuffer, TxLength, RxBuffer, RxLength, RecvPci, '\0');

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHICCPresence(DWORD Lun)
{
    Log2(PCSC_LOG_DEBUG, "Lun=0x%04lX", Lun);

    DWORD reader_num;
    DWORD slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    if (io_ctx.sock_client[slot_num] >= 0)
    {
        return IFD_ICC_PRESENT;
    }
    else
    {
        if (io_client_connect(&io_ctx, &io_ctx.sock_client[slot_num]) == 0)
        {
            return IFD_ICC_PRESENT;
        }
        else
        {
            return IFD_ICC_NOT_PRESENT;
        }
    }
    /**
     * Never returning IFD_NO_SUCH_DEVICE because a software-only reader is
     * always present.
     */
}
