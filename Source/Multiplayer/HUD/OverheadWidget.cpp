#include "OverheadWidget.h"
#include "Components/TextBlock.h"

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (IsValid(DisplayText))
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));//TextToDisplay 변수로 들어오는 text로 설정.
	}
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole RemoteRole = InPawn->GetRemoteRole();// Pawn의 Role이 뭔지 담는 변수
	FString Role;
	switch (RemoteRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("Autonomous Proxy");
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString("Simulated Proxy");
		break;
	case ENetRole::ROLE_None:
		Role = FString("None");
		break;
	}
	FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
	SetDisplayText(RemoteRoleString);// Pawn의 Role이 무엇인지 화면에 띄워준다.
}

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	RemoveFromParent();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}