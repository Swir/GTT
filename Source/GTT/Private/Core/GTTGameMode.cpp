#include "Core/GTTGameMode.h"
#include "Characters/GTTCharacter.h"

AGTTGameMode::AGTTGameMode()
{
    DefaultPawnClass = AGTTCharacter::StaticClass();
}
