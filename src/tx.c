#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "firefly-tx.h"

#include "firefly-cbor.h"
#include "firefly-rlp.h"


typedef struct TxBuilder {
    uint8_t *data;
    size_t offset;
    size_t length;
} TxBuilder;


static FfxTxStatus mungeRlpStatus(FfxRlpStatus status) {
    switch(status) {
        case FfxRlpStatusOK:
            return FfxTxStatusOK;
        case FfxRlpStatusBufferOverrun:
            return FfxTxStatusBufferOverrun;
        case FfxRlpStatusOverflow:
            return FfxTxStatusOverflow;
    }

    return FfxTxStatusBadData;
}

static FfxTxStatus mungeCborStatus(FfxCborStatus status) {
    switch(status) {
        case FfxCborStatusOK:
            return FfxTxStatusOK;
        case FfxCborStatusBufferOverrun:
            return FfxTxStatusBufferOverrun;
        case FfxCborStatusOverflow:
            return FfxTxStatusOverflow;
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

    FfxCborCursor value;
    ffx_cbor_clone(&value, tx);

    FfxCborStatus error = FfxCborStatusOK;

    if (!ffx_cbor_followKey(&value, key, &error)) {
        if (error) { return FfxTxStatusBadData; }
        ffx_rlp_appendData(rlp, NULL, 0);
        return mungeRlpStatus(rlp->status);
    }

    if (!ffx_cbor_checkType(&value, FfxCborTypeData)) {
        return FfxTxStatusBadData;
    }

    FfxCborData data = ffx_cbor_getData(&value, &error);
    if (error) { return FfxTxStatusBadData; }

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
    return mungeRlpStatus(rlp->status);
}

/*
static FfxTxStatus appendAccessListItem(FfxRlpBuilder *rlp,
  FfxCborCursor *item) {

    if (ffx_cbor_getType(item) != FfxCborTypeArray ||
      !ffx_cbor_isLength(item, 2)) { return FfxTxStatusBadData; }

    FfxRlpStatus rlpStatus = ffx_rlp_appendArray(rlp, 2);
    if (rlpStatus) { return mungeRlpStatus(rlpStatus); }

    {
        FfxCborCursor address = { 0 };
        ffx_cbor_clone(&address, item);
        FfxCborStatus cborStatus = ffx_cbor_followIndex(&address, 0);
        if (cborStatus) { return mungeCborStatus(cborStatus); }

        if (ffx_cbor_getType(&address) != FfxCborTypeData ||
          !ffx_cbor_isLength(&address, 20)) { return FfxTxStatusBadData; }

        const uint8_t *data = NULL;
        cborStatus = ffx_cbor_getData(&address, &data, NULL);
        if (cborStatus || data == NULL) { return FfxTxStatusBadData; }

        ffx_rlp_appendData(rlp, data, 20);
    }

    {
        FfxCborCursor storage = { 0 };
        ffx_cbor_clone(&storage, item);
        FfxCborStatus cborStatus = ffx_cbor_followIndex(&storage, 1);
        if (cborStatus) { return mungeCborStatus(cborStatus); }

        if (ffx_cbor_getType(&storage) != FfxCborTypeArray) {
            return FfxTxStatusBadData;
        }

        size_t count = 0;
        cborStatus = ffx_cbor_getLength(&storage, &count);
        if (cborStatus) { return FfxTxStatusBadData; }

        ffx_rlp_appendArray(rlp, count);

        FfxCborStatus error = 33; //FfxCborStatusOK;
        while (ffx_cbor_iterate(&storage, NULL, &error)) {
            if (ffx_cbor_getType(&storage) != FfxCborTypeData ||
              !ffx_cbor_isLength(&storage, 32)) { return FfxTxStatusBadData; }

            const uint8_t *data = NULL;
            cborStatus = ffx_cbor_getData(&storage, &data, NULL);
            if (cborStatus || data == NULL) { return FfxTxStatusBadData; }

            ffx_rlp_appendData(rlp, data, 32);
        }

        if (error) { return mungeCborStatus(error); }
    }

    return FfxTxStatusOK;
}
*/
/*
static FfxTxStatus appendAccessList(FfxRlpBuilder *rlp, FfxCborCursor *tx) {

    // Copy the access list (if any) to the RLP

    FfxCborStatus error = FfxCborStatusOK;

    FfxCborCursor accessList = { 0 };
    ffx_cbor_clone(&accessList, tx);
    if (!ffx_cbor_followKey(&accessList, "accessList", &error)) {
        if (error) { return FfxTxStatusBadData; }
        rlpStatus = ffx_rlp_appendArray(&rlp, 0);
        return FfxTxStatusOK;
    }

    if (!ffx_cbor_checkType(accessList, FfxCborArray)) {
        return FfxTxStatusBadData;
    }

    size_t length = ffx_cbor_getLength(&accessList, &error);
    if (error) { return FfxTxStatusBadData; }
    FfxRlpStatus rlpStatus = ffx_rlp_appendArray(rlp, length);
    if (rlpStatus) { return mungeRlpStatus(rlpStatus); }

    FfxCborStatus status = FfxCborStatusBeginIterator;
    while (ffx_cbor_iterate(&accessList, NULL, &status)) {

        if (!ffx_cbor_checkType(&accessList, FfxCborArray) ||
          ffx_cbor_getLength(&accessList, &status) != 2 || status) {
            return FfxTxStatusBadData;
        }

        // Address

        FfxCborCursor address;
        ffx_cbor_clone(&address, &accessList);

        if (!ffx_cbor_followIndex(address, 0, &status) ||
          !ffx_cbor_checkType(address, FfxCborTypeData) ||
          !ffx_cbor_checkLength(&address, 20) || status) {
            return FfxTxStatusBadData;
        }

        FfxCborData data = ffx_cbor_getData(&address, &status);
        if (status) { return FfxTxStatusBadData; }

        rlpStatus = ffx_rlp_appendData(rlp, data.bytes, data.length);
        if (rlpStatus) { return mungeRlpStatus(rlpStatus); }

        // Slots

        rlpStatus = ffx_rlp_appendArray(&rlp, 0);
    }

    return FfxTxStatusOK;
}
*/

#define readDataLength(NAME,CURSOR,LENGTH) \
    FfxCborData (NAME) = { 0 }; \
    { \
        if (!ffx_cbor_checkType((CURSOR), FfxCborTypeData)) { \
            return FfxTxStatusBadData; \
        } \
        FfxCborStatus error = FfxCborStatusOK; \
        (NAME) = ffx_cbor_getData((CURSOR), &error); \
        if (error || (NAME).length != (LENGTH)) { \
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
        if (!ffx_cbor_checkType((cbor), FfxCborTypeArray) || \
          !ffx_cbor_checkLength((cbor), (length))) { \
            return FfxTxStatusBadData; \
        } \
    } while(0);


static FfxTxStatus appendAccessList(FfxRlpBuilder *rlp, FfxCborCursor *tx) {
    // Copy the access list (if any) to the RLP

    FfxCborStatus error = FfxCborStatusOK;

    // If the accessList key is absent, use the default, an empty access list
    FfxCborCursor accessList = { 0 };
    ffx_cbor_clone(&accessList, tx);
    if (!ffx_cbor_followKey(&accessList, "accessList", &error)) {
        if (error) { return FfxTxStatusBadData; }
        ffx_rlp_appendArray(rlp, 0);
        return mungeRlpStatus(rlp->status);
    }

    checkArray(&accessList);

    size_t i = 0;
    FfxRlpBuilderTag iTag = ffx_rlp_appendMutableArray(rlp);
    if (rlp->status) { return mungeRlpStatus(rlp->status); }

    error = FfxCborStatusBeginIterator;
    while (ffx_cbor_iterate(&accessList, NULL, &error)) {

        // Check: [ address, slots ]
        checkArrayLength(&accessList, 2);

        ffx_rlp_appendArray(rlp, 2);
        if (rlp->status) { return mungeRlpStatus(rlp->status); }

        // Check: X = [ data: 20 bytes ]
        FfxCborCursor address = { 0 };
        ffx_cbor_clone(&address, &accessList);
        if (!ffx_cbor_followIndex(&address, 0, &error)) {
            return FfxTxStatusBadData;
        }

        {
            readDataLength(data, &address, 20);
            ffx_rlp_appendData(rlp, data.bytes, data.length);
            if (rlp->status) { return mungeRlpStatus(rlp->status); }
        }

        // Check: Y = [ ]
        FfxCborCursor slots = { 0 };
        ffx_cbor_clone(&slots, &accessList);
        if (!ffx_cbor_followIndex(&slots, 1, &error) ||
          !ffx_cbor_checkType(&slots, FfxCborTypeArray)) {
            return FfxTxStatusBadData;
        }

        size_t si = 0;
        FfxRlpBuilderTag siTag = ffx_rlp_appendMutableArray(rlp);
        if (rlp->status) { return mungeRlpStatus(rlp->status); }

        error = FfxCborStatusBeginIterator;
        while (ffx_cbor_iterate(&slots, NULL, &error)) {
            readDataLength(data, &slots, 32);

            ffx_rlp_appendData(rlp, data.bytes, data.length);
            if (rlp->status) { return mungeRlpStatus(rlp->status); }

            si++;
            ffx_rlp_adjustCount(rlp, siTag, si);
        }

        i++;
        ffx_rlp_adjustCount(rlp, iTag, i);
    }

    if (error) { return mungeCborStatus(error); }

    return mungeRlpStatus(rlp->status);
}

FfxTxStatus ffx_tx_serializeUnsigned(FfxCborCursor *tx, uint8_t *data, size_t *_length) {
    size_t length = *_length;

    if (length < 1) { return FfxTxStatusBufferOverrun; }

    // @TODO: for non-1559: FfxTxStatusUnsupportedVersion,

    // Add the EIP-2718 Envelope Type;
    data[0] = 2;

    // Skip the Envelope Type
    FfxRlpBuilder rlp = { 0 };
    ffx_rlp_build(&rlp, &data[1], length - 1);

    // The Unsigned EIP-1559 Tx has 9 fields
    if (!ffx_rlp_appendArray(&rlp, 9)) { return mungeRlpStatus(rlp.status); }

    FfxTxStatus status = FfxTxStatusOK;

    status = append(&rlp, FormatNumber, tx, "chainId");
    if (status) { return status; }

    status = append(&rlp, FormatNumber, tx, "nonce");
    if (status) { return status; }

    status = append(&rlp, FormatNumber, tx, "maxPriorityFeePerGas");
    if (status) { return status; }

    status = append(&rlp, FormatNumber, tx, "maxFeePerGas");
    if (status) { return status; }

    status = append(&rlp, FormatNumber, tx, "gasLimit");
    if (status) { return status; }

    status = append(&rlp, FormatNullableAddress, tx, "to");
    if (status) { return status; }

    status = append(&rlp, FormatNumber, tx, "value");
    if (status) { return status; }

    status = append(&rlp, FormatData, tx, "data");
    if (status) { return status; }

    //if (!ffx_rlp_appendArray(&rlp, 0)) {
    //    return mungeRlpStatus(rlp.status);
    //}

    status = appendAccessList(&rlp, tx);
    if (status) { return status; }

    *_length = ffx_rlp_finalize(&rlp) + 1;

    return mungeRlpStatus(rlp.status);
}
