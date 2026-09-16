#include "NPC/GTTLogisticsDispatcherPawn.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GTTDayNightCycle.h"

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
        const FString Shift = IsOnShift() ? TEXT("ON SHIFT") : TEXT("OFF SHIFT");
        RoleLabel->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), *DisplayName.ToUpper(), *Shift)));
    }
}
