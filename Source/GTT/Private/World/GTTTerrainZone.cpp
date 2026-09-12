#include "World/GTTTerrainZone.h"

#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Vehicles/GTTVehicleBase.h"

AGTTTerrainZone::AGTTTerrainZone()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Trigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    Trigger->SetGenerateOverlapEvents(true);
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGTTTerrainZone::HandleBeginOverlap);
    Trigger->OnComponentEndOverlap.AddDynamic(this, &AGTTTerrainZone::HandleEndOverlap);

    ZoneLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ZoneLabel"));
    ZoneLabel->SetupAttachment(Trigger);
    ZoneLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));
    ZoneLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    ZoneLabel->SetHorizontalAlignment(EHTA_Center);
    ZoneLabel->SetWorldSize(32.0f);
    ZoneLabel->SetTextRenderColor(FColor(205, 160, 80));
    ZoneLabel->SetCastShadow(true);
}

void AGTTTerrainZone::ConfigureZone(EGTTVehicleTerrainType InTerrainType, const FVector& BoxExtent, const FString& Label)
{
    TerrainType = InTerrainType;
    Trigger->SetBoxExtent(BoxExtent);
    ZoneLabel->SetText(FText::FromString(Label));
}

void AGTTTerrainZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(OtherActor))
    {
        ApplyToVehicle(Vehicle);
    }
}

void AGTTTerrainZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AGTTVehicleBase* Vehicle = Cast<AGTTVehicleBase>(OtherActor))
    {
        Vehicle->ClearTerrainHandling(this);
    }
}

void AGTTTerrainZone::ApplyToVehicle(AGTTVehicleBase* Vehicle) const
{
    if (!Vehicle) return;

    switch (TerrainType)
    {
    case EGTTVehicleTerrainType::Mud:
        Vehicle->SetTerrainHandling(TEXT("MUD"), 0.70f, 1.45f, 0.86f, const_cast<AGTTTerrainZone*>(this));
        break;
    case EGTTVehicleTerrainType::DeepMud:
        Vehicle->SetTerrainHandling(TEXT("DEEP MUD"), 0.48f, 2.05f, 0.72f, const_cast<AGTTTerrainZone*>(this));
        break;
    default:
        Vehicle->SetTerrainHandling(TEXT("DIRT"), 0.86f, 1.18f, 0.94f, const_cast<AGTTTerrainZone*>(this));
        break;
    }
}
