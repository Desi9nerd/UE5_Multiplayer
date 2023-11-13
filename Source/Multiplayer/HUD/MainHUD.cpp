#include "MainHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Announcement.h"
#include "ElimAnnouncement.h"

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AMainHUD::AddCharacterOverlay()
{
	TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayerController();
	if (PlayerController.IsValid() && CharacterOverlayClass)
	{
		CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController.Get(), CharacterOverlayClass);
		CharacterOverlay->AddToViewport(); // Viewport�� ���
	}
}

void AMainHUD::AddAnnouncement()
{
	TWeakObjectPtr<APlayerController> PlayerController = GetOwningPlayerController();
	if (PlayerController.IsValid() && AnnouncementClass)
	{
		Announcement = CreateWidget<UAnnouncement>(PlayerController.Get(), AnnouncementClass);
		Announcement->AddToViewport();
	}
}

void AMainHUD::AddElimAnnouncement(FString Attacker, FString Victim)
{
	OwningPlayer = OwningPlayer == nullptr ? GetOwningPlayerController() : OwningPlayer;
	if (IsValid(OwningPlayer) && IsValid(ElimAnnouncementClass))
	{
		TObjectPtr<UElimAnnouncement> ElimAnouncementWidget = CreateWidget<UElimAnnouncement>(OwningPlayer, ElimAnnouncementClass);
		if (ElimAnouncementWidget)
		{
			// '������ �÷��̾�'�� '�ǰ� ���ؼ� ���� �÷��̾�'�� ȭ�鿡 ����.
			ElimAnouncementWidget->SetElimAnnouncementText(Attacker, Victim);
			ElimAnouncementWidget->AddToViewport();
		}
	}
}

void AMainHUD::DrawHUD() //HUD �׸���(=Crosshair �׸���)
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (IsValid(GEngine))
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize); // ȭ�� ũ�⸦ ViewportSize������ ��´�.
		const FVector2D ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);// ȭ���߾� ��ġ�� ViewportCenter ������ ��´�.
		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread; // Crosshair ������ ����

		if (IsValid(HUDPackage.CrosshairCenter))
		{
			FVector2D Spread(0.0f, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (IsValid(HUDPackage.CrosshairLeft))
		{
			FVector2D Spread(-SpreadScaled, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (IsValid(HUDPackage.CrosshairRight))
		{
			FVector2D Spread(SpreadScaled, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (IsValid(HUDPackage.CrosshairTop))
		{
			FVector2D Spread(0.0f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if (IsValid(HUDPackage.CrosshairBottom))
		{
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
	}
}

void AMainHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();  // Texture �ʺ�
	const float TextureHeight = Texture->GetSizeY(); // Texture ����
	// Texture �׸��� ��ġ ����
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.0f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.0f) + Spread.Y
	);

	// Texture �׸���
	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.0f,0.0f,1.0f,1.0f, CrosshairColor);
}
