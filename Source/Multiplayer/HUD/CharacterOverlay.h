#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**  캐릭터의 상태(체력, 점수, 승리횟수 등)을 보여주는 UserWidget 클래스
 * 
 */
UCLASS()
class MULTIPLAYER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar; // 체력 바

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText; // 체력 숫자

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar; // 실드 바

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText; // 실드 숫자

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount; // 점수

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount; // 승리 횟수

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount; // 현재 총알 수

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount; // 탄창이 가질 수 있는 최대 총알 수

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText; // 남은 시간

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesText; // 수류탄 수
};
