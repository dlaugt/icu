/*
*******************************************************************************
* Copyright (C) 2013, International Business Machines
* Corporation and others.  All Rights Reserved.
*******************************************************************************
* collationdatareader.cpp
*
* created on: 2013feb07
* created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/udata.h"
#include "unicode/uscript.h"
#include "cmemory.h"
#include "collation.h"
#include "collationdata.h"
#include "collationdatareader.h"
#include "collationsettings.h"
#include "uassert.h"
#include "utrie2.h"

U_NAMESPACE_BEGIN

// TODO: only for cloning a reader that was used to deserialize API-input binary data,
// where there is no UDataMemory
CollationDataReader *
CollationDataReader::clone() const {
    LocalPointer<CollationDataReader> newReader(new CollationDataReader(*this));
    if(newReader.isNull()) {
        return NULL;
    }
    // The copy constructor does a shallow clone, copying all fields.
    // We cannot clone the UDataMemory.
    U_ASSERT(memory == NULL);
    newReader->memory = NULL;
    if(trie != NULL) {
        UErrorCode errorCode = U_ZERO_ERROR;
        newReader->trie = utrie2_clone(trie, &errorCode);
        if(newReader->trie == NULL || U_FAILURE(errorCode)) {
            return NULL;
        }
        newReader->data.trie = newReader->trie;
    }
    if(unsafeBackwardSet != NULL) {
        newReader->unsafeBackwardSet = new UnicodeSet(*unsafeBackwardSet);
        if(newReader->unsafeBackwardSet == NULL) {
            return NULL;
        }
        newReader->data.unsafeBackwardSet = newReader->unsafeBackwardSet;
    }
    return newReader.orphan();
}

CollationDataReader::~CollationDataReader() {
    udata_close(memory);
    utrie2_close(trie);
    delete unsafeBackwardSet;
}

namespace {

int32_t getIndex(const int32_t *indexes, int32_t length, int32_t i) {
    return (i < length) ? indexes[i] : 0;
}

};  // namespace

void
CollationDataReader::setData(const CollationData *baseData, const uint8_t *inBytes,
                             UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) { return; }
    const int32_t *inIndexes = reinterpret_cast<const int32_t *>(inBytes);
    int32_t indexesLength = inIndexes[IX_INDEXES_LENGTH];
    if(indexesLength < 2) {
        errorCode = U_INVALID_FORMAT_ERROR;  // Not enough indexes.
        return;
    }

    data.base = baseData;
    int32_t options = inIndexes[IX_OPTIONS];
    data.numericPrimary = options & 0xff000000;
    settings.options = options & 0xffffff;

    // Set pointers to non-empty data parts.
    // Do this in order of their byte offsets. (Should help porting to Java.)

    int32_t index;  // one of the indexes[] slots
    int32_t offset;  // byte offset for the index part
    int32_t length;  // number of bytes in the index part

    index = IX_REORDER_CODES_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 4) {
        settings.reorderCodes = reinterpret_cast<const int32_t *>(inBytes + offset);
        settings.reorderCodesLength = length / 4;
    } else {
        settings.reorderCodes = NULL;
        settings.reorderCodesLength = 0;
    }

    // There should be a reorder table only if there are reorder codes.
    // However, when there are reorder codes the reorder table may be omitted to reduce
    // the data size, and then the caller needs to allocate and build the reorder table.
    index = IX_REORDER_TABLE_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 256) {
        settings.reorderTable = inBytes + offset;
    } else {
        settings.reorderTable = NULL;
    }

    index = IX_TRIE_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 8) {
        data.trie = trie = utrie2_openFromSerialized(
            UTRIE2_32_VALUE_BITS, inBytes + offset, length, NULL,
            &errorCode);
        if(U_FAILURE(errorCode)) { return; }
    } else if(baseData != NULL) {
        // Copy all mappings from the baseData.
        // The trie value indexes into the arrays must match those arrays.
        data.trie = baseData->trie;
        data.ce32s = baseData->ce32s;
        data.ces = baseData->ces;
        data.contexts = baseData->contexts;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  // No mappings.
        return;
    }

    index = IX_CES_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 8) {
        if(data.ces != NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  // Tailored ces without tailored trie.
            return;
        }
        data.ces = reinterpret_cast<const int64_t *>(inBytes + offset);
    } else {
        data.ces = NULL;
    }

    int32_t jamoCEsStart = getIndex(inIndexes, indexesLength, IX_JAMO_CES_START);
    if(jamoCEsStart >= 0) {
        if(data.ces == NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  // Index into non-existent CEs[].
            return;
        }
        data.jamoCEs = data.ces + jamoCEsStart;
    } else if(baseData != NULL) {
        data.jamoCEs = baseData->jamoCEs;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  // No Jamo CEs for Hangul processing.
        return;
    }

    index = IX_CE32S_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 4) {
        if(data.ce32s != NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  // Tailored ce32s without tailored trie.
            return;
        }
        data.ce32s = reinterpret_cast<const uint32_t *>(inBytes + offset);
    } else {
        data.ce32s = NULL;
    }

    index = IX_CONTEXTS_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 2) {
        if(data.contexts != NULL) {
            errorCode = U_INVALID_FORMAT_ERROR;  // Tailored contexts without tailored trie.
            return;
        }
        data.contexts = reinterpret_cast<const UChar *>(inBytes + offset);
    } else {
        data.contexts = NULL;
    }

    index = IX_UNSAFE_BWD_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 2) {
        if(baseData == NULL) {
            // Create the unsafe-backward set for the root collator.
            // Include all non-zero combining marks and trail surrogates.
            // We do this at load time, rather than at build time,
            // to simplify Unicode version bootstrapping:
            // The root data builder only needs the new FractionalUCA.txt data,
            // but it need not be built with a version of ICU already updated to
            // the corresponding new Unicode Character Database.
            // TODO: Optimize, and reduce dependencies,
            // by enumerating the Normalizer2Impl data more directly.
            unsafeBackwardSet = new UnicodeSet(
                UNICODE_STRING_SIMPLE("[[:^lccc=0:][\\udc00-\\udfff]]"), errorCode);
            if(U_FAILURE(errorCode)) { return; }
        } else {
            // Clone the root collator's set.
            unsafeBackwardSet = new UnicodeSet(*baseData->unsafeBackwardSet);
        }
        if(unsafeBackwardSet == NULL) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        // Add the ranges from the data file to the unsafe-backward set.
        USerializedSet sset;
        const uint16_t *unsafeData = reinterpret_cast<const uint16_t *>(inBytes + offset);
        if(!uset_getSerializedSet(&sset, unsafeData, length / 2)) {
            errorCode = U_INVALID_FORMAT_ERROR;
            return;
        }
        int32_t count = uset_getSerializedRangeCount(&sset);
        for(int32_t i = 0; i < count; ++i) {
            UChar32 start, end;
            uset_getSerializedRange(&sset, i, &start, &end);
            unsafeBackwardSet->add(start, end);
        }
        // Mark each lead surrogate as "unsafe"
        // if any of its 1024 associated supplementary code points is "unsafe".
        UChar32 c = 0x10000;
        for(UChar lead = 0xd800; lead < 0xdc00; ++lead, c += 0x400) {
            if(!unsafeBackwardSet->containsNone(c, c + 0x3ff)) {
                unsafeBackwardSet->add(lead);
            }
        }
        unsafeBackwardSet->freeze();
        data.unsafeBackwardSet = unsafeBackwardSet;
    } else if(baseData != NULL) {
        // No tailoring-specific data: Alias the root collator's set.
        data.unsafeBackwardSet = baseData->unsafeBackwardSet;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  // No unsafeBackwardSet.
        return;
    }

    index = IX_SCRIPTS_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 2) {
        data.scripts = reinterpret_cast<const uint16_t *>(inBytes + offset);
        data.scriptsLength = length / 2;
    } else if(baseData != NULL) {
        data.scripts = baseData->scripts;
        data.scriptsLength = baseData->scriptsLength;
    } else {
        data.scripts = NULL;
        data.scriptsLength = 0;
    }

    index = IX_COMPRESSIBLE_BYTES_OFFSET;
    offset = getIndex(inIndexes, indexesLength, index);
    length = getIndex(inIndexes, indexesLength, index + 1) - offset;
    if(length >= 256) {
        data.compressibleBytes = reinterpret_cast<const UBool *>(inBytes + offset);
    } else if(baseData != NULL) {
        data.compressibleBytes = baseData->compressibleBytes;
    } else {
        errorCode = U_INVALID_FORMAT_ERROR;  // No compressibleBytes[].
        return;
    }

    // Set variableTop from options & ALTERNATE_MASK + MAX_VARIABLE_MASK and scripts data.
    int32_t maxVariable =
        (options & CollationSettings::MAX_VARIABLE_MASK) >> CollationSettings::MAX_VARIABLE_SHIFT;
    settings.variableTop =
        data.getVariableTopForMaxVariable((CollationSettings::MaxVariable)maxVariable);
    if(settings.variableTop == 0) {
        errorCode = U_INVALID_FORMAT_ERROR;
        return;
    }
}

UBool U_CALLCONV
CollationDataReader::isAcceptable(void *context,
                                  const char * /* type */, const char * /*name*/,
                                  const UDataInfo *pInfo) {
    if(
        pInfo->size >= 20 &&
        pInfo->isBigEndian == U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily == U_CHARSET_FAMILY &&
        pInfo->dataFormat[0] == 0x55 &&  // dataFormat="UCol"
        pInfo->dataFormat[1] == 0x43 &&
        pInfo->dataFormat[2] == 0x6f &&
        pInfo->dataFormat[3] == 0x6c &&
        pInfo->formatVersion[0] == 4
    ) {
        UVersionInfo *version = reinterpret_cast<UVersionInfo *>(context);
        if(version != NULL) {
            uprv_memcpy(version, pInfo->dataVersion, 4);
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

U_NAMESPACE_END

#endif  // !UCONFIG_NO_COLLATION
