#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Multiplayer/EnumTypes/EWeaponTypes.h"
#include "CharacterOverlay.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

/**  캐릭터의 상태(체력, 점수, 승리횟수 등)을 보여주는 UserWidget 클래스
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
	TObjectPtr<UProgressBar> HealthBar; // 체력 바

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText; // 체력 숫자

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ShieldBar; // 실드 바

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText; // 실드 숫자

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreAmount; // 점수

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RedTeamScore; // Red Team 점수
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlueTeamScore;// Blue Team 점수

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreSpacerText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DefeatsAmount; // 승리 횟수

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponAmmoAmount; // 현재 총알 수

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CarriedAmmoAmount; // 탄창이 가질 수 있는 최대 총알 수

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MatchCountdownText; // 남은 시간

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GrenadesText; // 수류탄 수

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> HighPingImage; // Ping이 높을때 나오는 이미지

	UPROPERTY(meta = (BindWidgetAnim), Transient) // Transient는 직렬화 과정에서 무시되는 변수를 정의할 때 사용
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