#include "CharacterOverlay.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"

UCharacterOverlay::UCharacterOverlay(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (false == IsValid(HealthBar))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("binding HealthBar is failed. Check WBP name to match HealthBar variable")));		
	}
}

void UCharacterOverlay::ChangeWeaponImage(EWeaponType weaponType)
{
	EWT_AssaultRifle->SetVisibility(ESlateVisibility::Hidden);
	EWT_RocketLauncher->SetVisibility(ESlateVisibility::Hidden);
	EWT_Pistol->SetVisibility(ESlateVisibility::Hidden);
	EWT_SubmachineGun->SetVisibility(ESlateVisibility::Hidden);
	EWT_Shotgun->SetVisibility(ESlateVisibility::Hidden);
	EWT_SniperRifle->SetVisibility(ESlateVisibility::Hidden);
	EWT_GrenadeLauncher->SetVisibility(ESlateVisibility::Hidden);
	EWT_Flag->SetVisibility(ESlateVisibility::Hidden);

	switch (weaponType)
	{
	case EWeaponType::EWT_AssaultRifle:
		EWT_AssaultRifle->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_RocketLauncher:
		EWT_RocketLauncher->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_Pistol:
		EWT_Pistol->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_SubmachineGun:
		EWT_SubmachineGun->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_Shotgun:
		EWT_Shotgun->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_SniperRifle:
		EWT_SniperRifle->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_GrenadeLauncher:
		EWT_GrenadeLauncher->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_Flag:
		EWT_Flag->SetVisibility(ESlateVisibility::Visible);
		break;
	case EWeaponType::EWT_MAX:
		break;
	}
}
