#include "MainHUD.h"

void AMainHUD::DrawHUD() //HUD 그리기(=Crosshair 그리기)
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (IsValid(GEngine))
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize); // 화면 크기를 ViewportSize변수에 담는다.
		const FVector2D ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);// 화면중앙 위치를 ViewportCenter 변수에 담는다.

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
	const float TextureWidth = Texture->GetSizeX();  // Texture 너비
	const float TextureHeight = Texture->GetSizeY(); // Texture 높이
	// Texture 그리기 위치 설정
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.0f),
		ViewportCenter.Y - (TextureHeight / 2.0f)
	);

	// Texture 그리기
	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.0f,0.0f,1.0f,1.0f, FLinearColor::White);
}
