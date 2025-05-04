#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "firefly-tx.h"

#include "firefly-cbor.h"
#include "firefly-rlp.h"

// DEBUG: Move to utils
void dumpBuffer(const char*, const uint8_t*, size_t);

typedef enum Format {
    FormatData = 0,
    FormatNumber,
    FormatAddress,
    FormatNullableAddress,
} Format;


static FfxDataError append(FfxRlpBuilder *rlp, Format format, FfxCborCursor tx,
  const char* key) {

    FfxCborCursor value = ffx_cbor_followKey(tx, key);
    if (value.error == FfxDataErrorNotFound) {
        ffx_rlp_appendData(rlp, NULL, 0);
        return rlp->error;
    } else if (value.error) {
        return value.error;
    }

    if (!ffx_cbor_checkType(value, FfxCborTypeData)) {
        return FfxDataErrorBadData;
    }

    FfxDataResult data = ffx_cbor_getData(value);
    if (data.error) { return FfxDataErrorBadData; }

    const uint8_t *bytes = data.bytes;
    size_t length = data.length;

    // Consume any leading 0 bytes
    if (format == FormatNumber) {
        while (length) {
            if (bytes[0]) { break; }
            bytes++;
            length--;
        }
        if (length > 32) { return FfxDataErrorOverflow; }

    } else if (format == FormatAddress) {
        if (length != 20) { return FfxDataErrorBadData; }

    } else if (format == FormatNullableAddress) {
        if (length != 0 && length != 20) { return FfxDataErrorBadData; }
    }

    ffx_rlp_appendData(rlp, bytes, length);
    return rlp->error;
}


static FfxDataError appendAccessList(FfxRlpBuilder *rlp, FfxCborCursor tx) {
    // Copy the access list (if any) to the RLP

    // If the accessList key is absent, use the default, an empty access list
    FfxCborCursor accessList = ffx_cbor_followKey(tx, "accessList");
    if (accessList.error == FfxDataErrorNotFound) {
        ffx_rlp_appendArray(rlp, 0);
        return rlp->error;
    } else if (accessList.error) {
        return accessList.error;
    }

    if (!ffx_cbor_checkType(accessList, FfxCborTypeArray)) {
        return FfxDataErrorBadData;
    }

    size_t i = 0;
    FfxRlpBuilderTag iTag = ffx_rlp_appendMutableArray(rlp);
    if (rlp->error) { return rlp->error; }

    FfxCborIterator iter = ffx_cbor_iterate(accessList);
    while (ffx_cbor_nextChild(&iter)) {

        // Check: [ address, slots ]
        if (!ffx_cbor_checkLength(iter.child, FfxCborTypeArray, 2)) {
            return FfxDataErrorBadData;
        }

        ffx_rlp_appendArray(rlp, 2);
        if (rlp->error) { return rlp->error; }

        // Check: X = [ data: 20 bytes ]
        FfxCborCursor address = ffx_cbor_followIndex(iter.child, 0);
        if (address.error) { return address.error; }

        {
            FfxDataResult data = ffx_cbor_getData(address);
            if (data.error) { return data.error; }
            if (data.length != 20) { return FfxDataErrorBadData; }

            ffx_rlp_appendData(rlp, data.bytes, data.length);
            if (rlp->error) { return rlp->error; }
        }

        // Check: Y = [ ]
        FfxCborCursor slots = ffx_cbor_followIndex(iter.child, 1);
        if (slots.error) { return slots.error; }
        if (!ffx_cbor_checkType(slots, FfxCborTypeArray)) {
            return FfxDataErrorBadData;
        }

        size_t si = 0;
        FfxRlpBuilderTag siTag = ffx_rlp_appendMutableArray(rlp);
        if (rlp->error) { return rlp->error; }

        FfxCborIterator iterSlots = ffx_cbor_iterate(slots);
        while (ffx_cbor_nextChild(&iterSlots)) {
            FfxDataResult data = ffx_cbor_getData(iterSlots.child);
            if (data.error) { return data.error; };
            if (data.length != 32) { return FfxDataErrorBadData; }

            ffx_rlp_appendData(rlp, data.bytes, data.length);
            if (rlp->error) { return rlp->error; }

            si++;
            ffx_rlp_adjustCount(rlp, siTag, si);
        }

        i++;
        ffx_rlp_adjustCount(rlp, iTag, i);
    }

    if (iter.child.error) { return iter.child.error; }

    return rlp->error;
}



FfxDataError serialize1559(FfxCborCursor tx, FfxRlpBuilder *rlp) {

    // The Unsigned EIP-1559 Tx has 9 fields
    if (!ffx_rlp_appendArray(rlp, 9)) { return rlp->error; }

    FfxDataError error = FfxDataErrorNone;

    error = append(rlp, FormatNumber, tx, "chainId");
    if (error) { return error; }

    error = append(rlp, FormatNumber, tx, "nonce");
    if (error) { return error; }
    // @TODO: Copy nonce

    error = append(rlp, FormatNumber, tx, "maxPriorityFeePerGas");
    if (error) { return error; }

    error = append(rlp, FormatNumber, tx, "maxFeePerGas");
    if (error) { return error; }

    error = append(rlp, FormatNumber, tx, "gasLimit");
    if (error) { return error; }

    error = append(rlp, FormatNullableAddress, tx, "to");
    if (error) { return error; }

    error = append(rlp, FormatNumber, tx, "value");
    if (error) { return error; }

    error = append(rlp, FormatData, tx, "data");
    if (error) { return error; }

    //if (!ffx_rlp_appendArray(&rlp, 0)) {
    //    return mungeDataError(rlp.error);
    //}

    error = appendAccessList(rlp, tx);
    if (error) { return error; }

    return FfxDataErrorNone;
}

static FfxValueResult readNumber(FfxCborCursor tx, const char* key) {

    FfxCborCursor follow = ffx_cbor_followKey(tx, key);
    if (follow.error) {
        return (FfxValueResult){ .error = follow.error };
    } else if (!ffx_cbor_checkType(follow, FfxCborTypeData)) {
        return (FfxValueResult){ .error = FfxDataErrorBadData };
    }

    FfxDataResult data = ffx_cbor_getData(follow);
    if (data.error) { return (FfxValueResult){ .error = data.error }; }

    if (data.length > 8) {
        return (FfxValueResult){ .error = FfxDataErrorOverflow };
    }

    uint64_t value = 0;
    for (int i = 0; i < data.length; i++) {
        value <<= 8;
        value |= data.bytes[i];
    }

    return (FfxValueResult){ .value = value };
}

FfxTx ffx_tx_serializeUnsigned(FfxCborCursor tx, uint8_t *data, size_t length) {
    FfxTx result = { 0 };

    {
        FfxValueResult type = readNumber(tx, "type");
        if (type.value > 0x7f) {
            return (FfxTx){ .error = FfxDataErrorUnsupportedFeature };
        }
        if (type.error) { return (FfxTx){ .error = type.error }; }

        result.type = type.value;
    }

    switch (result.type) {
        case 2:
            break;
        default:
            return (FfxTx){ .error = FfxDataErrorUnsupportedFeature };
    }

    // Add the EIP-2718 Envelope Type;
    if (length < 1) {
        return (FfxTx){ .error = FfxDataErrorBufferOverrun };
    }
    data[0] = result.type;

    // Skip the Envelope Type during RLP output
    FfxRlpBuilder rlp = ffx_rlp_build(&data[1], length - 1);

    result.rlp.bytes = data;

    FfxDataError error = serialize1559(tx, &rlp);
    result.rlp.length = ffx_rlp_finalize(&rlp) + 1;
    if (error) { result.error = error; }

    return result;
}

static FfxRlpCursor getRlp(FfxTx tx) {
    if (tx.type == 2) {
        if (tx.rlp.length == 0) {
            return (FfxRlpCursor){ .error = FfxDataErrorBadData };
        }
        return ffx_rlp_walk(&tx.rlp.bytes[1], tx.rlp.length - 1);
    }
    return (FfxRlpCursor){ .error = FfxDataErrorUnsupportedFeature };
}

static FfxDataResult readFormat(FfxTx tx, Format format, size_t index) {
    if (tx.error) { return (FfxDataResult){ .error = tx.error }; }

    FfxRlpCursor rlp = getRlp(tx);
    if (rlp.error) { return (FfxDataResult){ .error = rlp.error }; }

    if (tx.type == 2) {
        // Make sure it is a valid EIP-1559 tx
        FfxSizeResult count = ffx_rlp_getArrayCount(rlp);
        if (count.error) { return (FfxDataResult){ .error = count.error }; }
        if (count.value != 9 && count.value != 12) {
            return (FfxDataResult){ .error = FfxDataErrorBadData };
        }
    } else {
        return (FfxDataResult){ .error = FfxDataErrorUnsupportedFeature };
    }

    FfxRlpCursor follow = ffx_rlp_followIndex(rlp, index);
    if (follow.error) { return (FfxDataResult){ .error = follow.error }; }

    FfxDataResult result = ffx_rlp_getData(follow);
    if (result.error) { return result; }

    // It must be an init tx (no address) or have a valid address
    switch(format) {
        case FormatAddress:
            if (result.length == 20) { return result; }
            break;
        case FormatNullableAddress:
            if (result.length == 0 || result.length == 20) { return result; }
            break;
        case FormatNumber:
            if (result.length <= 32) { return result; }
            break;
        case FormatData:
            return result;
    }

    return (FfxDataResult){ .error = FfxDataErrorBadData };
}

FfxDataResult ffx_tx_getAddress(FfxTx tx) {
    // @TODO: in the future, when adding support for other types, this
    //        result should be checked for nullable vs non-nullable
    return readFormat(tx, FormatNullableAddress, 5);
}

FfxDataResult ffx_tx_getChainId(FfxTx tx) {
    return readFormat(tx, FormatNumber, 0);
}

FfxDataResult ffx_tx_getData(FfxTx tx) {
    return readFormat(tx, FormatData, 7);
}

FfxDataResult ffx_tx_getValue(FfxTx tx) {
    return readFormat(tx, FormatNumber, 6);
}


bool ffx_tx_isSigned(FfxTx tx) {
    if (tx.error) { return false; }

    FfxRlpCursor rlp = getRlp(tx);
    if (rlp.error) { return false; }

    switch (tx.type) {
        case 2: {
            FfxSizeResult count = ffx_rlp_getArrayCount(rlp);
            if (count.error) { return false; }
            return (count.value == 12);
        }
        default:
            break;
    }

    return false;
}

void ffx_tx_dump(FfxTx tx) {
    printf("Transaction: type=%d error=%d\n", tx.type, tx.error);

    if (tx.error) {
        printf("\n");
        return;
    }

    printf("RLP Data: 0x");
    for (int i = 0; i < tx.rlp.length; i++) { printf("%02x", tx.rlp.bytes[i]); }
    printf(" (length=%d)\n", (int)tx.rlp.length);

    printf("RLP Structured: ");
    ffx_rlp_dump(ffx_rlp_walk(&tx.rlp.bytes[1], tx.rlp.length - 1));
}
