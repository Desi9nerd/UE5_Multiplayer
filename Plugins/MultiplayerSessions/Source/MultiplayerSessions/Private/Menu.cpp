#include "Menu.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"

void UMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PlayerController = World->GetFirstPlayerController();
		if (PlayerController)
		{
			FInputModeUIOnly InputModeData;
			InputModeData.SetWidgetToFocus(TakeWidget());
			InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputModeData);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		//헤더에서 만든 MultiplayerSessionsSubsystem 변수에 Subsystem을 리턴 값을 넣어준다.
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
	}
}

bool UMenu::Initialize()
{
	//Super버젼이 false면 false를 리턴한다.(=상위 클래스에서 Initialize 하는게 없으면 false를 리턴하여 Initialize()를 하지 않는다.)
	if (!Super::Initialize())
	{
		return false;
	}

	if (HostButton)
	{
		//HostButton 클릭할 때 발생하는 이벤트를 Delegate로 연결해준다.
		HostButton->OnClicked.AddDynamic(this, &ThisClass::HostButtonClicked);
	}
	if (JoinButton)
	{
		//JoinButton 클릭할 때 발생하는 이벤트를 Delegate로 연결해준다.
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::JoinButtonClicked);
	}

	return true;
}

void UMenu::HostButtonClicked()
{
	//디버깅 테스트
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString(TEXT("Host Button Clicked"))
		);
	}

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->CreateSession(4, FString("FreeForAll"));
	}
}

void UMenu::JoinButtonClicked()
{
	//디버깅 테스트
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			15.f,
			FColor::Yellow,
			FString(TEXT("Join Button Clicked"))
		);
	}
}