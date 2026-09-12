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

AGTTCitizenPawn::AGTTCitizenPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCharacterMovement()->bRunPhysicsWithNoController = true;
    GetCharacterMovement()->MaxWalkSpeed = WanderSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 260.0f, 0.0f);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent); BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); BodyMesh->SetRelativeLocation(FVector(0,0,-20)); BodyMesh->SetRelativeScale3D(FVector(.28f,.28f,.85f));
    if (CylinderFinder.Succeeded()) BodyMesh->SetStaticMesh(CylinderFinder.Object);
    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(RootComponent); HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); HeadMesh->SetRelativeLocation(FVector(0,0,65)); HeadMesh->SetRelativeScale3D(FVector(.22f));
    if (SphereFinder.Succeeded()) HeadMesh->SetStaticMesh(SphereFinder.Object);
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
}

void AGTTCitizenPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    CombatCooldown=FMath::Max(0.0f,CombatCooldown-DeltaSeconds);
    if(bKnockedOut)
    {
        KnockoutTimeRemaining=FMath::Max(0.0f,KnockoutTimeRemaining-DeltaSeconds);
        if(KnockoutTimeRemaining<=0.0f)
        {
            bKnockedOut=false; Health=MaxHealth*.55f; bBrawlParticipant=false; SetActorHiddenInGame(false); SetActorEnableCollision(true); GetCharacterMovement()->SetMovementMode(MOVE_Walking); CombatTarget.Reset(); ChooseNewWanderTarget();
        }
        return;
    }
    if(CombatTarget.IsValid()) { UpdateCombatBehavior(DeltaSeconds); return; }
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
    GetCharacterMovement()->MaxWalkSpeed=235.0f;
}

void AGTTCitizenPawn::ConfigureHostileArchetype(EGTTHostileArchetype NewArchetype, APawn* Target)
{
    HostileArchetype=NewArchetype;
    bBrawlParticipant=true;
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
    Health=FMath::Max(0.0f,Health-Damage);
    LaunchCharacter(HitDirection.GetSafeNormal2D()*Knockback+FVector(0,0,FMath::Min(180.0f,Knockback*.35f)),true,true);
    CombatTarget=Attacker;
    if(Health<=0.0f)
    {
        bKnockedOut=true; KnockoutTimeRemaining=22.0f; CombatTarget.Reset(); GetCharacterMovement()->DisableMovement(); SetActorEnableCollision(false); SetActorHiddenInGame(true);
        return;
    }
    if(HostileArchetype==EGTTHostileArchetype::Civilian) GetCharacterMovement()->MaxWalkSpeed = Health < MaxHealth*.30f ? 310.0f : 225.0f;
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
