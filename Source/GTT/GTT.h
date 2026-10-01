#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGTT, Log, All);

GTT_API void GTTWriteRuntimeEvidence(const FString& Message);

#define GTT_LOG(Verbosity, Format, ...) \
    do \
    { \
        const FString GTTFormattedRuntimeEvidence = FString::Printf(Format, ##__VA_ARGS__); \
        UE_LOG(LogGTT, Verbosity, TEXT("%s"), *GTTFormattedRuntimeEvidence); \
        GTTWriteRuntimeEvidence(GTTFormattedRuntimeEvidence); \
    } while (false)
