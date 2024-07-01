#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

/**  ĳ������ ����(ü��, ����, �¸�Ƚ�� ��)�� �����ִ� UserWidget Ŭ����
 * 
 */

UCLASS(Blueprintable)
class MULTIPLAYER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UCharacterOverlay(const FObjectInitializer& ObjectInitializer);

	//void ChangeWeaponIcon(TObjectPtr<UTexture2D> texture);
	void ChangeWeaponImage(EWeaponType weaponType);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar; // ü�� ��

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText; // ü�� ����

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ShieldBar; // �ǵ� ��

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText; // �ǵ� ����

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreAmount; // ����

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RedTeamScore; // Red Team ����
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlueTeamScore;// Blue Team ����

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreSpacerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefeatsAmount; // �¸� Ƚ��

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponAmmoAmount; // ���� �Ѿ� ��

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CarriedAmmoAmount; // źâ�� ���� �� �ִ� �ִ� �Ѿ� ��

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MatchCountdownText; // ���� �ð�

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GrenadesText; // ����ź ��

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighPingImage; // Ping�� ������ ������ �̹���

	UPROPERTY(meta = (BindWidgetAnim), Transient) // Transient�� ����ȭ �������� ���õǴ� ������ ������ �� ���
	TObjectPtr<UWidgetAnimation> HighPingAnimation;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_AssaultRifle; 
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_RocketLauncher;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_Pistol;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_SubmachineGun;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_Shotgun;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_SniperRifle;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_GrenadeLauncher;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EWT_Flag;
};