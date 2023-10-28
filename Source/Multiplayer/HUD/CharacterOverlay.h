#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**  ĳ������ ����(ü��, ����, �¸�Ƚ�� ��)�� �����ִ� UserWidget Ŭ����
 * 
 */
UCLASS()
class MULTIPLAYER_API UCharacterOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HealthBar; // ü�� ��

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HealthText; // ü�� ����

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ShieldBar; // �ǵ� ��

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ShieldText; // �ǵ� ����

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreAmount; // ����

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DefeatsAmount; // �¸� Ƚ��

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponAmmoAmount; // ���� �Ѿ� ��

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CarriedAmmoAmount; // źâ�� ���� �� �ִ� �ִ� �Ѿ� ��

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchCountdownText; // ���� �ð�

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadesText; // ����ź ��
};
