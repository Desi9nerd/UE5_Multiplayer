#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

/** HUD. UserWidget들을 통합하여 관리하는 HUD 클래스
 *  HUD는 GameMode에 등록된다.
 */

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	class UTexture2D* CrosshairCenter;
	UTexture2D* CrosshairLeft;
	UTexture2D* CrosshairRight;
	UTexture2D* CrosshairTop;
	UTexture2D* CrosshairBottom;
	float CrosshairSpread;
	FLinearColor CrosshairColor;
};

UCLASS()
class MULTIPLAYER_API AMainHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Player Attributes")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;
	
	TWeakObjectPtr<class UCharacterOverlay> CharacterOverlay;

protected:
	virtual void BeginPlay() override;
	void AddCharacterOverlay();

private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.0f; // Crosshair 퍼지는 정도 최대값

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};