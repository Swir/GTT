#include "World/GTTRuralEconomyTerminal.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"
#include "World/GTTRuralEconomySubsystem.h"

AGTTRuralEconomyTerminal::AGTTRuralEconomyTerminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
}

void AGTTRuralEconomyTerminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    UWorld* World = GetWorld();
    UGTTRuralEconomySubsystem* EconomySystem = World ? World->GetSubsystem<UGTTRuralEconomySubsystem>() : nullptr;
    if (!Pawn || !EconomySystem) return;

    switch (TerminalType)
    {
        case EGTTRuralEconomyTerminalType::Fence: EconomySystem->SellContraband(Pawn); break;
        case EGTTRuralEconomyTerminalType::Insurance: EconomySystem->BuyOrUseInsurance(Pawn); break;
        case EGTTRuralEconomyTerminalType::Impound: EconomySystem->ReleaseImpoundedVehicle(Pawn); break;
    }
}

FText AGTTRuralEconomyTerminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTRuralEconomyTerminalType::Fence:
            return NSLOCTEXT("GTT", "FenceTerminal", "Backlot fence: sell contraband");
        case EGTTRuralEconomyTerminalType::Insurance:
            return NSLOCTEXT("GTT", "InsuranceTerminal", "Farm Mutual: buy policy / claim repair");
        case EGTTRuralEconomyTerminalType::Impound:
            return NSLOCTEXT("GTT", "ImpoundTerminal", "County impound: release owned vehicle");
    }
    return FText::GetEmpty();
}
