#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "CharacterOverlay.generated.h"

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
	class UProgressBar* HealthBar; // ü�� ��

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* HealthText; // ü�� ����

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* ShieldBar; // �ǵ� ��

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ShieldText; // �ǵ� ����

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ScoreAmount; // ����

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedTeamScore; // Red Team ����
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueTeamScore;// Blue Team ����

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ScoreSpacerText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DefeatsAmount; // �¸� Ƚ��

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* WeaponAmmoAmount; // ���� �Ѿ� ��

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CarriedAmmoAmount; // źâ�� ���� �� �ִ� �ִ� �Ѿ� ��

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* MatchCountdownText; // ���� �ð�

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GrenadesText; // ����ź ��

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* HighPingImage; // Ping�� ������ ������ �̹���

	UPROPERTY(meta = (BindWidgetAnim), Transient) // Transient�� ����ȭ �������� ���õǴ� ������ ������ �� ���
	UWidgetAnimation* HighPingAnimation;

	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_AssaultRifle; 
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_RocketLauncher;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_Pistol;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_SubmachineGun;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_Shotgun;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_SniperRifle;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_GrenadeLauncher;
	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* EWT_Flag;
};