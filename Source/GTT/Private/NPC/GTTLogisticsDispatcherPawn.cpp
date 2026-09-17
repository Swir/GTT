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
    if (!Logistics) return TEXT("LOGISTICS OFFLINE");

    if (RoleTag == FeedDepotDispatcherRole)
    {
        return FString::Printf(TEXT("E NEGOTIATE | T%d | %s"),
            Logistics->GetActiveCargoOrderTier(), *Logistics->GetCargoNegotiationOptionsLabel());
    }
    if (RoleTag == HillFarmReceiverRole)
    {
        return FString::Printf(TEXT("HILL NEED %d | E STATUS"), Logistics->GetHillFarmDemand());
    }
    if (RoleTag == WoodYardForemanRole)
    {
        return FString::Printf(TEXT("WOOD NEED %d | E STATUS"), Logistics->GetWoodYardDemand());
    }
    return TEXT("E TALK");
}

void AGTTLogisticsDispatcherPawn::Interact_Implementation(AActor* Interactor)
{
    APawn* PlayerPawn = Cast<APawn>(Interactor);
    if (!PlayerPawn || !GetWorld()) return;

    UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn);
    UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Economy || !Logistics) return;

    if (!IsOnShift())
    {
        Economy->PushMessage(FString::Printf(TEXT("%s is off shift. Rural logistics staff return at 07:00."), *DisplayName), 4.5f);
        return;
    }

    if (RoleTag == FeedDepotDispatcherRole)
    {
        FString Summary;
        const bool bChanged = Logistics->CycleCargoNegotiatedOrder(Summary);
        Economy->PushMessage(bChanged
            ? FString::Printf(TEXT("FEED DISPATCH NEGOTIATION: %s"), *Summary)
            : Summary,
            bChanged ? 7.0f : 5.0f);
        if (bChanged)
        {
            if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
        }
        return;
    }

    if (RoleTag == HillFarmReceiverRole)
    {
        Economy->PushMessage(FString::Printf(TEXT("HILL RECEIVER: need %d units. Current %s | %s. Wanted drivers get no legal handoff."),
            Logistics->GetHillFarmDemand(), *Logistics->GetCargoNegotiationStatusLabel(), *Logistics->GetCargoCommodityLabel()), 6.0f);
        return;
    }

    if (RoleTag == WoodYardForemanRole)
    {
        Economy->PushMessage(FString::Printf(TEXT("WOOD FOREMAN: need %d units, backlog pressure %d. Current %s | %s."),
            Logistics->GetWoodYardDemand(), Logistics->GetCargoBacklogPressure(),
            *Logistics->GetCargoNegotiationStatusLabel(), *Logistics->GetRoadSupplySignalLabel()), 6.0f);
        return;
    }

    Economy->PushMessage(FString::Printf(TEXT("%s: %s"), *DisplayName, *Logistics->GetCargoStockSummary()), 5.0f);
}

FText AGTTLogisticsDispatcherPawn::GetInteractionText_Implementation() const
{
    if (!GetWorld()) return FText::GetEmpty();
    const UGTTLogisticsReputationSubsystem* Logistics = GetWorld()->GetSubsystem<UGTTLogisticsReputationSubsystem>();
    if (!Logistics) return FText::FromString(TEXT("Logistics unavailable"));

    if (!IsOnShift())
    {
        return FText::FromString(FString::Printf(TEXT("%s off shift | 07:00-17:30"), *DisplayName));
    }
    if (RoleTag == FeedDepotDispatcherRole)
    {
        return FText::FromString(FString::Printf(TEXT("Negotiate CARGO order | %s"), *Logistics->GetCargoNegotiationStatusLabel()));
    }
    if (RoleTag == HillFarmReceiverRole)
    {
        return FText::FromString(FString::Printf(TEXT("Ask Hill Receiver | need %d | order T%d"),
            Logistics->GetHillFarmDemand(), Logistics->GetActiveCargoOrderTier()));
    }
    if (RoleTag == WoodYardForemanRole)
    {
        return FText::FromString(FString::Printf(TEXT("Ask Wood Foreman | need %d | backlog %d"),
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
