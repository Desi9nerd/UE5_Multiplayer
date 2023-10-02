#include "MainHUD.h"

void AMainHUD::DrawHUD() //HUD �׸���(=Crosshair �׸���)
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (IsValid(GEngine))
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize); // ȭ�� ũ�⸦ ViewportSize������ ��´�.
		const FVector2D ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);// ȭ���߾� ��ġ�� ViewportCenter ������ ��´�.

		if (IsValid(HUDPackage.CrosshairCenter))
		{
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter);
		}
		if (IsValid(HUDPackage.CrosshairLeft))
		{
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter);
		}
		if (IsValid(HUDPackage.CrosshairRight))
		{
			DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter);
		}
		if (IsValid(HUDPackage.CrosshairTop))
		{
			DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter);
		}
		if (IsValid(HUDPackage.CrosshairBottom))
		{
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter);
		}
	}
}

void AMainHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter)
{
	const float TextureWidth = Texture->GetSizeX();  // Texture �ʺ�
	const float TextureHeight = Texture->GetSizeY(); // Texture ����
	// Texture �׸��� ��ġ ����
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.0f),
		ViewportCenter.Y - (TextureHeight / 2.0f)
	);

	// Texture �׸���
	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.0f,0.0f,1.0f,1.0f, FLinearColor::White);
}
