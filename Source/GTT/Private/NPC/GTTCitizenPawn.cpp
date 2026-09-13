#include "NPC/GTTCitizenPawn.h"
#include "Combat/GTTCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "Wanted/GTTWantedComponent.h"
#include "World/GTTDayNightCycle.h"
#include "World/GTTWorldPerformanceSubsystem.h"

AGTTCitizenPawn::AGTTCitizenPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 260.0f, 0.0f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent); BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); BodyMesh->SetRelativeLocation(FVector(0,0,-20)); BodyMesh->SetRelativeScale3D(FVector(.28f,.28f,.85f));
    if (CylinderFinder.Succeeded()) BodyMesh->SetStaticMesh(CylinderFinder.Object);
    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(RootComponent); HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); HeadMesh->SetRelativeLocation(FVector(0,0,65)); HeadMesh->SetRelativeScale3D(FVector(.22f));
    if (SphereFinder.Succeeded()) HeadMesh->SetStaticMesh(SphereFinder.Object);

    CombatProp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CombatProp"));
    CombatProp->SetupAttachment(RootComponent); CombatProp->SetCollisionEnabled(ECollisionEnabled::NoCollision); CombatProp->SetRelativeLocation(FVector(32,25,25)); CombatProp->SetRelativeRotation(FRotator(0,0,-18)); CombatProp->SetRelativeScale3D(FVector(.045f,.045f,.42f)); CombatProp->SetVisibility(false);
    if (CylinderFinder.Succeeded()) CombatProp->SetStaticMesh(CylinderFinder.Object);
    CombatPropDetail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CombatPropDetail"));
    CombatPropDetail->SetupAttachment(RootComponent); CombatPropDetail->SetCollisionEnabled(ECollisionEnabled::NoCollision); CombatPropDetail->SetRelativeLocation(FVector(32,25,64)); CombatPropDetail->SetRelativeScale3D(FVector(.08f,.20f,.06f)); CombatPropDetail->SetVisibility(false);
    if (CubeFinder.Succeeded()) CombatPropDetail->SetStaticMesh(CubeFinder.Object);
}

void AGTTCitizenPawn::BeginPlay()
{
    Super::BeginPlay();
    Health = MaxHealth;
    HomeLocation = GetActorLocation();
    const FVector2D WorkOffset=FMath::RandPointInCircle(1500.0f), SocialOffset=FMath::RandPointInCircle(1100.0f);
    WorkLocation=HomeLocation+FVector(WorkOffset.X,WorkOffset.Y,0); SocialLocation=HomeLocation+FVector(SocialOffset.X,SocialOffset.Y,0);
    DayNightCycle=Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this,AGTTDayNightCycle::StaticClass()));
    LastScheduleCenter=GetScheduleCenter(); ChooseNewWanderTarget();
    ConfigureCombatProp();
}

void AGTTCitizenPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    EGTTWorldSimulationTier SimulationTier = EGTTWorldSimulationTier::Critical;
    const bool bUrgentSimulation = bKnockedOut || CombatTarget.IsValid() || bBrawlParticipant || IsFactionHostile() || HitReactionTimeRemaining > 0.0f || AttackPresentationTimeRemaining > 0.0f;
    if (GetWorld())
    {
        if (UGTTWorldPerformanceSubsystem* Performance = GetWorld()->GetSubsystem<UGTTWorldPerformanceSubsystem>())
        {
            SimulationTier = Performance->GetSimulationTier(this, bUrgentSimulation);
            const float DesiredInterval = Performance->GetRecommendedTickInterval(this, bUrgentSimulation);
            if (!FMath::IsNearlyEqual(GetActorTickInterval(), DesiredInterval, 0.01f))
                SetActorTickInterval(DesiredInterval);
        }
    }

    UpdateCombatPresentation(DeltaSeconds);
    CombatCooldown=FMath::Max(0.0f,CombatCooldown-DeltaSeconds);
    if(bKnockedOut)
    {
        KnockoutTimeRemaining=FMath::Max(0.0f,KnockoutTimeRemaining-DeltaSeconds);
        if(KnockoutTimeRemaining<=0.0f)
        {
            bKnockedOut=false; Health=MaxHealth*.55f; bBrawlParticipant=false; SetActorHiddenInGame(false); SetActorEnableCollision(true); GetCharacterMovement()->SetMovementMode(MOVE_Walking); CombatTarget.Reset(); ChooseNewWanderTarget();
            if (BodyMesh) { BodyMesh->SetRelativeRotation(FRotator::ZeroRotator); BodyMesh->SetRelativeLocation(FVector(0,0,-20)); }
            if (HeadMesh) { HeadMesh->SetRelativeRotation(FRotator::ZeroRotator); HeadMesh->SetRelativeLocation(FVector(0,0,65)); }
        }
        return;
    }
    if(CombatTarget.IsValid()) { UpdateCombatBehavior(DeltaSeconds); return; }

    if(SimulationTier == EGTTWorldSimulationTier::Dormant) return;

    if(!DayNightCycle.IsValid()) DayNightCycle=Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this,AGTTDayNightCycle::StaticClass()));
    RetargetTimeRemaining-=DeltaSeconds;
    const FVector ScheduleCenter=GetScheduleCenter();
    if(FVector::DistSquared2D(ScheduleCenter,LastScheduleCenter)>FMath::Square(100.0f)){ LastScheduleCenter=ScheduleCenter; ChooseNewWanderTarget(); }
    FVector ToTarget=WanderTarget-GetActorLocation(); ToTarget.Z=0;
    if(RetargetTimeRemaining<=0 || ToTarget.SizeSquared2D()<FMath::Square(90.0f)){ ChooseNewWanderTarget(); ToTarget=WanderTarget-GetActorLocation(); ToTarget.Z=0; }
    if(!ToTarget.IsNearlyZero()) AddMovementInput(ToTarget.GetSafeNormal2D(),1.0f);
}

void AGTTCitizenPawn::StartBrawlWith(APawn* Opponent)
{
    if(!Opponent||bKnockedOut) return;
    bBrawlParticipant=true;
    CombatTarget=Opponent;
    Health=MaxHealth;
    SetActorTickInterval(0.0f);
    GetCharacterMovement()->MaxWalkSpeed=235.0f;
    ConfigureCombatProp();
}

void AGTTCitizenPawn::ConfigureHostileArchetype(EGTTHostileArchetype NewArchetype, APawn* Target)
{
    HostileArchetype=NewArchetype;
    bBrawlParticipant=true;
    SetActorTickInterval(0.0f);
    switch(NewArchetype)
    {
        case EGTTHostileArchetype::Runner:
            MaxHealth=72.0f; RetaliationDamage=5.0f; RetaliationDistance=150.0f; GetCharacterMovement()->MaxWalkSpeed=365.0f;
            if(BodyMesh) BodyMesh->SetRelativeScale3D(FVector(.23f,.23f,.78f));
            break;
        case EGTTHostileArchetype::Bruiser:
            MaxHealth=165.0f; RetaliationDamage=13.0f; RetaliationDistance=205.0f; GetCharacterMovement()->MaxWalkSpeed=190.0f;
            if(BodyMesh) BodyMesh->SetRelativeScale3D(FVector(.38f,.38f,.95f));
            break;
        case EGTTHostileArchetype::Enforcer:
            MaxHealth=125.0f; RetaliationDamage=10.0f; RetaliationDistance=250.0f; GetCharacterMovement()->MaxWalkSpeed=255.0f;
            if(BodyMesh) BodyMesh->SetRelativeScale3D(FVector(.31f,.31f,.9f));
            break;
        case EGTTHostileArchetype::Scrapper:
            MaxHealth=100.0f; RetaliationDamage=8.5f; RetaliationDistance=180.0f; GetCharacterMovement()->MaxWalkSpeed=245.0f;
            break;
        default:
            HostileArchetype=EGTTHostileArchetype::Civilian;
            break;
    }
    Health=MaxHealth;
    if(Target) CombatTarget=Target;
    ConfigureCombatProp();
}

void AGTTCitizenPawn::ConfigureCombatProp()
{
    if (!CombatProp || !CombatPropDetail) return;
    const bool bShow = HostileArchetype != EGTTHostileArchetype::Civilian;
    CombatProp->SetVisibility(bShow);
    CombatPropDetail->SetVisibility(bShow);
    if (!bShow) return;

    CombatProp->SetRelativeLocation(FVector(32,25,25));
    CombatProp->SetRelativeRotation(FRotator(0,0,-18));
    CombatPropDetail->SetRelativeLocation(FVector(32,25,64));
    switch (HostileArchetype)
    {
        case EGTTHostileArchetype::Runner:
            CombatProp->SetRelativeScale3D(FVector(.035f,.035f,.32f));
            CombatPropDetail->SetRelativeScale3D(FVector(.07f,.12f,.05f));
            break;
        case EGTTHostileArchetype::Bruiser:
            CombatProp->SetRelativeScale3D(FVector(.075f,.075f,.44f));
            CombatPropDetail->SetRelativeScale3D(FVector(.15f,.26f,.09f));
            break;
        case EGTTHostileArchetype::Enforcer:
            CombatProp->SetRelativeScale3D(FVector(.05f,.05f,.52f));
            CombatPropDetail->SetRelativeScale3D(FVector(.10f,.22f,.07f));
            break;
        default:
            CombatProp->SetRelativeScale3D(FVector(.045f,.045f,.40f));
            CombatPropDetail->SetRelativeScale3D(FVector(.09f,.18f,.06f));
            break;
    }
}

FString AGTTCitizenPawn::GetArchetypeLabel() const
{
    switch(HostileArchetype)
    {
        case EGTTHostileArchetype::Runner: return TEXT("RUNNER");
        case EGTTHostileArchetype::Bruiser: return TEXT("BRUISER");
        case EGTTHostileArchetype::Enforcer: return TEXT("ENFORCER");
        case EGTTHostileArchetype::Scrapper: return TEXT("SCRAPPER");
        default: return TEXT("CIVILIAN");
    }
}

void AGTTCitizenPawn::ApplyCombatHit(float Damage, const FVector& HitDirection, float Knockback, APawn* Attacker)
{
    if(bKnockedOut||Damage<=0.0f) return;
    SetActorTickInterval(0.0f);
    Health=FMath::Max(0.0f,Health-Damage);
    LastHitDirection = HitDirection.GetSafeNormal2D();
    HitReactionTimeRemaining = 0.34f;
    LaunchCharacter(LastHitDirection*Knockback+FVector(0,0,FMath::Min(180.0f,Knockback*.35f)),true,true);
    CombatTarget=Attacker;
    if(Health<=0.0f)
    {
        bKnockedOut=true; KnockoutTimeRemaining=22.0f; CombatTarget.Reset(); GetCharacterMovement()->DisableMovement(); SetActorEnableCollision(false);
        if (BodyMesh) BodyMesh->SetRelativeRotation(FRotator(0,78,86));
        if (HeadMesh) HeadMesh->SetRelativeLocation(FVector(18,0,12));
        return;
    }
    if(HostileArchetype==EGTTHostileArchetype::Civilian) GetCharacterMovement()->MaxWalkSpeed = Health < MaxHealth*.30f ? 310.0f : 225.0f;
}

void AGTTCitizenPawn::UpdateCombatPresentation(float DeltaSeconds)
{
    HitReactionTimeRemaining=FMath::Max(0.0f,HitReactionTimeRemaining-DeltaSeconds);
    AttackPresentationTimeRemaining=FMath::Max(0.0f,AttackPresentationTimeRemaining-DeltaSeconds);
    if (bKnockedOut) return;

    const float HitAlpha = HitReactionTimeRemaining > 0.0f ? HitReactionTimeRemaining / 0.34f : 0.0f;
    const float HitPulse = HitAlpha > 0.0f ? FMath::Sin(HitAlpha * PI) : 0.0f;
    if (BodyMesh)
    {
        const float Side = FMath::Clamp(LastHitDirection.Y, -1.0f, 1.0f);
        BodyMesh->SetRelativeRotation(FRotator(-20.0f*HitPulse,0,Side*24.0f*HitPulse));
        BodyMesh->SetRelativeLocation(FVector(-10.0f*HitPulse,0,-20));
    }
    if (HeadMesh)
    {
        HeadMesh->SetRelativeRotation(FRotator(-12.0f*HitPulse,0,LastHitDirection.Y*18.0f*HitPulse));
        HeadMesh->SetRelativeLocation(FVector(-7.0f*HitPulse,0,65+5.0f*HitPulse));
    }

    if (CombatProp && CombatProp->IsVisible())
    {
        const float AttackAlpha = AttackPresentationTimeRemaining > 0.0f ? AttackPresentationTimeRemaining / 0.30f : 0.0f;
        const float Swing = AttackAlpha > 0.0f ? FMath::Sin(AttackAlpha * PI) : 0.0f;
        CombatProp->SetRelativeRotation(FRotator(65.0f*Swing,18.0f*Swing,-18.0f));
        CombatPropDetail->SetRelativeRotation(FRotator(65.0f*Swing,18.0f*Swing,-18.0f));
    }
}

void AGTTCitizenPawn::UpdateCombatBehavior(float DeltaSeconds)
{
    APawn* Target=CombatTarget.Get();
    if(!Target){ CombatTarget.Reset(); return; }
    FVector ToTarget=Target->GetActorLocation()-GetActorLocation(); ToTarget.Z=0;
    const float Distance=ToTarget.Size2D();
    const float ChaseLimit=HostileArchetype==EGTTHostileArchetype::Civilian?2600.0f:4200.0f;
    if(Distance>ChaseLimit){ CombatTarget.Reset(); bBrawlParticipant=false; GetCharacterMovement()->MaxWalkSpeed=WanderSpeed; ChooseNewWanderTarget(); return; }
    if(Health < MaxHealth*.30f && !bBrawlParticipant){ AddMovementInput((-ToTarget).GetSafeNormal2D(),1.0f); return; }
    if(Distance>RetaliationDistance) AddMovementInput(ToTarget.GetSafeNormal2D(),1.0f);
    else if(CombatCooldown<=0.0f)
    {
        const float MinCooldown=HostileArchetype==EGTTHostileArchetype::Runner?.55f:.78f;
        const float MaxCooldown=HostileArchetype==EGTTHostileArchetype::Bruiser?1.35f:1.05f;
        CombatCooldown=FMath::FRandRange(MinCooldown,MaxCooldown);
        AttackPresentationTimeRemaining=0.30f;
        if(UGTTCombatComponent* Combat=Target->FindComponentByClass<UGTTCombatComponent>()) Combat->ApplyIncomingDamage(RetaliationDamage, bBrawlParticipant?FString::Printf(TEXT("%s hit"),*GetArchetypeLabel()):TEXT("Villager retaliation"));
    }
}

bool AGTTCitizenPawn::TryWitnessVehicleTheft(AGTTVehicleBase* Vehicle, APawn* Offender)
{
    if(!Vehicle||!Offender||LastWitnessedVehicle.Get()==Vehicle||!GetWorld()||bKnockedOut) return false;
    if(FVector::DistSquared2D(GetActorLocation(),Vehicle->GetActorLocation())>FMath::Square(WitnessRadius)) return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(GTTWitnessSight),false,this); Params.AddIgnoredActor(this); Params.AddIgnoredActor(Offender);
    const FVector Start=GetActorLocation()+FVector(0,0,70), End=Vehicle->GetActorLocation()+FVector(0,0,80);
    const bool bBlocked=GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params);
    if(bBlocked&&Hit.GetActor()!=Vehicle) return false;
    LastWitnessedVehicle=Vehicle;
    if(UGTTWantedComponent* Wanted=UGTTGameplayStatics::FindWantedComponentForPawn(Offender)) Wanted->AddHeat(WitnessHeat);
    if(UGTTPlayerEconomyComponent* Economy=UGTTGameplayStatics::FindEconomyComponentForPawn(Offender)) Economy->PushMessage(TEXT("A villager saw the theft and called the police!"),5.0f);
    return true;
}

FVector AGTTCitizenPawn::GetScheduleCenter() const
{
    const AGTTDayNightCycle* Cycle=DayNightCycle.Get(); if(!Cycle) return HomeLocation;
    const float Hour=Cycle->GetTimeOfDayHours();
    if(Hour>=7.0f&&Hour<17.5f) return WorkLocation;
    if(Hour>=17.5f&&Hour<21.5f) return SocialLocation;
    return HomeLocation;
}

void AGTTCitizenPawn::ChooseNewWanderTarget()
{
    const FVector Center=GetScheduleCenter();
    const float Radius=DayNightCycle.IsValid()&&DayNightCycle->IsNight()?WanderRadius*.25f:WanderRadius;
    const FVector2D Offset=FMath::RandPointInCircle(Radius); WanderTarget=Center+FVector(Offset.X,Offset.Y,0); RetargetTimeRemaining=FMath::FRandRange(3.0f,8.0f);
}
