#include "NPC/GTTSocialNPC.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/GTTGameplayStatics.h"
#include "Economy/GTTPlayerEconomyComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Vehicles/GTTVehicleBase.h"
#include "World/GTTDayNightCycle.h"

AGTTSocialNPC::AGTTSocialNPC()
{
    PrimaryActorTick.bCanEverTick = false;

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    SetRootComponent(BodyMesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CubeFinder.Succeeded()) BodyMesh->SetStaticMesh(CubeFinder.Object);
    BodyMesh->SetRelativeScale3D(FVector(0.34f, 0.34f, 0.9f));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
    HeadMesh->SetupAttachment(BodyMesh);
    if (SphereFinder.Succeeded()) HeadMesh->SetStaticMesh(SphereFinder.Object);
    HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 92.0f));
    HeadMesh->SetRelativeScale3D(FVector(0.55f));
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
    NameText->SetupAttachment(BodyMesh);
    NameText->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
    NameText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
    NameText->SetHorizontalAlignment(EHTA_Center);
    NameText->SetWorldSize(28.0f);
    NameText->SetText(FText::FromString(DisplayName));
}

void AGTTSocialNPC::ConfigureSocialRole(EGTTSocialRole InRole, const FString& InDisplayName)
{
    Role = InRole;
    DisplayName = InDisplayName;
    if (NameText) NameText->SetText(FText::FromString(DisplayName));
}

void AGTTSocialNPC::Interact_Implementation(AActor* Interactor)
{
    if (!Interactor) return;
    const FString Line = BuildConversation(Interactor);
    if (UGTTPlayerEconomyComponent* Economy = UGTTGameplayStatics::FindEconomyComponentForPawn(Cast<APawn>(Interactor)))
    {
        Economy->PushMessage(FString::Printf(TEXT("%s: %s"), *DisplayName, *Line), 6.0f);
    }
    ++ConversationIndex;
}

FText AGTTSocialNPC::GetInteractionText_Implementation() const
{
    return FText::FromString(FString::Printf(TEXT("Talk to %s"), *DisplayName));
}

FString AGTTSocialNPC::BuildConversation(AActor* Interactor)
{
    float Hour = 12.0f;
    for (TActorIterator<AGTTDayNightCycle> It(GetWorld()); It; ++It)
    {
        Hour = It->GetTimeOfDayHours();
        break;
    }
    const bool bPartyHours = Hour >= 18.5f || Hour < 2.5f;

    AGTTVehicleBase* NearestOwnedVehicle = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();
    for (TActorIterator<AGTTVehicleBase> It(GetWorld()); It; ++It)
    {
        AGTTVehicleBase* Vehicle = *It;
        if (!Vehicle || !Vehicle->IsOwnedByPlayer()) continue;
        const float DistSq = FVector::DistSquared(Interactor->GetActorLocation(), Vehicle->GetActorLocation());
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            NearestOwnedVehicle = Vehicle;
        }
    }

    switch (Role)
    {
        case EGTTSocialRole::Bartender:
            if (!bPartyHours) return ConversationIndex % 2 == 0 ? TEXT("Quiet before dark. The Bent Axle gets busy after 18:30.") : TEXT("Legal work pays steady; trouble pays fast and costs more later.");
            return ConversationIndex % 3 == 0 ? TEXT("Crowd is full tonight. Keep the fists outside unless you want the whole room watching.")
                : ConversationIndex % 3 == 1 ? TEXT("Rangers care about the forest, police care about the roads. Mix both and your night gets expensive.")
                : TEXT("North Pass people have long memories. Faction victories travel faster than tractors.");

        case EGTTSocialRole::HallOrganizer:
            return bPartyHours ? TEXT("Community night is active. Talk around, then use the roads before the crowd disperses at 02:30.")
                               : TEXT("Hall opens properly at 18:30. Daytime is for jobs, repairs and planning the evening.");

        case EGTTSocialRole::MechanicLocal:
            if (NearestOwnedVehicle)
            {
                return FString::Printf(TEXT("Your nearest %s is at %.0f%% condition, %.0f%% fuel, tires %.0f%%. Workshop first if you plan a hard run."),
                    *NearestOwnedVehicle->GetVehicleDisplayName().ToString(),
                    NearestOwnedVehicle->GetConditionPercent() * 100.0f,
                    NearestOwnedVehicle->GetFuelPercent() * 100.0f,
                    NearestOwnedVehicle->GetTireIntegrity() * 100.0f);
            }
            return TEXT("Bring an owned vehicle near the village and I can tell you what shape it is in.");

        case EGTTSocialRole::FarmerLocal:
        default:
            return ConversationIndex % 3 == 0 ? TEXT("North Wood pays honest money if your tractor can survive the load.")
                : ConversationIndex % 3 == 1 ? TEXT("Mud eats weak tires. A cheap shortcut can turn into an expensive tow.")
                : TEXT("If the warden is already looking for you, finish that problem before taking legal farm work.");
    }
}
