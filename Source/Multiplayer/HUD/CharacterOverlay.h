#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**  ĳ������ ����(ü�� ��)�� �����ִ� UserWidget Ŭ����
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

};
