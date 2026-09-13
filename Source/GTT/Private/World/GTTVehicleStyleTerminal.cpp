#include "World/GTTVehicleStyleTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Core/GTTGameMode.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"
#include "Vehicles/GTTOldCarPawn.h"
#include "Vehicles/GTTTractorPawn.h"
#include "Vehicles/GTTVehicleBase.h"

namespace
{
    const FName CustomVisualTag(TEXT("GTT_CustomVisual"));
}

AGTTVehicleStyleTerminal::AGTTVehicleStyleTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.0f));
    if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"))) Mesh->SetStaticMesh(Cube);
}

void AGTTVehicleStyleTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UGTTPlayerEconomyComponent* Economy = Pawn ? UGTTGameplayStatics::FindEconomyComponentForPawn(Pawn) : nullptr;
    if (!Pawn || !Economy) return;

    AGTTVehicleBase* Vehicle = FindNearestOwnedVehicle();
    if (!Vehicle)
    {
        Economy->PushMessage(TEXT("BODY SHOP: park an owned vehicle close to the workshop."), 4.0f);
        return;
    }

    bool bChanged = false;
    switch (ServiceType)
    {
        case EGTTVehicleStyleService::TractorVisual: bChanged = ApplyTractorPackage(Vehicle, Economy); break;
        case EGTTVehicleStyleService::OldCarVariant: bChanged = ApplyOldCarVariant(Vehicle, Economy); break;
        case EGTTVehicleStyleService::BodyPanels: bChanged = ReplaceBodyPanels(Vehicle, Economy); break;
    }

    if (bChanged)
    {
        if (AGTTGameMode* GameMode = Cast<AGTTGameMode>(UGameplayStatics::GetGameMode(this))) GameMode->SaveProgress();
    }
}

FText AGTTVehicleStyleTerminal::GetInteractionText_Implementation() const
{
    switch (ServiceType)
    {
        case EGTTVehicleStyleService::TractorVisual: return NSLOCTEXT("GTT", "TractorStyle", "Install Fieldmaster visual package");
        case EGTTVehicleStyleService::OldCarVariant: return NSLOCTEXT("GTT", "RattlebackVariant", "Build Rattleback street variant");
        case EGTTVehicleStyleService::BodyPanels: return NSLOCTEXT("GTT", "BodyPanels", "Replace missing body panels");
    }
    return NSLOCTEXT("GTT", "BodyShop", "Vehicle body shop");
}

AGTTVehicleBase* AGTTVehicleStyleTerminal::FindNearestOwnedVehicle() const
{
    if (!GetWorld()) return nullptr;
    AGTTVehicleBase* Best = nullptr;
    float BestDistSq = FMath::Square(VehicleSearchRadius);
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || !Vehicle->IsOwnedByPlayer()) continue;
        const float DistSq = FVector::DistSquared2D(GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq <= BestDistSq)
        {
            Best = Vehicle;
            BestDistSq = DistSq;
        }
    }
    return Best;
}

bool AGTTVehicleStyleTerminal::ApplyTractorPackage(AGTTVehicleBase* Vehicle, UGTTPlayerEconomyComponent* Economy)
{
    if (!Cast<AGTTTractorPawn>(Vehicle))
    {
        Economy->PushMessage(TEXT("FIELDMASTER STYLE: this bay only accepts the Rusty Fieldmaster 60."), 4.0f);
        return false;
    }

    const int32 Stage = FMath::Min(Vehicle->GetEngineUpgradeLevel(), 2) + FMath::Min(Vehicle->GetTireUpgradeLevel(), 1);
    if (Stage >= 3)
    {
        RebuildVisualPackage(Vehicle);
        Economy->PushMessage(TEXT("FIELDMASTER STYLE: full brush guard, work lights and rear toolbox already fitted."), 4.0f);
        return false;
    }

    const int32 Cost = VisualPackageBaseCost + Stage * 140;
    if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Fieldmaster visual package - $%d"), Cost))) return false;

    if (Vehicle->GetEngineUpgradeLevel() < 2) Vehicle->InstallEngineUpgrade();
    else Vehicle->InstallTireUpgrade();
    RebuildVisualPackage(Vehicle);
    Economy->PushMessage(FString::Printf(TEXT("FIELDMASTER STYLE stage %d/3 installed. Package also uses the persistent tune level."), Stage + 1), 5.0f);
    return true;
}

bool AGTTVehicleStyleTerminal::ApplyOldCarVariant(AGTTVehicleBase* Vehicle, UGTTPlayerEconomyComponent* Economy)
{
    if (!Cast<AGTTOldCarPawn>(Vehicle))
    {
        Economy->PushMessage(TEXT("RATTLEBACK VARIANT: bring your Rattleback 82 to this bay."), 4.0f);
        return false;
    }

    const int32 Stage = FMath::Min(Vehicle->GetEngineUpgradeLevel(), 2) + FMath::Min(Vehicle->GetTireUpgradeLevel(), 1);
    if (Stage >= 3)
    {
        RebuildVisualPackage(Vehicle);
        Economy->PushMessage(TEXT("RATTLEBACK STREET: full intake, ducktail and wide-bumper package already fitted."), 4.0f);
        return false;
    }

    const int32 Cost = VisualPackageBaseCost + 60 + Stage * 180;
    if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Rattleback street variant - $%d"), Cost))) return false;

    if (Vehicle->GetEngineUpgradeLevel() < 2) Vehicle->InstallEngineUpgrade();
    else Vehicle->InstallTireUpgrade();
    RebuildVisualPackage(Vehicle);
    Economy->PushMessage(FString::Printf(TEXT("RATTLEBACK STREET stage %d/3 installed: persistent power/grip tune plus visible body changes."), Stage + 1), 5.0f);
    return true;
}

bool AGTTVehicleStyleTerminal::ReplaceBodyPanels(AGTTVehicleBase* Vehicle, UGTTPlayerEconomyComponent* Economy)
{
    const int32 Missing = Vehicle->GetDetachedPartCount();
    if (Missing <= 0)
    {
        Economy->PushMessage(TEXT("BODY PANELS: nothing is missing from this vehicle."), 3.5f);
        return false;
    }

    const int32 Cost = FMath::Max(90, Missing * PanelReplacementCostPerPart);
    if (!Economy->SpendCash(Cost, FString::Printf(TEXT("Replacement body panels - $%d"), Cost))) return false;

    // Existing repair logic reattaches registered doors/fenders/hood/bumper/wheel stages once body condition is restored.
    Vehicle->RepairVehicle(10000.0f);
    RebuildVisualPackage(Vehicle);
    Economy->PushMessage(FString::Printf(TEXT("BODY SHOP: replaced %d missing part(s) and realigned the body for $%d."), Missing, Cost), 5.0f);
    return true;
}

void AGTTVehicleStyleTerminal::RebuildVisualPackage(AGTTVehicleBase* Vehicle) const
{
    if (!Vehicle) return;

    TArray<UStaticMeshComponent*> Components;
    Vehicle->GetComponents<UStaticMeshComponent>(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (Component && Component->ComponentHasTag(CustomVisualTag)) Component->DestroyComponent();
    }

    const int32 EngineLevel = Vehicle->GetEngineUpgradeLevel();
    const int32 TireLevel = Vehicle->GetTireUpgradeLevel();

    if (Cast<AGTTTractorPawn>(Vehicle))
    {
        if (EngineLevel >= 1) AddVisualPart(Vehicle, TEXT("FieldmasterBrushGuard"), FVector(150, 0, 35), FVector(0.16f, 1.2f, 0.45f));
        if (EngineLevel >= 2)
        {
            AddVisualPart(Vehicle, TEXT("FieldmasterLightBar"), FVector(-35, 0, 245), FVector(0.18f, 0.9f, 0.12f));
            AddVisualPart(Vehicle, TEXT("FieldmasterWorkLampL"), FVector(-25, -70, 245), FVector(0.18f));
            AddVisualPart(Vehicle, TEXT("FieldmasterWorkLampR"), FVector(-25, 70, 245), FVector(0.18f));
        }
        if (TireLevel >= 1) AddVisualPart(Vehicle, TEXT("FieldmasterToolbox"), FVector(-135, 0, 70), FVector(0.55f, 0.95f, 0.28f));
    }
    else if (Cast<AGTTOldCarPawn>(Vehicle))
    {
        if (EngineLevel >= 1) AddVisualPart(Vehicle, TEXT("RattlebackHoodScoop"), FVector(70, 0, 85), FVector(0.45f, 0.35f, 0.18f));
        if (EngineLevel >= 2) AddVisualPart(Vehicle, TEXT("RattlebackDucktail"), FVector(-140, 0, 78), FVector(0.22f, 0.9f, 0.16f));
        if (TireLevel >= 1)
        {
            AddVisualPart(Vehicle, TEXT("RattlebackFrontLip"), FVector(145, 0, -12), FVector(0.16f, 1.12f, 0.11f));
            AddVisualPart(Vehicle, TEXT("RattlebackRearLip"), FVector(-145, 0, -12), FVector(0.16f, 1.12f, 0.11f));
        }
    }
}

UStaticMeshComponent* AGTTVehicleStyleTerminal::AddVisualPart(AGTTVehicleBase* Vehicle, FName Name, const FVector& Location, const FVector& Scale) const
{
    if (!Vehicle) return nullptr;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube) return nullptr;

    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(Vehicle, Name);
    Part->ComponentTags.Add(CustomVisualTag);
    Part->SetStaticMesh(Cube);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetGenerateOverlapEvents(false);
    Part->SetRelativeLocation(Location);
    Part->SetRelativeScale3D(Scale);
    Part->RegisterComponent();
    Part->AttachToComponent(Vehicle->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    return Part;
}
