#include "Missions/GTTArc4Terminal.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Missions/GTTArc4Director.h"
#include "UObject/ConstructorHelpers.h"

AGTTArc4Terminal::AGTTArc4Terminal()
{
    PrimaryActorTick.bCanEverTick = false;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    SetRootComponent(Mesh);
    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
    Sign->SetupAttachment(Mesh);
    Sign->SetRelativeLocation(FVector(0,0,120));
    Sign->SetHorizontalAlignment(EHTA_Center);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) Mesh->SetStaticMesh(Cube.Object);
    SetTerminalType(TerminalType);
}

void AGTTArc4Terminal::SetTerminalType(EGTTArc4TerminalType NewType)
{
    TerminalType = NewType;
    if (!Sign) return;
    const TCHAR* Label = TerminalType == EGTTArc4TerminalType::NorthPass ? TEXT("NORTH PASS") :
        (TerminalType == EGTTArc4TerminalType::RidgeExchange ? TEXT("RIDGE EXCHANGE") : TEXT("FARM OFFICE - ARC 4"));
    Sign->SetText(FText::FromString(Label));
}

void AGTTArc4Terminal::Interact_Implementation(AActor* Interactor)
{
    APawn* Pawn = Cast<APawn>(Interactor);
    AGTTArc4Director* Director = Cast<AGTTArc4Director>(UGameplayStatics::GetActorOfClass(this, AGTTArc4Director::StaticClass()));
    if (!Pawn || !Director) return;
    if (TerminalType == EGTTArc4TerminalType::FarmOffice) Director->TryFarmContact(Pawn);
    else if (TerminalType == EGTTArc4TerminalType::NorthPass) Director->TryNorthPass(Pawn);
    else Director->TryRidgeExchange(Pawn);
}

FText AGTTArc4Terminal::GetInteractionText_Implementation() const
{
    if (TerminalType == EGTTArc4TerminalType::NorthPass) return FText::FromString(TEXT("Hit North Pass checkpoint"));
    if (TerminalType == EGTTArc4TerminalType::RidgeExchange) return FText::FromString(TEXT("Make ridge exchange"));
    return FText::FromString(TEXT("Main Story Arc 4"));
}
