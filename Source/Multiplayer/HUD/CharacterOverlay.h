#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterOverlay.generated.h"

/**  캐릭터의 상태(체력 등)을 보여주는 UserWidget 클래스
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

};
