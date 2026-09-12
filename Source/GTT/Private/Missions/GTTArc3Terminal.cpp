#include "Missions/GTTArc3Terminal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTArc3Director.h"
#include "UObject/ConstructorHelpers.h"

AGTTArc3Terminal::AGTTArc3Terminal()
{
    PrimaryActorTick.bCanEverTick = false;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded()) Mesh->SetStaticMesh(CubeFinder.Object);
    Mesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.1f));

    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
    Sign->SetupAttachment(Mesh);
    Sign->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
    Sign->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Sign->SetHorizontalAlignment(EHTA_Center);
    Sign->SetWorldSize(40.0f);
    Sign->SetTextRenderColor(FColor(255, 170, 70));
    SetTerminalType(EGTTArc3TerminalType::FarmOffice);
}

void AGTTArc3Terminal::SetTerminalType(EGTTArc3TerminalType NewType)
{
    TerminalType = NewType;
    if (!Sign) return;
    switch (TerminalType)
    {
        case EGTTArc3TerminalType::FarmOffice: Sign->SetText(FText::FromString(TEXT("ARC 3 / FARM OFFICE"))); break;
        case EGTTArc3TerminalType::RedBarn: Sign->SetText(FText::FromString(TEXT("RED BARN / RECKONING"))); break;
        case EGTTArc3TerminalType::CountyDrop: Sign->SetText(FText::FromString(TEXT("COUNTY EVIDENCE DROP"))); break;
    }
}

void AGTTArc3Terminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTArc3Director* Director = Cast<AGTTArc3Director>(UGameplayStatics::GetActorOfClass(this, AGTTArc3Director::StaticClass()));
    if (!Pawn || !Director) return;

    switch (TerminalType)
    {
        case EGTTArc3TerminalType::FarmOffice: Director->TryFarmContact(Pawn); break;
        case EGTTArc3TerminalType::RedBarn: Director->TryRedBarn(Pawn); break;
        case EGTTArc3TerminalType::CountyDrop: Director->TryCountyDrop(Pawn); break;
    }
}

FText AGTTArc3Terminal::GetInteractionText_Implementation() const
{
    switch (TerminalType)
    {
        case EGTTArc3TerminalType::FarmOffice: return NSLOCTEXT("GTT", "Arc3Farm", "Continue Main Story Arc 3");
        case EGTTArc3TerminalType::RedBarn: return NSLOCTEXT("GTT", "Arc3Barn", "Confront Red Barn crew / recover ledger");
        case EGTTArc3TerminalType::CountyDrop: return NSLOCTEXT("GTT", "Arc3County", "Deliver payoff ledger evidence");
    }
    return FText::GetEmpty();
}
