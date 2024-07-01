#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UCharacterOverlay;
class UElimAnnouncement;
class UAnnouncement;

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
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }

	virtual void DrawHUD() override;
	void AddCharacterOverlay();
	void AddAnnouncement();
	void AddElimAnnouncement(FString Attacker, FString Victim);

	UPROPERTY(EditAnywhere, Category = "Player UI")
	TSubclassOf<UUserWidget> CharacterOverlayClass;
	UPROPERTY()
	TObjectPtr<UCharacterOverlay> CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Player UI")
	TSubclassOf<UUserWidget> AnnouncementClass;
	UPROPERTY()
	TObjectPtr<UAnnouncement> Announcement;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY()
	APlayerController* OwningPlayer;

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.0f; // Crosshair 퍼지는 정도 최대값

	UPROPERTY(EditAnywhere)
	TSubclassOf<UElimAnnouncement> ElimAnnouncementClass;
	UPROPERTY(EditAnywhere)
	float ElimAnnouncementTime = 2.5f; // 화면에 띄우는 시간
	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMessages;

	FHUDPackage HUDPackage;
};