#include "MainHUD.h"

void AMainHUD::DrawHUD() //HUD 그리기(=Crosshair 그리기)
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (IsValid(GEngine))
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize); // 화면 크기를 ViewportSize변수에 담는다.
		const FVector2D ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f);// 화면중앙 위치를 ViewportCenter 변수에 담는다.
		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread; // Crosshair 퍼지는 정도

		if (IsValid(HUDPackage.CrosshairCenter))
		{
			FVector2D Spread(0.0f, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, Spread);
		}
		if (IsValid(HUDPackage.CrosshairLeft))
		{
			FVector2D Spread(-SpreadScaled, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, Spread);
		}
		if (IsValid(HUDPackage.CrosshairRight))
		{
			FVector2D Spread(SpreadScaled, 0.0f);
			DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, Spread);
		}
		if (IsValid(HUDPackage.CrosshairTop))
		{
			FVector2D Spread(0.0f, -SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, Spread);
		}
		if (IsValid(HUDPackage.CrosshairBottom))
		{
			FVector2D Spread(0.f, SpreadScaled);
			DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, Spread);
		}
	}
}

void AMainHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread)
{
	const float TextureWidth = Texture->GetSizeX();  // Texture 너비
	const float TextureHeight = Texture->GetSizeY(); // Texture 높이
	// Texture 그리기 위치 설정
	const FVector2D TextureDrawPoint(
		ViewportCenter.X - (TextureWidth / 2.0f) + Spread.X,
		ViewportCenter.Y - (TextureHeight / 2.0f) + Spread.Y
	);

	// Texture 그리기
	DrawTexture(Texture, TextureDrawPoint.X, TextureDrawPoint.Y, TextureWidth, TextureHeight, 0.0f,0.0f,1.0f,1.0f, FLinearColor::White);
}
