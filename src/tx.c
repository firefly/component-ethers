#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "firefly-tx.h"

#include "firefly-cbor.h"
#include "firefly-rlp.h"


static FfxTxStatus mungeDataError(FfxDataError error) {
    switch (error) {
        case FfxDataErrorNone:
            return FfxTxStatusOK;
        case FfxDataErrorOverflow:
            return FfxTxStatusOverflow;
        case FfxDataErrorBufferOverrun:
            return FfxTxStatusBufferOverrun;
        default:
            break;
    }

    return FfxTxStatusBadData;
}

typedef enum Format {
    FormatData = 0,
    FormatNumber,
    FormatAddress,
    FormatNullableAddress,
} Format;


static FfxTxStatus append(FfxRlpBuilder *rlp, Format format, FfxCborCursor *tx,
  const char* key) {

    FfxCborCursor value = ffx_cbor_followKey(tx, key);
    if (value.error == FfxDataErrorNotFound) {
        ffx_rlp_appendData(rlp, NULL, 0);
        return mungeDataError(rlp->error);
    } else if (value.error) {
        return FfxTxStatusBadData;
    }

    if (!ffx_cbor_checkType(&value, FfxCborTypeData)) {
        return FfxTxStatusBadData;
    }

    FfxDataResult data = ffx_cbor_getData(&value);
    if (data.error) { return FfxTxStatusBadData; }

    const uint8_t *bytes = data.bytes;
    size_t length = data.length;

    // Consume any leading 0 bytes
    if (format == FormatNumber) {
        while (length) {
            if (bytes[0]) { break; }
            bytes++;
            length--;
        }
        if (length > 32) { return FfxTxStatusOverflow; }

    } else if (format == FormatAddress) {
        if (length != 20) { return FfxTxStatusBadData; }

    } else if (format == FormatNullableAddress) {
        if (length != 0 && length != 20) { return FfxTxStatusBadData; }
    }

    ffx_rlp_appendData(rlp, bytes, length);
    return mungeDataError(rlp->error);
}

#define readDataLength(NAME,CURSOR,LENGTH) \
    FfxDataResult (NAME) = { 0 }; \
    { \
        if (!ffx_cbor_checkType((CURSOR), FfxCborTypeData)) { \
            return FfxTxStatusBadData; \
        } \
        (NAME) = ffx_cbor_getData(CURSOR); \
        if ((NAME).error || (NAME).length != (LENGTH)) { \
            return FfxTxStatusBadData;  \
        } \
    }

#define checkArray(cbor) \
    do { \
        if (!ffx_cbor_checkType((cbor), FfxCborTypeArray)) { \
            return FfxTxStatusBadData; \
        } \
    } while(0);

#define checkArrayLength(cbor,length) \
    do { \
        if (!ffx_cbor_checkLength((cbor), FfxCborTypeArray, (length))) { \
            return FfxTxStatusBadData; \
        } \
    } while(0);


static FfxTxStatus appendAccessList(FfxRlpBuilder *rlp, FfxCborCursor *tx) {
    // Copy the access list (if any) to the RLP

    // If the accessList key is absent, use the default, an empty access list
    FfxCborCursor accessList = ffx_cbor_followKey(tx, "accessList");
    if (accessList.error == FfxDataErrorNotFound) {
        ffx_rlp_appendArray(rlp, 0);
        return mungeDataError(rlp->error);
    } else if (accessList.error) {
        return FfxTxStatusBadData;
    }

    checkArray(&accessList);

    size_t i = 0;
    FfxRlpBuilderTag iTag = ffx_rlp_appendMutableArray(rlp);
    if (rlp->error) { return mungeDataError(rlp->error); }

    FfxCborIterator iter = ffx_cbor_iterate(&accessList);
    while (ffx_cbor_nextChild(&iter)) {

        // Check: [ address, slots ]
        checkArrayLength(&iter.child, 2);

        ffx_rlp_appendArray(rlp, 2);
        if (rlp->error) { return mungeDataError(rlp->error); }

        // Check: X = [ data: 20 bytes ]
        FfxCborCursor address = ffx_cbor_followIndex(&iter.child, 0);
        if (address.error) { return FfxTxStatusBadData; }

        {
            readDataLength(data, &address, 20);
            ffx_rlp_appendData(rlp, data.bytes, data.length);
            if (rlp->error) { return mungeDataError(rlp->error); }
        }

        // Check: Y = [ ]
        FfxCborCursor slots = ffx_cbor_followIndex(&iter.child, 1);
        if (slots.error || !ffx_cbor_checkType(&slots, FfxCborTypeArray)) {
            return FfxTxStatusBadData;
        }

        size_t si = 0;
        FfxRlpBuilderTag siTag = ffx_rlp_appendMutableArray(rlp);
        if (rlp->error) { return mungeDataError(rlp->error); }

        FfxCborIterator iterSlots = ffx_cbor_iterate(&slots);
        while (ffx_cbor_nextChild(&iterSlots)) {
            readDataLength(data, &iterSlots.child, 32);

            ffx_rlp_appendData(rlp, data.bytes, data.length);
            if (rlp->error) { return mungeDataError(rlp->error); }

            si++;
            ffx_rlp_adjustCount(rlp, siTag, si);
        }

        i++;
        ffx_rlp_adjustCount(rlp, iTag, i);
    }

    if (iter.child.error) { return mungeDataError(iter.child.error); }

    return mungeDataError(rlp->error);
}



FfxTxStatus serialize1559(FfxCborCursor *tx, FfxRlpBuilder *rlp) {

    // The Unsigned EIP-1559 Tx has 9 fields
    if (!ffx_rlp_appendArray(rlp, 9)) { return mungeDataError(rlp->error); }

    FfxTxStatus status = FfxTxStatusOK;

    status = append(rlp, FormatNumber, tx, "chainId");
    if (status) { return status; }

    status = append(rlp, FormatNumber, tx, "nonce");
    if (status) { return status; }
    // @TODO: Copy nonce

    status = append(rlp, FormatNumber, tx, "maxPriorityFeePerGas");
    if (status) { return status; }

    status = append(rlp, FormatNumber, tx, "maxFeePerGas");
    if (status) { return status; }

    status = append(rlp, FormatNumber, tx, "gasLimit");
    if (status) { return status; }

    status = append(rlp, FormatNullableAddress, tx, "to");
    if (status) { return status; }

    status = append(rlp, FormatNumber, tx, "value");
    if (status) { return status; }

    status = append(rlp, FormatData, tx, "data");
    if (status) { return status; }

    //if (!ffx_rlp_appendArray(&rlp, 0)) {
    //    return mungeDataError(rlp.status);
    //}

    status = appendAccessList(rlp, tx);
    if (status) { return status; }

    return FfxTxStatusOK;
}

typedef struct ReadNumber {
    uint64_t value;
    FfxTxStatus status;
} ReadNumber;

ReadNumber readNumber(FfxCborCursor *tx, const char* key) {
    ReadNumber result = { 0 };

    FfxCborCursor follow = ffx_cbor_followKey(tx, key);
    if (follow.error) {
        return (ReadNumber){ .status = mungeDataError(follow.error) };
    } else if (!ffx_cbor_checkType(&follow, FfxCborTypeData)) {
        return (ReadNumber){ .status = FfxTxStatusBadData };
    }

    FfxDataResult data = ffx_cbor_getData(&follow);
    if (data.error) {
        result.status = mungeDataError(data.error);
        return result;
    }

    if (data.length > 8) {
        result.status = FfxTxStatusOverflow;
        return result;
    }

    for (int i = 0; i < data.length; i++) {
        result.value <<= 8;
        result.value |= data.bytes[i];
    }

    return result;
}

FfxTx ffx_tx_serializeUnsigned(FfxCborCursor *tx, uint8_t *data, size_t length) {
    FfxTx result = { 0 };

    {
        ReadNumber type = readNumber(tx, "type");
        if (type.value > 0x7f) { type.status = FfxTxStatusUnsupportedType; }
        if (type.status) {
            result.status = type.status;
            return result;
        }

        result.type = type.value;
    }

    switch (result.type) {
        case 2:
            break;
        default:
            result.status = FfxTxStatusUnsupportedType;
            return result;
    }

    // Add the EIP-2718 Envelope Type;
    if (length < 1) {
        result.status = FfxTxStatusBufferOverrun;
        return result;
    }
    data[0] = result.type;

    // Skip the Envelope Type during RLP output
    FfxRlpBuilder rlp = ffx_rlp_build(&data[1], length - 1);

    result.rlp.bytes = data;

    FfxTxStatus status = serialize1559(tx, &rlp);
    result.rlp.length = ffx_rlp_finalize(&rlp) + 1;
    if (status) { result.status = status; }

    return result;
}
