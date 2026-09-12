#include "NPC/GTTCitizenPawn.h"
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
    HomeLocation = GetActorLocation();
    const FVector2D WorkOffset=FMath::RandPointInCircle(1500.0f), SocialOffset=FMath::RandPointInCircle(1100.0f);
    WorkLocation=HomeLocation+FVector(WorkOffset.X,WorkOffset.Y,0); SocialLocation=HomeLocation+FVector(SocialOffset.X,SocialOffset.Y,0);
    DayNightCycle=Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this,AGTTDayNightCycle::StaticClass()));
    LastScheduleCenter=GetScheduleCenter(); ChooseNewWanderTarget();
}

void AGTTCitizenPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!DayNightCycle.IsValid()) DayNightCycle=Cast<AGTTDayNightCycle>(UGameplayStatics::GetActorOfClass(this,AGTTDayNightCycle::StaticClass()));
    RetargetTimeRemaining-=DeltaSeconds;
    const FVector ScheduleCenter=GetScheduleCenter();
    if(FVector::DistSquared2D(ScheduleCenter,LastScheduleCenter)>FMath::Square(100.0f)){ LastScheduleCenter=ScheduleCenter; ChooseNewWanderTarget(); }
    FVector ToTarget=WanderTarget-GetActorLocation(); ToTarget.Z=0;
    if(RetargetTimeRemaining<=0 || ToTarget.SizeSquared2D()<FMath::Square(90.0f)){ ChooseNewWanderTarget(); ToTarget=WanderTarget-GetActorLocation(); ToTarget.Z=0; }
    if(!ToTarget.IsNearlyZero()) AddMovementInput(ToTarget.GetSafeNormal2D(),1.0f);
}

bool AGTTCitizenPawn::TryWitnessVehicleTheft(AGTTVehicleBase* Vehicle, APawn* Offender)
{
    if(!Vehicle||!Offender||LastWitnessedVehicle.Get()==Vehicle||!GetWorld()) return false;
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
