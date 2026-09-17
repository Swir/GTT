#include "NPC/GTTLogisticsDispatcherPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTDispatcherRelationshipSubsystem.h"
#include "World/GTTLogisticsReputationSubsystem.h"

namespace
{
    const FName FeedDepotDispatcherRole(TEXT("FeedDepotDispatcher"));
    const FName HillFarmReceiverRole(TEXT("HillFarmReceiver"));
    const FName WoodYardForemanRole(TEXT("WoodYardForeman"));
}

AGTTLogisticsDispatcherPawn::AGTTLogisticsDispatcherPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.12f;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 240.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
    BodyMesh->SetRelativeScale3D(FVector(0.30f, 0.30f, 0.86f));
    if (CylinderFinder.Succeeded()) BodyMesh->SetStaticMesh(CylinderFinder.Object);

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(RootComponent);
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
    HeadMesh->SetRelativeScale3D(FVector(0.22f));
    if (SphereFinder.Succeeded()) HeadMesh->SetStaticMesh(SphereFinder.Object);

    RoleLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RoleLabel"));
    RoleLabel->SetupAttachment(RootComponent);
    RoleLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));
    RoleLabel->SetHorizontalAlignment(EHTA_Center);
    RoleLabel->SetWorldSize(18.0f);
    RoleLabel->SetTextRenderColor(FColor(120, 220, 255));
    RoleLabel->SetCastShadow(true);
}

void AGTTLogisticsDispatcherPawn::BeginPlay()
{
    Super::BeginPlay();
    if (WorkLocation.IsNearlyZero()) WorkLocation = GetActorLocation();
    if (HomeLocation.IsNearlyZero()) HomeLocation = WorkLocation + FVector(-900.0f, -2200.0f, 0.0f);
    DayNightCycle = Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()));
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    if (RoleLabel) RoleLabel->SetText(FText::FromString(DisplayName.ToUpper()));
}

void AGTTLogisticsDispatcherPawn::ConfigureDispatcher(
    FName InRoleTag,
    const FString& InDisplayName,
    const FVector& InWorkLocation,
    const FVector& InHomeLocation)
{
    RoleTag = InRoleTag;
    DisplayName = InDisplayName;
    WorkLocation = InWorkLocation;
    HomeLocation = InHomeLocation;
    if (RoleLabel) RoleLabel->SetText(FText::FromString(DisplayName.ToUpper()));
    SetActorLocation(ResolveScheduleTarget());
}

bool AGTTLogisticsDispatcherPawn::IsOnShift() const
{
    const AGTTDayNightCycle* Cycle = DayNightCycle.Get();
    const float Hour = Cycle ? Cycle->GetTimeOfDayHours() : 12.0f;
    return Hour >= WorkStartHour && Hour < WorkEndHour;
}

FVector AGTTLogisticsDispatcherPawn::ResolveScheduleTarget() const
{
    return IsOnShift() ? WorkLocation : HomeLocation;
}

FString AGTTLogisticsDispatcherPawn::BuildOnShiftStatusLine() const
{
    if (!GetWorld()) return TEXT("LOGISTICS OFFLINE");
    const UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    const UGTTDispatcherRelationshipSubsystem* Relationships = GetWorld()->GetSubsystem<UGTTDispatcherRelationshipSubsystem>();
    if (!Logistics || !Relationships) return TEXT("LOGISTICS OFFLINE");

    if (RoleTag == FeedDepotDispatcherRole)
    {
        return FString::Printf(TEXT("E NEGOTIATE | DESK T%d | Q%d/%d | %s %d"),
            Logistics->GetActiveCargoOrderTier(), Logistics->GetCargoReservationCount(),
            Relationships->GetCargoReservationCapacity(), *Relationships->GetFeedRelationshipLabel(), Relationships->GetFeedDispatcherRelationship());
    }
    if (RoleTag == HillFarmReceiverRole)
    {
        return FString::Printf(TEXT("HILL NEED %d | E STATUS | %s"), Logistics->GetHillFarmDemand(), *Relationships->GetHillRelationshipLabel());
    }
    if (RoleTag == WoodYardForemanRole)
    {
        return FString::Printf(TEXT("WOOD NEED %d | E STATUS | %s"), Logistics->GetWoodYardDemand(), *Relationships->GetWoodRelationshipLabel());
    }
    return TEXT("E TALK");
}

void AGTTLogisticsDispatcherPawn::Interact_Implementation(AActor* Interactor)
{
    APawn* PlayerPawn = Cast<APawn>(Interactor);
    if (!PlayerPawn || !GetWorld()) return;

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    const UGTTDispatcherRelationshipSubsystem* Relationships = GetWorld()->GetSubsystem<UGTTDispatcherRelationshipSubsystem>();
    if (!Economy || !Logistics || !Relationships) return;

    if (!IsOnShift())
    {
        Economy->PushMessage(FString::Printf(TEXT("%s is off shift. Rural logistics staff return at 07:00."), *DisplayName), 4.5f);
        return;
    }

    if (RoleTag == FeedDepotDispatcherRole)
    {
        FString Summary;
        bool bChanged = false;
        const int32 AccessTier = Relationships->GetCargoDeskAccessTier();

        // Cycle the authoritative 0.1.7 negotiation, but skip contracts this specific dispatcher
        // relationship has not unlocked yet. T1 is always the safe fallback when it is fulfillable.
        for (int32 Attempt = 0; Attempt < 3; ++Attempt)
        {
            bChanged = Logistics->CycleCargoNegotiatedOrder(Summary);
            const int32 NegotiatedTier = Logistics->GetNegotiatedCargoOrderTier();
            // Keep the existing active-tier guard, but when a queued reservation owns the active
            // slot we must also validate the freshly negotiated tier before protecting stock.
            if (!bChanged || (Logistics->GetActiveCargoOrderTier() <= AccessTier && NegotiatedTier <= AccessTier)) break;
        }
        if (bChanged && Logistics->GetNegotiatedCargoOrderTier() > AccessTier)
        {
            Logistics->ClearCargoNegotiatedOrder();
            bChanged = false;
            Summary = FString::Printf(TEXT("Feed Dispatcher trust only covers T%d work right now. Complete clean deliveries to unlock heavier manual reservations."), AccessTier);
        }

        FString ReservationSummary;
        if (bChanged)
        {
            const bool bReserved = Logistics->ReserveNegotiatedCargoOrder(
                Relationships->GetCargoReservationHoldMinutes(),
                Relationships->GetCargoReservationCapacity(),
                ReservationSummary);
            if (!bReserved)
            {
                Summary = FString::Printf(TEXT("%s | HOLD NOT WRITTEN: %s"), *Summary, *ReservationSummary);
            }
            else
            {
                Summary = FString::Printf(TEXT("%s | %s"), *Summary, *ReservationSummary);
            }
        }

        const FString RelationshipLine = FString::Printf(TEXT("%s %d | DESK ACCESS T%d | %s | %s"),
            *Relationships->GetFeedRelationshipLabel(), Relationships->GetFeedDispatcherRelationship(), AccessTier,
            *Relationships->GetCargoReservationFavorLabel(), *Relationships->GetDispatcherReaction(FeedDepotDispatcherRole));
        Economy->PushMessage(bChanged
            ? FString::Printf(TEXT("FEED DISPATCH NEGOTIATION | CONTRACT DESK: %s | %s | %s"), *Summary, *RelationshipLine, *Relationships->GetContractDeskSummary())
            : FString::Printf(TEXT("%s | %s | %s"), *Summary, *RelationshipLine, *Logistics->GetCargoReservationSummary()),
            bChanged ? 9.0f : 6.5f);
        if (bChanged)
        {
            // Even a failed second hold can clear/advance the negotiated selection. Persist the
            // authoritative queue/market state so a reload cannot restore stock that was held.
            if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
        }
        return;
    }

    if (RoleTag == HillFarmReceiverRole)
    {
        Economy->PushMessage(FString::Printf(TEXT("HILL RECEIVER [%s %d]: need %d units. Current %s | %s | %s. %s Wanted drivers get no legal handoff."),
            *Relationships->GetHillRelationshipLabel(), Relationships->GetHillReceiverRelationship(),
            Logistics->GetHillFarmDemand(), *Logistics->GetCargoNegotiationStatusLabel(), *Logistics->GetCargoCommodityLabel(),
            *Logistics->GetCargoReservationSummary(), *Relationships->GetDispatcherReaction(HillFarmReceiverRole)), 7.5f);
        return;
    }

    if (RoleTag == WoodYardForemanRole)
    {
        Economy->PushMessage(FString::Printf(TEXT("WOOD FOREMAN [%s %d]: need %d units, backlog pressure %d. Current %s | %s | %s. %s"),
            *Relationships->GetWoodRelationshipLabel(), Relationships->GetWoodForemanRelationship(),
            Logistics->GetWoodYardDemand(), Logistics->GetCargoBacklogPressure(),
            *Logistics->GetCargoNegotiationStatusLabel(), *Logistics->GetRoadSupplySignalLabel(),
            *Logistics->GetCargoReservationSummary(), *Relationships->GetDispatcherReaction(WoodYardForemanRole)), 7.5f);
        return;
    }

    Economy->PushMessage(FString::Printf(TEXT("%s: %s"), *DisplayName, *Logistics->GetCargoStockSummary()), 5.0f);
}

FText AGTTLogisticsDispatcherPawn::GetInteractionText_Implementation() const
{
    if (!GetWorld()) return FText::GetEmpty();
    const UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    const UGTTDispatcherRelationshipSubsystem* Relationships = GetWorld()->GetSubsystem<UGTTDispatcherRelationshipSubsystem>();
    if (!Logistics || !Relationships) return FText::FromString(TEXT("Logistics unavailable"));

    if (!IsOnShift())
    {
        return FText::FromString(FString::Printf(TEXT("%s off shift | 07:00-17:30"), *DisplayName));
    }
    if (RoleTag == FeedDepotDispatcherRole)
    {
        return FText::FromString(FString::Printf(TEXT("Negotiate CARGO order | %s | access T%d | queue %d/%d | %s"),
            *Relationships->GetFeedRelationshipLabel(), Relationships->GetCargoDeskAccessTier(),
            Logistics->GetCargoReservationCount(), Relationships->GetCargoReservationCapacity(), *Logistics->GetCargoNegotiationStatusLabel()));
    }
    if (RoleTag == HillFarmReceiverRole)
    {
        return FText::FromString(FString::Printf(TEXT("Ask Hill Receiver | %s %d | need %d | order T%d"),
            *Relationships->GetHillRelationshipLabel(), Relationships->GetHillReceiverRelationship(),
            Logistics->GetHillFarmDemand(), Logistics->GetActiveCargoOrderTier()));
    }
    if (RoleTag == WoodYardForemanRole)
    {
        return FText::FromString(FString::Printf(TEXT("Ask Wood Foreman | %s %d | need %d | backlog %d"),
            *Relationships->GetWoodRelationshipLabel(), Relationships->GetWoodForemanRelationship(),
            Logistics->GetWoodYardDemand(), Logistics->GetCargoBacklogPressure()));
    }
    return FText::FromString(FString::Printf(TEXT("Talk to %s"), *DisplayName));
}

void AGTTLogisticsDispatcherPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!DayNightCycle.IsValid())
    {
        DayNightCycle = Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this, AGTTDayNightCycle::StaticClass()));
    }

    const FVector Target = ResolveScheduleTarget();
    FVector Delta = Target - GetActorLocation();
    Delta.Z = 0.0f;
    if (Delta.SizeSquared2D() > FMath::Square(85.0f))
    {
        AddMovementInput(Delta.GetSafeNormal2D(), 1.0f);
    }

    if (RoleLabel)
    {
        const FString Shift = IsOnShift() ? BuildOnShiftStatusLine() : TEXT("OFF SHIFT | 07:00");
        RoleLabel->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), *DisplayName.ToUpper(), *Shift)));
    }
}
