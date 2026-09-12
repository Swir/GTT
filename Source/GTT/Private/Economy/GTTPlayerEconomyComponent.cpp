#include "Economy/GTTPlayerEconomyComponent.h"

#include "GameFramework/Pawn.h"
#include "World/GTTRuralEconomySubsystem.h"

UGTTPlayerEconomyComponent::UGTTPlayerEconomyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UGTTPlayerEconomyComponent::BeginPlay()
{
    Super::BeginPlay();
    Cash = FMath::Max(0, StartingCash);
    BroadcastEconomy();
}

void UGTTPlayerEconomyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (ActivityMessageTimeRemaining > 0.0f)
    {
        ActivityMessageTimeRemaining -= DeltaTime;
        if (ActivityMessageTimeRemaining <= 0.0f)
        {
            ActivityMessage.Empty();
            ActivityMessageTimeRemaining = 0.0f;
        }
    }
}

void UGTTPlayerEconomyComponent::AddCash(int32 Amount, const FString& Reason)
{
    if (Amount <= 0) return;
    Cash += Amount;
    PushMessage(Reason.IsEmpty() ? FString::Printf(TEXT("Received $%d"), Amount) : Reason);
    BroadcastEconomy();
}

bool UGTTPlayerEconomyComponent::SpendCash(int32 Amount, const FString& Reason)
{
    if (Amount <= 0) return true;
    if (Cash < Amount)
    {
        PushMessage(FString::Printf(TEXT("Not enough cash. Need $%d, have $%d."), Amount, Cash));
        return false;
    }
    Cash -= Amount;
    PushMessage(Reason.IsEmpty() ? FString::Printf(TEXT("Spent $%d"), Amount) : Reason);
    BroadcastEconomy();
    return true;
}

int32 UGTTPlayerEconomyComponent::ChargeFine(int32 Amount, const FString& Reason)
{
    if (Amount <= 0) return 0;

    // Arrest fines are the authoritative point where the police loop has already forced the player out of the vehicle,
    // but wanted heat has not yet been cleared. This lets the rural-law subsystem seize that nearby owned vehicle.
    if (Reason.StartsWith(TEXT("ARRESTED")))
    {
        UWorld* World = GetWorld();
        APawn* OwnerPawn = Cast<APawn>(GetOwner());
        if (World && OwnerPawn)
        {
            if (UGTTRuralEconomySubsystem* RuralEconomy = World->GetSubsystem<UGTTRuralEconomySubsystem>())
            {
                RuralEconomy->HandleArrestImpound(OwnerPawn);
            }
        }
    }

    const int32 Charged = FMath::Min(Cash, Amount);
    Cash -= Charged;
    PushMessage(Reason.IsEmpty()
        ? FString::Printf(TEXT("Fine paid: $%d"), Charged)
        : FString::Printf(TEXT("%s  Paid: $%d"), *Reason, Charged),
        5.0f);
    BroadcastEconomy();
    return Charged;
}

void UGTTPlayerEconomyComponent::AddFish(float WeightKg, const FString& Species)
{
    if (WeightKg <= 0.0f) return;
    ++FishCount;
    FishWeightKg += WeightKg;
    PushMessage(FString::Printf(TEXT("Caught %s - %.2f kg"), *Species, WeightKg));
    BroadcastEconomy();
}

int32 UGTTPlayerEconomyComponent::SellAllFish(float PricePerKg)
{
    if (FishCount <= 0 || FishWeightKg <= 0.0f || PricePerKg <= 0.0f)
    {
        PushMessage(TEXT("No fish to sell."));
        return 0;
    }

    const int32 SaleValue = FMath::Max(1, FMath::RoundToInt(FishWeightKg * PricePerKg));
    const int32 SoldCount = FishCount;
    const float SoldWeight = FishWeightKg;
    FishCount = 0;
    FishWeightKg = 0.0f;
    Cash += SaleValue;
    PushMessage(FString::Printf(TEXT("Sold %d fish (%.2f kg) for $%d"), SoldCount, SoldWeight, SaleValue));
    BroadcastEconomy();
    return SaleValue;
}

float UGTTPlayerEconomyComponent::ConfiscateAllFish()
{
    const float ConfiscatedWeight = FishWeightKg;
    FishCount = 0;
    FishWeightKg = 0.0f;
    BroadcastEconomy();
    return ConfiscatedWeight;
}

void UGTTPlayerEconomyComponent::RestoreState(int32 InCash, int32 InFishCount, float InFishWeightKg)
{
    Cash = FMath::Max(0, InCash);
    FishCount = FMath::Max(0, InFishCount);
    FishWeightKg = FMath::Max(0.0f, InFishWeightKg);
    BroadcastEconomy();
}

void UGTTPlayerEconomyComponent::PushMessage(const FString& Message, float Duration)
{
    ActivityMessage = Message;
    ActivityMessageTimeRemaining = FMath::Max(0.25f, Duration);
}

void UGTTPlayerEconomyComponent::BroadcastEconomy()
{
    OnEconomyChanged.Broadcast(Cash, FishCount, FishWeightKg);
}
