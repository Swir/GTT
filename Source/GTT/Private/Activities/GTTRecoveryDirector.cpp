#include "Activities/GTTRecoveryDirector.h"

#include "Activities/GTTRecoveryTargetVehicle.h"
#include "Components/PrimitiveComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTRecoveryDirector::AGTTRecoveryDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    TowConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("TowConstraint"));
    RootComponent = TowConstraint;
    TowConstraint->SetDisableCollision(true);
    TowConstraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, 650.0f);
    TowConstraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, 650.0f);
    TowConstraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, 280.0f);
    TowConstraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 0.0f);
    TowConstraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 0.0f);
    TowConstraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 0.0f);
}

void AGTTRecoveryDirector::BeginPlay()
{
    Super::BeginPlay();
    SpawnRecoveryTarget();
}

void AGTTRecoveryDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Stage == EGTTRecoveryStage::Idle || Stage == EGTTRecoveryStage::Completed) return;

    ContractTimeRemaining = FMath::Max(0.0f, ContractTimeRemaining - DeltaSeconds);
    if (ContractTimeRemaining <= 0.0f)
    {
        DetachTow();
        Stage = EGTTRecoveryStage::Idle;
        if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
                Economy->PushMessage(TEXT("RECOVERY FAILED: contract timer expired."), 5.0f);
        }
        if (DisabledVehicle) DisabledVehicle->SetActorLocation(BreakdownLocation, false, nullptr, ETeleportType::TeleportPhysics);
        return;
    }

    if (bTowAttached && TowVehicle && DisabledVehicle)
    {
        const float CableDistance = FVector::Distance(TowVehicle->GetActorLocation(), DisabledVehicle->GetActorLocation());
        TowCableLoad = FMath::Clamp((CableDistance - 420.0f) / 260.0f, 0.0f, 1.0f);

        if (CableDistance > 920.0f)
        {
            DetachTow();
            Stage = EGTTRecoveryStage::HookVehicle;
            if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
            {
                if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
                    Economy->PushMessage(TEXT("TOW LINE SNAPPED: return to the disabled van and hook it again."), 5.0f);
            }
        }
    }
}

bool AGTTRecoveryDirector::CanTakeLegalJob(APawn* PlayerPawn) const
{
    return PlayerPawn && UGTTGameplayStatics::GetPlayerWantedLevel(this, 0) <= 0;
}

AGTTVehicleBase* AGTTRecoveryDirector::FindNearbyPlayerVehicle(const FVector& Origin, float Radius) const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(Radius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Candidate = *It;
        if (!Candidate || Candidate == DisabledVehicle) continue;
        const float DistSq = FVector::DistSquared(Candidate->GetActorLocation(), Origin);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = Candidate;
        }
    }
    return Best;
}

void AGTTRecoveryDirector::SpawnRecoveryTarget()
{
    if (!GetWorld() || DisabledVehicle) return;
    DisabledVehicle = GetWorld()->SpawnActor<AGTTRecoveryTargetVehicle>(BreakdownLocation, FRotator(0.0f, -120.0f, 0.0f));
    if (DisabledVehicle)
    {
        DisabledVehicle->ApplyVehicleDamage(78.0f);
        DisabledVehicle->ApplyTireDamage(0.35f);
    }
}

bool AGTTRecoveryDirector::TryStartRecovery(APawn* PlayerPawn)
{
    if (Stage != EGTTRecoveryStage::Idle || !CanTakeLegalJob(PlayerPawn)) return false;
    SpawnRecoveryTarget();
    if (!DisabledVehicle) return false;

    DisabledVehicle->SetActorLocation(BreakdownLocation, false, nullptr, ETeleportType::TeleportPhysics);
    DisabledVehicle->SetActorRotation(FRotator(0.0f, -120.0f, 0.0f), ETeleportType::TeleportPhysics);
    ContractTimeRemaining = ContractTimeLimit;
    Stage = EGTTRecoveryStage::ReachBreakdown;

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->PushMessage(TEXT("RECOVERY CONTRACT: reach EAST ROAD BREAKDOWN, park a working vehicle by the van, then use the recovery hook."), 7.0f);
    return true;
}

bool AGTTRecoveryDirector::TryHookRecoveryVehicle(APawn* PlayerPawn)
{
    if (!PlayerPawn || !DisabledVehicle || (Stage != EGTTRecoveryStage::ReachBreakdown && Stage != EGTTRecoveryStage::HookVehicle)) return false;
    if (FVector::Distance(PlayerPawn->GetActorLocation(), DisabledVehicle->GetActorLocation()) > 650.0f) return false;

    AGTTVehicleBase* Candidate = FindNearbyPlayerVehicle(DisabledVehicle->GetActorLocation(), 900.0f);
    if (!Candidate) return false;

    UPrimitiveComponent* TowRoot = Cast<UPrimitiveComponent>(Candidate->GetRootComponent());
    UPrimitiveComponent* TargetRoot = Cast<UPrimitiveComponent>(DisabledVehicle->GetRootComponent());
    if (!TowRoot || !TargetRoot) return false;

    TowVehicle = Candidate;
    TowConstraint->SetWorldLocation((Candidate->GetActorLocation() + DisabledVehicle->GetActorLocation()) * 0.5f);
    TowConstraint->SetConstrainedComponents(TowRoot, NAME_None, TargetRoot, NAME_None);
    bTowAttached = true;
    TowCableLoad = 0.0f;
    Stage = EGTTRecoveryStage::TowToWorkshop;

    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->PushMessage(TEXT("TOW ATTACHED: pull the disabled Mulebox back to WORKSHOP RECOVERY BAY. Avoid sharp pulls or the line will snap."), 7.0f);
    return true;
}

bool AGTTRecoveryDirector::TryFinishRecovery(APawn* PlayerPawn)
{
    if (!PlayerPawn || Stage != EGTTRecoveryStage::TowToWorkshop || !DisabledVehicle) return false;
    if (FVector::Distance(DisabledVehicle->GetActorLocation(), WorkshopDropLocation) > 850.0f) return false;

    const bool bFast = ContractTimeRemaining > ContractTimeLimit * 0.45f;
    const float ConditionFactor = FMath::Clamp(DisabledVehicle->GetConditionPercent() / 100.0f, 0.35f, 1.0f);
    const int32 Reward = FMath::RoundToInt(BaseReward * ConditionFactor) + (bFast ? FastBonus : 0);

    DetachTow();
    Stage = EGTTRecoveryStage::Completed;
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(PlayerPawn))
        Economy->AddCash(Reward, FString::Printf(TEXT("Roadside recovery payout: $%d%s"), Reward, bFast ? TEXT(" + fast bonus") : TEXT("")));
    return true;
}

void AGTTRecoveryDirector::DetachTow()
{
    if (TowConstraint) TowConstraint->BreakConstraint();
    TowVehicle = nullptr;
    bTowAttached = false;
    TowCableLoad = 0.0f;
}

FString AGTTRecoveryDirector::GetObjectiveText() const
{
    switch (Stage)
    {
        case EGTTRecoveryStage::Idle: return TEXT("RECOVERY: available at workshop");
        case EGTTRecoveryStage::ReachBreakdown: return FString::Printf(TEXT("RECOVERY: reach East Road | %.0fs"), ContractTimeRemaining);
        case EGTTRecoveryStage::HookVehicle: return FString::Printf(TEXT("RECOVERY: re-hook disabled van | %.0fs"), ContractTimeRemaining);
        case EGTTRecoveryStage::TowToWorkshop: return FString::Printf(TEXT("RECOVERY: tow to workshop | %.0fs | cable %.0f%%"), ContractTimeRemaining, TowCableLoad * 100.0f);
        case EGTTRecoveryStage::Completed: return TEXT("RECOVERY: completed");
        default: return TEXT("RECOVERY");
    }
}
